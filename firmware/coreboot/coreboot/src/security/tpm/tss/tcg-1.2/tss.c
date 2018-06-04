/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* A lightweight TPM command library.
 *
 * The general idea is that TPM commands are array of bytes whose
 * fields are mostly compile-time constant.  The goal is to build much
 * of the commands at compile time (or build time) and change some of
 * the fields at run time as needed.  The code in
 * utility/tlcl_generator.c builds structures containing the commands,
 * as well as the offsets of the fields that need to be set at run
 * time.
 */

#include <arch/early_variables.h>
#include <assert.h>
#include <string.h>
#include <security/tpm/tis.h>
#include <vb2_api.h>
#include <security/tpm/tss.h>
#include "tss_internal.h"
#include "tss_structures.h"

#ifdef FOR_TEST
#include <stdio.h>
#define VBDEBUG(format, args...) printf(format, ## args)
#else
#include <console/console.h>
#define VBDEBUG(format, args...) printk(BIOS_DEBUG, format, ## args)
#endif

static int tpm_send_receive(const uint8_t *request,
				uint32_t request_length,
				uint8_t *response,
				uint32_t *response_length)
{
	size_t len = *response_length;
	if (tis_sendrecv(request, request_length, response, &len))
		return VB2_ERROR_UNKNOWN;
	/* check 64->32bit overflow and (re)check response buffer overflow */
	if (len > *response_length)
		return VB2_ERROR_UNKNOWN;
	*response_length = len;
	return VB2_SUCCESS;
}

/* Sets the size field of a TPM command. */
static inline void set_tpm_command_size(uint8_t *buffer, uint32_t size)
{
	to_tpm_uint32(buffer + sizeof(uint16_t), size);
}

/* Gets the size field of a TPM command. */
__attribute__((unused))
static inline int tpm_command_size(const uint8_t *buffer)
{
	uint32_t size;
	from_tpm_uint32(buffer + sizeof(uint16_t), &size);
	return (int) size;
}

/* Gets the code field of a TPM command. */
static inline int tpm_command_code(const uint8_t *buffer)
{
	uint32_t code;
	from_tpm_uint32(buffer + sizeof(uint16_t) + sizeof(uint32_t), &code);
	return code;
}

/* Gets the return code field of a TPM result. */
static inline int tpm_return_code(const uint8_t *buffer)
{
	return tpm_command_code(buffer);
}

/* Like TlclSendReceive below, but do not retry if NEEDS_SELFTEST or
 * DOING_SELFTEST errors are returned.
 */
static uint32_t tlcl_send_receive_no_retry(const uint8_t *request,
					   uint8_t *response, int max_length)
{
	uint32_t response_length = max_length;
	uint32_t result;

	result = tpm_send_receive(request, tpm_command_size(request),
					response, &response_length);
	if (result != 0) {
		/* Communication with TPM failed, so response is garbage */
		VBDEBUG("TPM: command 0x%x send/receive failed: 0x%x\n",
			tpm_command_code(request), result);
		return result;
	}
	/* Otherwise, use the result code from the response */
	result = tpm_return_code(response);

	/* TODO: add paranoia about returned response_length vs. max_length
	 * (and possibly expected length from the response header).  See
	 * crosbug.com/17017 */

	VBDEBUG("TPM: command 0x%x returned 0x%x\n",
		tpm_command_code(request), result);

return result;
}


/* Sends a TPM command and gets a response.  Returns 0 if success or the TPM
 * error code if error. Waits for the self test to complete if needed. */
uint32_t tlcl_send_receive(const uint8_t *request, uint8_t *response,
			   int max_length)
{
	uint32_t result = tlcl_send_receive_no_retry(request, response,
						     max_length);
	/* If the command fails because the self test has not completed, try it
	 * again after attempting to ensure that the self test has completed. */
	if (result == TPM_E_NEEDS_SELFTEST || result == TPM_E_DOING_SELFTEST) {
		result = tlcl_continue_self_test();
		if (result != TPM_SUCCESS)
			return result;
#if defined(TPM_BLOCKING_CONTINUESELFTEST) || defined(VB_RECOVERY_MODE)
		/* Retry only once */
		result = tlcl_send_receive_no_retry(request, response,
						    max_length);
#else
		/* This needs serious testing. The TPM specification says: "iii.
		 * The caller MUST wait for the actions of TPM_ContinueSelfTest
		 * to complete before reissuing the command C1."  But, if
		 * ContinueSelfTest is non-blocking, how do we know that the
		 * actions have completed other than trying again? */
		do {
			result = tlcl_send_receive_no_retry(request, response,
							    max_length);
		} while (result == TPM_E_DOING_SELFTEST);
#endif
	}
	return result;
}

/* Sends a command and returns the error code. */
static uint32_t send(const uint8_t *command)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	return tlcl_send_receive(command, response, sizeof(response));
}

/* Exported functions. */

static uint8_t tlcl_init_done CAR_GLOBAL;

uint32_t tlcl_lib_init(void)
{
	uint8_t done = car_get_var(tlcl_init_done);
	if (done)
		return VB2_SUCCESS;

	if (tis_init())
		return VB2_ERROR_UNKNOWN;
	if (tis_open())
		return VB2_ERROR_UNKNOWN;

	car_set_var(tlcl_init_done, 1);

	return VB2_SUCCESS;
}

uint32_t tlcl_startup(void)
{
	VBDEBUG("TPM: Startup\n");
	return send(tpm_startup_cmd.buffer);
}

uint32_t tlcl_resume(void)
{
	VBDEBUG("TPM: Resume\n");
	return send(tpm_resume_cmd.buffer);
}

uint32_t tlcl_self_test_full(void)
{
	VBDEBUG("TPM: Self test full\n");
	return send(tpm_selftestfull_cmd.buffer);
}

uint32_t tlcl_continue_self_test(void)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	VBDEBUG("TPM: Continue self test\n");
	/* Call the No Retry version of SendReceive to avoid recursion. */
	return tlcl_send_receive_no_retry(tpm_continueselftest_cmd.buffer,
					  response, sizeof(response));
}

uint32_t tlcl_define_space(uint32_t index, uint32_t perm, uint32_t size)
{
	struct s_tpm_nv_definespace_cmd cmd;
	VBDEBUG("TPM: TlclDefineSpace(0x%x, 0x%x, %d)\n", index, perm, size);
	memcpy(&cmd, &tpm_nv_definespace_cmd, sizeof(cmd));
	to_tpm_uint32(cmd.buffer + tpm_nv_definespace_cmd.index, index);
	to_tpm_uint32(cmd.buffer + tpm_nv_definespace_cmd.perm, perm);
	to_tpm_uint32(cmd.buffer + tpm_nv_definespace_cmd.size, size);
	return send(cmd.buffer);
}

uint32_t tlcl_write(uint32_t index, const void *data, uint32_t length)
{
	struct s_tpm_nv_write_cmd cmd;
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	const int total_length =
			kTpmRequestHeaderLength + kWriteInfoLength + length;

	VBDEBUG("TPM: tlcl_write(0x%x, %d)\n", index, length);
	memcpy(&cmd, &tpm_nv_write_cmd, sizeof(cmd));
	assert(total_length <= TPM_LARGE_ENOUGH_COMMAND_SIZE);
	set_tpm_command_size(cmd.buffer, total_length);

	to_tpm_uint32(cmd.buffer + tpm_nv_write_cmd.index, index);
	to_tpm_uint32(cmd.buffer + tpm_nv_write_cmd.length, length);
	memcpy(cmd.buffer + tpm_nv_write_cmd.data, data, length);

	return tlcl_send_receive(cmd.buffer, response, sizeof(response));
}

uint32_t tlcl_read(uint32_t index, void *data, uint32_t length)
{
	struct s_tpm_nv_read_cmd cmd;
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t result_length;
	uint32_t result;

	VBDEBUG("TPM: tlcl_read(0x%x, %d)\n", index, length);
	memcpy(&cmd, &tpm_nv_read_cmd, sizeof(cmd));
	to_tpm_uint32(cmd.buffer + tpm_nv_read_cmd.index, index);
	to_tpm_uint32(cmd.buffer + tpm_nv_read_cmd.length, length);

	result = tlcl_send_receive(cmd.buffer, response, sizeof(response));
	if (result == TPM_SUCCESS && length > 0) {
		uint8_t *nv_read_cursor = response + kTpmResponseHeaderLength;
		from_tpm_uint32(nv_read_cursor, &result_length);
		if (result_length > length)
			return TPM_E_IOERROR;
		nv_read_cursor += sizeof(uint32_t);
		memcpy(data, nv_read_cursor, result_length);
	}

	return result;
}


uint32_t tlcl_assert_physical_presence(void)
{
	VBDEBUG("TPM: Asserting physical presence\n");
	return send(tpm_ppassert_cmd.buffer);
}

uint32_t tlcl_physical_presence_cmd_enable(void)
{
	VBDEBUG("TPM: Enable the physical presence command\n");
	return send(tpm_ppenable_cmd.buffer);
}

uint32_t tlcl_finalize_physical_presence(void)
{
	VBDEBUG("TPM: Enable PP cmd, disable HW pp, and set lifetime lock\n");
	return send(tpm_finalizepp_cmd.buffer);
}

uint32_t tlcl_set_nv_locked(void)
{
	VBDEBUG("TPM: Set NV locked\n");
	return tlcl_define_space(TPM_NV_INDEX_LOCK, 0, 0);
}

uint32_t tlcl_force_clear(void)
{
	VBDEBUG("TPM: Force clear\n");
	return send(tpm_forceclear_cmd.buffer);
}

uint32_t tlcl_set_enable(void)
{
	VBDEBUG("TPM: Enabling TPM\n");
	return send(tpm_physicalenable_cmd.buffer);
}

uint32_t tlcl_set_deactivated(uint8_t flag)
{
	struct s_tpm_physicalsetdeactivated_cmd cmd;
	VBDEBUG("TPM: SetDeactivated(%d)\n", flag);
	memcpy(&cmd, &tpm_physicalsetdeactivated_cmd, sizeof(cmd));
	*(cmd.buffer + cmd.deactivated) = flag;
	return send(cmd.buffer);
}

uint32_t tlcl_get_permanent_flags(TPM_PERMANENT_FLAGS *pflags)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t size;
	uint32_t result = tlcl_send_receive(tpm_getflags_cmd.buffer, response,
					    sizeof(response));
	if (result != TPM_SUCCESS)
		return result;
	from_tpm_uint32(response + kTpmResponseHeaderLength, &size);
	if (size != sizeof(TPM_PERMANENT_FLAGS))
		return TPM_E_IOERROR;
	memcpy(pflags, response + kTpmResponseHeaderLength + sizeof(size),
	       sizeof(TPM_PERMANENT_FLAGS));
	return result;
}

uint32_t tlcl_get_flags(uint8_t *disable, uint8_t *deactivated,
			uint8_t *nvlocked)
{
	TPM_PERMANENT_FLAGS pflags;
	uint32_t result = tlcl_get_permanent_flags(&pflags);
	if (result == TPM_SUCCESS) {
		if (disable)
			*disable = pflags.disable;
		if (deactivated)
			*deactivated = pflags.deactivated;
		if (nvlocked)
			*nvlocked = pflags.nvLocked;
		VBDEBUG("TPM: flags disable=%d, deactivated=%d, nvlocked=%d\n",
			pflags.disable, pflags.deactivated, pflags.nvLocked);
	}
	return result;
}

uint32_t tlcl_set_global_lock(void)
{
	uint32_t x;
	VBDEBUG("TPM: Set global lock\n");
	return tlcl_write(TPM_NV_INDEX0, (uint8_t *) &x, 0);
}

uint32_t tlcl_extend(int pcr_num, const uint8_t *in_digest,
		     uint8_t *out_digest)
{
	struct s_tpm_extend_cmd cmd;
	uint8_t response[kTpmResponseHeaderLength + kPcrDigestLength];
	uint32_t result;

	memcpy(&cmd, &tpm_extend_cmd, sizeof(cmd));
	to_tpm_uint32(cmd.buffer + tpm_extend_cmd.pcrNum, pcr_num);
	memcpy(cmd.buffer + cmd.inDigest, in_digest, kPcrDigestLength);

	result = tlcl_send_receive(cmd.buffer, response, sizeof(response));
	if (result != TPM_SUCCESS)
		return result;

	if (out_digest)
		memcpy(out_digest, response + kTpmResponseHeaderLength,
		       kPcrDigestLength);
	return result;
}
