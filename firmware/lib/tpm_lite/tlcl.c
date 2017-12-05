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

#include "2sysincludes.h"
#include "2common.h"
#include "2hmac.h"
#include "2sha.h"

#include "sysincludes.h"
#include "tlcl.h"
#include "tlcl_internal.h"
#include "tlcl_structures.h"
#include "utility.h"
#include "vboot_api.h"

/* Sets the size field of a TPM command. */
static inline void SetTpmCommandSize(uint8_t* buffer, uint32_t size)
{
	ToTpmUint32(buffer + sizeof(uint16_t), size);
}

/* Gets the size field of a TPM command. */
__attribute__((unused))
static inline int TpmCommandSize(const uint8_t* buffer)
{
	uint32_t size;
	FromTpmUint32(buffer + sizeof(uint16_t), &size);
	return (int) size;
}

/* Gets the size field of a TPM request or response. */
int TlclPacketSize(const uint8_t* packet)
{
	return TpmCommandSize(packet);
}

/* Gets the code field of a TPM command. */
static inline int TpmCommandCode(const uint8_t* buffer)
{
	uint32_t code;
	FromTpmUint32(buffer + sizeof(uint16_t) + sizeof(uint32_t), &code);
	return code;
}

/* Gets the return code field of a TPM result. */
static inline int TpmReturnCode(const uint8_t* buffer)
{
	return TpmCommandCode(buffer);
}

/* Like TlclSendReceive below, but do not retry if NEEDS_SELFTEST or
 * DOING_SELFTEST errors are returned.
 */
static uint32_t TlclSendReceiveNoRetry(const uint8_t* request,
                                       uint8_t* response, int max_length)
{

	uint32_t response_length = max_length;
	uint32_t result;

#ifdef EXTRA_LOGGING
	VB2_DEBUG("TPM: command: %x%x %x%x%x%x %x%x%x%x\n",
		  request[0], request[1],
		  request[2], request[3], request[4], request[5],
		  request[6], request[7], request[8], request[9]);
#endif

	result = VbExTpmSendReceive(request, TpmCommandSize(request),
				    response, &response_length);
	if (0 != result) {
		/* Communication with TPM failed, so response is garbage */
		VB2_DEBUG("TPM: command 0x%x send/receive failed: 0x%x\n",
			  TpmCommandCode(request), result);
		return result;
	}
	/* Otherwise, use the result code from the response */
	result = TpmReturnCode(response);

	/* TODO: add paranoia about returned response_length vs. max_length
	 * (and possibly expected length from the response header).  See
	 * crosbug.com/17017 */

#ifdef EXTRA_LOGGING
	VB2_DEBUG("TPM: response: %x%x %x%x%x%x %x%x%x%x\n",
		  response[0], response[1],
		  response[2], response[3], response[4], response[5],
		  response[6], response[7], response[8], response[9]);
#endif

	VB2_DEBUG("TPM: command 0x%x returned 0x%x\n",
		  TpmCommandCode(request), result);

	return result;
}

/* Sends a TPM command and gets a response.  Returns 0 if success or the TPM
 * error code if error. In the firmware, waits for the self test to complete
 * if needed. In the host, reports the first error without retries. */
uint32_t TlclSendReceive(const uint8_t* request, uint8_t* response,
                         int max_length)
{
	uint32_t result = TlclSendReceiveNoRetry(request, response, max_length);
	/* When compiling for the firmware, hide command failures due to the
	 * self test not having run or completed. */
#ifndef CHROMEOS_ENVIRONMENT
	/* If the command fails because the self test has not completed, try it
	 * again after attempting to ensure that the self test has completed. */
	if (result == TPM_E_NEEDS_SELFTEST || result == TPM_E_DOING_SELFTEST) {
		result = TlclContinueSelfTest();
		if (result != TPM_SUCCESS) {
			return result;
		}
#if defined(TPM_BLOCKING_CONTINUESELFTEST) || defined(VB_RECOVERY_MODE)
		/* Retry only once */
		result = TlclSendReceiveNoRetry(request, response, max_length);
#else
		/* This needs serious testing.  The TPM specification says:
		 * "iii. The caller MUST wait for the actions of
		 * TPM_ContinueSelfTest to complete before reissuing the
		 * command C1."  But, if ContinueSelfTest is non-blocking, how
		 * do we know that the actions have completed other than trying
		 * again? */
		do {
			result = TlclSendReceiveNoRetry(request, response,
							max_length);
		} while (result == TPM_E_DOING_SELFTEST);
#endif
	}
#endif  /* ! defined(CHROMEOS_ENVIRONMENT) */
	return result;
}

/* Sends a command and returns the error code. */
static uint32_t Send(const uint8_t* command)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	return TlclSendReceive(command, response, sizeof(response));
}

#ifdef CHROMEOS_ENVIRONMENT

struct auth_session
{
	uint32_t handle;
	TPM_NONCE nonce_even;
	TPM_NONCE nonce_odd;
	uint8_t shared_secret[TPM_AUTH_DATA_LEN];
	uint8_t valid;
};

static uint32_t StartOIAPSession(struct auth_session* session,
				 const uint8_t secret[TPM_AUTH_DATA_LEN])
{
	session->valid = 0;

	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t result = TlclSendReceive(tpm_oiap_cmd.buffer, response,
					  sizeof(response));
	if (result != TPM_SUCCESS) {
		return result;
	}

	uint32_t size;
	FromTpmUint32(response + sizeof(uint16_t), &size);
	if (size < kTpmResponseHeaderLength + sizeof(uint32_t)
			+ sizeof(TPM_NONCE)) {
		return TPM_E_INVALID_RESPONSE;
	}

	const uint8_t* cursor = response + kTpmResponseHeaderLength;
	session->handle = ReadTpmUint32(&cursor);
	memcpy(session->nonce_even.nonce, cursor, sizeof(TPM_NONCE));
	cursor += sizeof(TPM_NONCE);
	VbAssert(cursor - response <= TPM_LARGE_ENOUGH_COMMAND_SIZE);

	memcpy(session->shared_secret, secret, TPM_AUTH_DATA_LEN);
	session->valid = 1;

	return result;
}

static uint32_t StartOSAPSession(
		struct auth_session* session,
		uint16_t entity_type,
		uint32_t entity_value,
		const uint8_t entity_usage_auth[TPM_AUTH_DATA_LEN])
{
	session->valid = 0;

	/* Build OSAP command. */
	struct s_tpm_osap_cmd cmd;
	memcpy(&cmd, &tpm_osap_cmd, sizeof(cmd));
	ToTpmUint16(cmd.buffer + cmd.entityType, entity_type);
	ToTpmUint32(cmd.buffer + cmd.entityValue, entity_value);
	if (VbExTpmGetRandom(cmd.buffer + cmd.nonceOddOSAP,
			     sizeof(TPM_NONCE)) != VB2_SUCCESS) {
		return TPM_E_INTERNAL_ERROR;
	}

	/* Send OSAP command. */
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t result = TlclSendReceive(cmd.buffer, response,
					  sizeof(response));
	if (result != TPM_SUCCESS) {
		return result;
	}

	/* Parse response. */
	uint32_t size;
	FromTpmUint32(response + sizeof(uint16_t), &size);
	if (size < kTpmResponseHeaderLength + sizeof(uint32_t)
			+ 2 * sizeof(TPM_NONCE)) {
		return TPM_E_INVALID_RESPONSE;
	}

	const uint8_t* cursor = response + kTpmResponseHeaderLength;
	session->handle = ReadTpmUint32(&cursor);
	memcpy(session->nonce_even.nonce, cursor, sizeof(TPM_NONCE));
	cursor += sizeof(TPM_NONCE);
	const uint8_t* nonce_even_osap = cursor;
	cursor += sizeof(TPM_NONCE);
	VbAssert(cursor - response <= TPM_LARGE_ENOUGH_COMMAND_SIZE);

	/* Compute shared secret */
	uint8_t hmac_input[2 * sizeof(TPM_NONCE)];
	memcpy(hmac_input, nonce_even_osap, sizeof(TPM_NONCE));
	memcpy(hmac_input + sizeof(TPM_NONCE), cmd.buffer + cmd.nonceOddOSAP,
	       sizeof(TPM_NONCE));
	if (hmac(VB2_HASH_SHA1, entity_usage_auth, TPM_AUTH_DATA_LEN,
		 hmac_input, sizeof(hmac_input), session->shared_secret,
		 sizeof(session->shared_secret))) {
		return TPM_E_INTERNAL_ERROR;
	}
	session->valid = 1;

	return result;
}

/* Fills in the authentication block at the end of the command. The command body
 * should already be initialized in |command_buffer|, and the included command
 * size should account for the auth block that gets filled in. */
uint32_t AddRequestAuthBlock(struct auth_session* auth_session,
			     uint8_t* command_buffer,
			     uint32_t command_buffer_size,
			     uint8_t continue_auth_session)
{
	if (!auth_session->valid) {
		return TPM_E_AUTHFAIL;
	}

	/* Sanity check to make sure the command buffer has sufficient space to
	 * add the auth block at the end of the command. */
	if (command_buffer_size < kTpmRequestHeaderLength) {
		return TPM_E_BUFFER_SIZE;
	}
	const uint32_t size = TpmCommandSize(command_buffer);
	if (size < kTpmResponseHeaderLength + kTpmRequestAuthBlockLength ||
	    size > command_buffer_size) {
		return TPM_E_INTERNAL_ERROR;
	}
	const uint32_t auth_offset = size - kTpmRequestAuthBlockLength;

	/*
	 * The digest of the command is computed over the command buffer, but
	 * excluding the leading tag and paramSize fields.
	 */
	struct vb2_sha1_context sha1_ctx;
	vb2_sha1_init(&sha1_ctx);
	vb2_sha1_update(&sha1_ctx,
			command_buffer + sizeof(uint16_t) + sizeof(uint32_t),
			auth_offset - sizeof(uint16_t) - sizeof(uint32_t));
	uint8_t buf[TPM_SHA1_160_HASH_LEN + 2 * sizeof(TPM_NONCE) + 1];
	vb2_sha1_finalize(&sha1_ctx, buf);

	/* Generate a fresh nonce. */
	if (VbExTpmGetRandom(auth_session->nonce_odd.nonce,
			     sizeof(TPM_NONCE)) != VB2_SUCCESS) {
		return TPM_E_INTERNAL_ERROR;
	}

	/* Append the authentication block to the command buffer. */
	uint8_t* cursor = command_buffer + auth_offset;
	ToTpmUint32(cursor, auth_session->handle);
	cursor += sizeof(uint32_t);
	memcpy(cursor, auth_session->nonce_odd.nonce, sizeof(TPM_NONCE));
	cursor += sizeof(TPM_NONCE);
	*cursor++ = continue_auth_session;

	/* Compute and append the MAC. */
	memcpy(buf + TPM_SHA1_160_HASH_LEN, auth_session->nonce_even.nonce,
	       sizeof(TPM_NONCE));
	memcpy(buf + TPM_SHA1_160_HASH_LEN + sizeof(TPM_NONCE),
	       auth_session->nonce_odd.nonce, sizeof(TPM_NONCE));
	buf[TPM_SHA1_160_HASH_LEN + 2 * sizeof(TPM_NONCE)] =
			continue_auth_session;
	if (hmac(VB2_HASH_SHA1, auth_session->shared_secret,
		 sizeof(auth_session->shared_secret), buf, sizeof(buf), cursor,
		 TPM_SHA1_160_HASH_LEN)) {
		return TPM_E_AUTHFAIL;
	}
	cursor += TPM_SHA1_160_HASH_LEN;

	return TPM_SUCCESS;
}

uint32_t CheckResponseAuthBlock(struct auth_session* auth_session,
				TPM_COMMAND_CODE ordinal,
				uint8_t* response_buffer,
				uint32_t response_buffer_size)
{
	if (!auth_session->valid) {
		return TPM_E_AUTHFAIL;
	}

	if (response_buffer_size < kTpmResponseHeaderLength) {
		return TPM_E_INVALID_RESPONSE;
	}

	/* Parse and validate the actual response size from the response. */
	uint32_t size;
	FromTpmUint32(response_buffer + sizeof(uint16_t), &size);
	if (size >= response_buffer_size ||
	    size < kTpmResponseHeaderLength + kTpmResponseAuthBlockLength) {
		return TPM_E_INVALID_RESPONSE;
	}
	uint32_t auth_offset = size - kTpmResponseAuthBlockLength;

	/*
	 * The digest of the response is computed over the return code, ordinal,
	 * response payload.
	 */
	struct vb2_sha1_context sha1_ctx;
	vb2_sha1_init(&sha1_ctx);
	vb2_sha1_update(&sha1_ctx,
			response_buffer + sizeof(uint16_t) + sizeof(uint32_t),
			sizeof(uint32_t));
	uint8_t ordinal_buf[sizeof(ordinal)];
	ToTpmUint32(ordinal_buf, ordinal);
	vb2_sha1_update(&sha1_ctx, ordinal_buf, sizeof(ordinal_buf));
	vb2_sha1_update(&sha1_ctx,
			response_buffer + kTpmResponseHeaderLength,
			auth_offset - kTpmResponseHeaderLength);
	uint8_t hmac_input[TPM_SHA1_160_HASH_LEN + 2 * sizeof(TPM_NONCE) + 1];
	vb2_sha1_finalize(&sha1_ctx, hmac_input);

	/* Compute the MAC. */
	uint8_t* cursor = response_buffer + auth_offset;
	memcpy(hmac_input + TPM_SHA1_160_HASH_LEN, cursor, sizeof(TPM_NONCE));
	cursor += sizeof(TPM_NONCE);
	memcpy(hmac_input + TPM_SHA1_160_HASH_LEN + sizeof(TPM_NONCE),
	       auth_session->nonce_odd.nonce, sizeof(TPM_NONCE));
	auth_session->valid = *cursor++;
	hmac_input[TPM_SHA1_160_HASH_LEN + 2 * sizeof(TPM_NONCE)] =
			auth_session->valid;
	uint8_t mac[TPM_SHA1_160_HASH_LEN];
	if (hmac(VB2_HASH_SHA1, auth_session->shared_secret,
		 sizeof(auth_session->shared_secret), hmac_input,
		 sizeof(hmac_input), mac, sizeof(mac))) {
		auth_session->valid = 0;
		return TPM_E_AUTHFAIL;
	}

	/* Check the MAC. */
	if (vb2_safe_memcmp(mac, cursor, sizeof(mac))) {
		auth_session->valid = 0;
		return TPM_E_AUTHFAIL;
	}

	/* Success, save the even nonce. */
	memcpy(auth_session->nonce_even.nonce, response_buffer + auth_offset,
	       sizeof(TPM_NONCE));

	return TPM_SUCCESS;
}

#endif  /* CHROMEOS_ENVIRONMENT */

/* Exported functions. */

uint32_t TlclLibInit(void)
{
	return VbExTpmInit();
}

uint32_t TlclLibClose(void)
{
	return VbExTpmClose();
}

uint32_t TlclStartup(void)
{
	VB2_DEBUG("TPM: Startup\n");
	return Send(tpm_startup_cmd.buffer);
}

uint32_t TlclSaveState(void)
{
	VB2_DEBUG("TPM: SaveState\n");
	return Send(tpm_savestate_cmd.buffer);
}

uint32_t TlclResume(void)
{
	VB2_DEBUG("TPM: Resume\n");
	return Send(tpm_resume_cmd.buffer);
}

uint32_t TlclSelfTestFull(void)
{
	VB2_DEBUG("TPM: Self test full\n");
	return Send(tpm_selftestfull_cmd.buffer);
}

uint32_t TlclContinueSelfTest(void)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	VB2_DEBUG("TPM: Continue self test\n");
	/* Call the No Retry version of SendReceive to avoid recursion. */
	return TlclSendReceiveNoRetry(tpm_continueselftest_cmd.buffer,
				      response, sizeof(response));
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size)
{
	VB2_DEBUG("TPM: TlclDefineSpace(0x%x, 0x%x, %d)\n", index, perm, size);
	return TlclDefineSpaceEx(NULL, 0, index, perm, size, NULL, 0);
}

uint32_t TlclDefineSpaceEx(const uint8_t* owner_auth, uint32_t owner_auth_size,
			   uint32_t index, uint32_t perm, uint32_t size,
			   const void* auth_policy, uint32_t auth_policy_size)
{
	uint32_t result;

	/* Build the request data. */
	uint8_t cmd[sizeof(tpm_nv_definespace_cmd.buffer) +
			kTpmRequestAuthBlockLength];
	memcpy(cmd, &tpm_nv_definespace_cmd, sizeof(tpm_nv_definespace_cmd));
	ToTpmUint32(cmd + tpm_nv_definespace_cmd.index, index);
	ToTpmUint32(cmd + tpm_nv_definespace_cmd.perm, perm);
	ToTpmUint32(cmd + tpm_nv_definespace_cmd.size, size);
	if (auth_policy != NULL) {
		if (auth_policy_size != sizeof(TPM_NV_AUTH_POLICY)) {
			return TPM_E_BUFFER_SIZE;
		}

		const TPM_NV_AUTH_POLICY* policy = auth_policy;
		memcpy(cmd + tpm_nv_definespace_cmd.pcr_info_read,
		       &policy->pcr_info_read, sizeof(policy->pcr_info_read));
		memcpy(cmd + tpm_nv_definespace_cmd.pcr_info_write,
		       &policy->pcr_info_write, sizeof(policy->pcr_info_write));
	}

#ifdef CHROMEOS_ENVIRONMENT
	struct auth_session auth_session;
	if (owner_auth) {
		if (owner_auth_size != TPM_AUTH_DATA_LEN) {
			return TPM_E_AUTHFAIL;
		}

		result = StartOSAPSession(&auth_session, TPM_ET_OWNER, 0,
					  owner_auth);
		if (result != TPM_SUCCESS) {
			return result;
		}

		ToTpmUint32(cmd + sizeof(uint16_t), sizeof(cmd));
		ToTpmUint16(cmd, TPM_TAG_RQU_AUTH1_COMMAND);
		result = AddRequestAuthBlock(&auth_session, cmd, sizeof(cmd),
					     0);
		if (result != TPM_SUCCESS) {
			return result;
		}
	}
#endif

	/* Send the command. */
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	result = TlclSendReceive(cmd, response, sizeof(response));
	if (result != TPM_SUCCESS) {
		return result;
	}

#ifdef CHROMEOS_ENVIRONMENT
	if (owner_auth) {
		result = CheckResponseAuthBlock(&auth_session,
						TPM_ORD_NV_DefineSpace,
						response, sizeof(response));
	}
#endif

	return result;
}

uint32_t TlclInitNvAuthPolicy(uint32_t pcr_selection_bitmap,
			      const uint8_t pcr_values[][TPM_PCR_DIGEST],
			      void* auth_policy, uint32_t* auth_policy_size)
{
	uint32_t buffer_size = *auth_policy_size;
	*auth_policy_size = sizeof(TPM_NV_AUTH_POLICY);
	if (!auth_policy || buffer_size < sizeof(TPM_NV_AUTH_POLICY)) {
		return TPM_E_BUFFER_SIZE;
	}
	TPM_NV_AUTH_POLICY* policy = auth_policy;

	/* Note that although the struct definition allocates space for 3 bytes
	 * worth of PCR selection, it is technically a variably-sized field in
	 * the TPM structure definition. Since this is part of the policy
	 * digest, we need to carefully match our selection sizes. For now, we
	 * use 3 bytes, which aligns with TrouSerS behavior. */
	TPM_PCR_SELECTION* select = &policy->pcr_info_read.pcrSelection;
	ToTpmUint16((uint8_t*)&select->sizeOfSelect, sizeof(select->pcrSelect));
	select->pcrSelect[0] = (pcr_selection_bitmap >> 0) & 0xff;
	select->pcrSelect[1] = (pcr_selection_bitmap >> 8) & 0xff;
	select->pcrSelect[2] = (pcr_selection_bitmap >> 16) & 0xff;
	VbAssert((pcr_selection_bitmap & 0xff000000) == 0);

	/* Allow all localities except locality 3. Rationale:
	 *
	 * We don't actually care about restricting NVRAM access to specific
	 * localities. In fact localities aren't used in Chrome OS and locality
	 * 0 is used for everything.
	 *
	 * However, the TPM specification makes an effort to not allow NVRAM
	 * spaces that do not have some write access control configured: When
	 * defining a space, either at least one of OWNERWRITE, AUTHWRITE,
	 * WRITEDEFINE, PPWRITE or a locality restriction must be specified (See
	 * TPM_NV_DefineSpace command description in the spec).
	 *
	 * This complicates matters when defining an NVRAM space that should be
	 * writable and lockable (for the remainder of the boot cycle) by the OS
	 * even when the TPM is not owned:
	 *  * OWNERWRITE doesn't work because there might be no owner.
	 *  * PPWRITE restricts writing to firmware only.
	 *  * Use of WRITEDEFINE prevents use of WRITE_STCLEAR (i.e. the space
	 *    can either not be locked, or it would remain locked until next TPM
	 *    clear).
	 *  * AUTHWRITE looks workable at first sight (by setting a well-known
	 *    auth secret). However writes must use TPM_NV_WriteValueAuth, which
	 *    only works when the TPM is owned. Interestingly, the spec admits
	 *    to that being a mistake in the TPM_NV_WriteValueAuth comment but
	 *    asserts that the behavior is normative.
	 *
	 * Having ruled out all attributes, we're left with locality restriction
	 * as the only option to pass the access control requirement check in
	 * TPM_NV_DefineSpace. We choose to disallow locality 3, since that is
	 * the most unlikely one to be used in practice, i.e. the spec says
	 * locality 3 is for "Auxiliary components. Use of this is optional and,
	 * if used, it is implementation dependent."
         */
	policy->pcr_info_read.localityAtRelease =
		TPM_ALL_LOCALITIES & ~TPM_LOC_THREE;

	struct vb2_sha1_context sha1_ctx;
	vb2_sha1_init(&sha1_ctx);

	vb2_sha1_update(&sha1_ctx,
			(const uint8_t*)&policy->pcr_info_read.pcrSelection,
			sizeof(policy->pcr_info_read.pcrSelection));

	uint32_t num_pcrs = 0;
	int i;
	for (i = 0; i < sizeof(pcr_selection_bitmap) * 8; ++i) {
		if ((1 << i) & pcr_selection_bitmap) {
			num_pcrs++;
		}
	}

	uint8_t pcrs_size[sizeof(uint32_t)];
	ToTpmUint32(pcrs_size, num_pcrs * TPM_PCR_DIGEST);
	vb2_sha1_update(&sha1_ctx, pcrs_size, sizeof(pcrs_size));

	for (i = 0; i < num_pcrs; ++i) {
		vb2_sha1_update(&sha1_ctx, pcr_values[i], TPM_PCR_DIGEST);
	}

	vb2_sha1_finalize(&sha1_ctx,
			  policy->pcr_info_read.digestAtRelease.digest);

	/* Make the write policy an identical copy of the read auth policy. */
	memcpy(&policy->pcr_info_write, &policy->pcr_info_read,
	       sizeof(policy->pcr_info_read));

	return TPM_SUCCESS;
}

uint32_t TlclWrite(uint32_t index, const void* data, uint32_t length)
{
	struct s_tpm_nv_write_cmd cmd;
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	const int total_length =
			kTpmRequestHeaderLength + kWriteInfoLength + length;

	VB2_DEBUG("TPM: TlclWrite(0x%x, %d)\n", index, length);
	memcpy(&cmd, &tpm_nv_write_cmd, sizeof(cmd));
	VbAssert(total_length <= TPM_LARGE_ENOUGH_COMMAND_SIZE);
	SetTpmCommandSize(cmd.buffer, total_length);

	ToTpmUint32(cmd.buffer + tpm_nv_write_cmd.index, index);
	ToTpmUint32(cmd.buffer + tpm_nv_write_cmd.length, length);
	memcpy(cmd.buffer + tpm_nv_write_cmd.data, data, length);

	return TlclSendReceive(cmd.buffer, response, sizeof(response));
}

uint32_t TlclRead(uint32_t index, void* data, uint32_t length)
{
	struct s_tpm_nv_read_cmd cmd;
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t result;

	VB2_DEBUG("TPM: TlclRead(0x%x, %d)\n", index, length);
	memcpy(&cmd, &tpm_nv_read_cmd, sizeof(cmd));
	ToTpmUint32(cmd.buffer + tpm_nv_read_cmd.index, index);
	ToTpmUint32(cmd.buffer + tpm_nv_read_cmd.length, length);

	result = TlclSendReceive(cmd.buffer, response, sizeof(response));
	if (result == TPM_SUCCESS && length > 0) {
		const uint8_t* nv_read_cursor =
				response + kTpmResponseHeaderLength;
		uint32_t result_length = ReadTpmUint32(&nv_read_cursor);
		if (result_length > length)
			result_length = length;  /* Truncate to fit buffer */
		memcpy(data, nv_read_cursor, result_length);
	}

	return result;
}

uint32_t TlclPCRRead(uint32_t index, void* data, uint32_t length)
{
	struct s_tpm_pcr_read_cmd cmd;
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t result;

	VB2_DEBUG("TPM: TlclPCRRead(0x%x, %d)\n", index, length);
	if (length < kPcrDigestLength) {
		return TPM_E_IOERROR;
	}
	memcpy(&cmd, &tpm_pcr_read_cmd, sizeof(cmd));
	ToTpmUint32(cmd.buffer + tpm_pcr_read_cmd.pcrNum, index);

	result = TlclSendReceive(cmd.buffer, response, sizeof(response));
	if (result == TPM_SUCCESS) {
		const uint8_t* pcr_read_cursor =
				response + kTpmResponseHeaderLength;
		memcpy(data, pcr_read_cursor, kPcrDigestLength);
	}

	return result;
}

uint32_t TlclWriteLock(uint32_t index)
{
	VB2_DEBUG("TPM: Write lock 0x%x\n", index);
	return TlclWrite(index, NULL, 0);
}

uint32_t TlclReadLock(uint32_t index)
{
	VB2_DEBUG("TPM: Read lock 0x%x\n", index);
	return TlclRead(index, NULL, 0);
}

uint32_t TlclAssertPhysicalPresence(void)
{
	VB2_DEBUG("TPM: Asserting physical presence\n");
	return Send(tpm_ppassert_cmd.buffer);
}

uint32_t TlclPhysicalPresenceCMDEnable(void)
{
	VB2_DEBUG("TPM: Enable the physical presence command\n");
	return Send(tpm_ppenable_cmd.buffer);
}

uint32_t TlclFinalizePhysicalPresence(void)
{
	VB2_DEBUG("TPM: Enable PP cmd, disable HW pp, and set lifetime lock\n");
	return Send(tpm_finalizepp_cmd.buffer);
}

uint32_t TlclAssertPhysicalPresenceResult(void)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	return TlclSendReceive(tpm_ppassert_cmd.buffer, response,
			       sizeof(response));
}

uint32_t TlclLockPhysicalPresence(void)
{
	VB2_DEBUG("TPM: Lock physical presence\n");
	return Send(tpm_pplock_cmd.buffer);
}

uint32_t TlclSetNvLocked(void)
{
	VB2_DEBUG("TPM: Set NV locked\n");
	return TlclDefineSpace(TPM_NV_INDEX_LOCK, 0, 0);
}

int TlclIsOwned(void)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE + TPM_PUBEK_SIZE];
	uint32_t result;
	result = TlclSendReceive(tpm_readpubek_cmd.buffer,
				 response, sizeof(response));
	return (result != TPM_SUCCESS);
}

uint32_t TlclForceClear(void)
{
	VB2_DEBUG("TPM: Force clear\n");
	return Send(tpm_forceclear_cmd.buffer);
}

uint32_t TlclSetEnable(void)
{
	VB2_DEBUG("TPM: Enabling TPM\n");
	return Send(tpm_physicalenable_cmd.buffer);
}

uint32_t TlclClearEnable(void)
{
	VB2_DEBUG("TPM: Disabling TPM\n");
	return Send(tpm_physicaldisable_cmd.buffer);
}

uint32_t TlclSetDeactivated(uint8_t flag)
{
	struct s_tpm_physicalsetdeactivated_cmd cmd;
	VB2_DEBUG("TPM: SetDeactivated(%d)\n", flag);
	memcpy(&cmd, &tpm_physicalsetdeactivated_cmd, sizeof(cmd));
	*(cmd.buffer + cmd.deactivated) = flag;
	return Send(cmd.buffer);
}

uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS* pflags)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t size;
	uint32_t result = TlclSendReceive(tpm_getflags_cmd.buffer, response,
					  sizeof(response));
	if (result != TPM_SUCCESS)
		return result;
	FromTpmUint32(response + kTpmResponseHeaderLength, &size);
	/* TODO(crbug.com/379255): This fails. Find out why.
	 * VbAssert(size == sizeof(TPM_PERMANENT_FLAGS));
	 */
	memcpy(pflags,
	       response + kTpmResponseHeaderLength + sizeof(size),
	       sizeof(TPM_PERMANENT_FLAGS));
	return result;
}

uint32_t TlclGetSTClearFlags(TPM_STCLEAR_FLAGS* vflags)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t size;
	uint32_t result = TlclSendReceive(tpm_getstclearflags_cmd.buffer,
					  response, sizeof(response));
	if (result != TPM_SUCCESS)
		return result;
	FromTpmUint32(response + kTpmResponseHeaderLength, &size);
	/* Ugly assertion, but the struct is padded up by one byte. */
	/* TODO(crbug.com/379255): This fails. Find out why.
	 * VbAssert(size == 7 && sizeof(TPM_STCLEAR_FLAGS) - 1 == 7);
	 */
	memcpy(vflags,
	       response + kTpmResponseHeaderLength + sizeof(size),
	       sizeof(TPM_STCLEAR_FLAGS));
	return result;
}

uint32_t TlclGetFlags(uint8_t* disable,
                      uint8_t* deactivated,
                      uint8_t *nvlocked)
{
	TPM_PERMANENT_FLAGS pflags;
	uint32_t result = TlclGetPermanentFlags(&pflags);
	if (result == TPM_SUCCESS) {
		if (disable)
			*disable = pflags.disable;
		if (deactivated)
			*deactivated = pflags.deactivated;
		if (nvlocked)
			*nvlocked = pflags.nvLocked;
		VB2_DEBUG("TPM: Got flags disable=%d, deactivated=%d, "
			  "nvlocked=%d\n",
			  pflags.disable, pflags.deactivated, pflags.nvLocked);
	}
	return result;
}

uint32_t TlclSetGlobalLock(void)
{
	uint32_t x;
	VB2_DEBUG("TPM: Set global lock\n");
	return TlclWrite(TPM_NV_INDEX0, (uint8_t*) &x, 0);
}

uint32_t TlclExtend(int pcr_num, const uint8_t* in_digest,
                    uint8_t* out_digest)
{
	struct s_tpm_extend_cmd cmd;
	uint8_t response[kTpmResponseHeaderLength + kPcrDigestLength];
	uint32_t result;

	memcpy(&cmd, &tpm_extend_cmd, sizeof(cmd));
	ToTpmUint32(cmd.buffer + tpm_extend_cmd.pcrNum, pcr_num);
	memcpy(cmd.buffer + cmd.inDigest, in_digest, kPcrDigestLength);

	result = TlclSendReceive(cmd.buffer, response, sizeof(response));
	if (result != TPM_SUCCESS)
		return result;

	memcpy(out_digest, response + kTpmResponseHeaderLength,
	       kPcrDigestLength);
	return result;
}

uint32_t TlclGetPermissions(uint32_t index, uint32_t* permissions)
{
	struct s_tpm_getpermissions_cmd cmd;
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint8_t* nvdata;
	uint32_t result;
	uint32_t size;

	memcpy(&cmd, &tpm_getpermissions_cmd, sizeof(cmd));
	ToTpmUint32(cmd.buffer + tpm_getpermissions_cmd.index, index);
	result = TlclSendReceive(cmd.buffer, response, sizeof(response));
	if (result != TPM_SUCCESS)
		return result;

	nvdata = response + kTpmResponseHeaderLength + sizeof(size);
	FromTpmUint32(nvdata + kNvDataPublicPermissionsOffset, permissions);
	return result;
}

uint32_t TlclGetOwnership(uint8_t* owned)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t size;
	uint32_t result = TlclSendReceive(tpm_getownership_cmd.buffer,
					  response, sizeof(response));
	if (result != TPM_SUCCESS)
		return result;
	FromTpmUint32(response + kTpmResponseHeaderLength, &size);
	/* TODO(crbug.com/379255): This fails. Find out why.
	 * VbAssert(size == sizeof(*owned));
	 */
	memcpy(owned,
	       response + kTpmResponseHeaderLength + sizeof(size),
	       sizeof(*owned));
	return result;
}

uint32_t TlclGetRandom(uint8_t* data, uint32_t length, uint32_t *size)
{
	struct s_tpm_get_random_cmd cmd;
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t result;

	VB2_DEBUG("TPM: TlclGetRandom(%d)\n", length);
	memcpy(&cmd, &tpm_get_random_cmd, sizeof(cmd));
	ToTpmUint32(cmd.buffer + tpm_get_random_cmd.bytesRequested, length);
	/* There must be room in the response buffer for the bytes. */
	if (length > TPM_LARGE_ENOUGH_COMMAND_SIZE - kTpmResponseHeaderLength
	    - sizeof(uint32_t)) {
		return TPM_E_IOERROR;
	}

	result = TlclSendReceive(cmd.buffer, response, sizeof(response));
	if (result == TPM_SUCCESS) {
		const uint8_t* get_random_cursor =
				response + kTpmResponseHeaderLength;
		*size = ReadTpmUint32(&get_random_cursor);

		/* There must be room in the target buffer for the bytes. */
		if (*size > length) {
			return TPM_E_RESPONSE_TOO_LARGE;
		}
		memcpy(data, get_random_cursor, *size);
	}

	return result;
}

uint32_t TlclGetVersion(uint32_t* vendor, uint64_t* firmware_version,
                        uint8_t* vendor_specific_buf,
                        size_t* vendor_specific_buf_size)
{
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	uint32_t result = TlclSendReceive(tpm_getversionval_cmd.buffer,
					  response, sizeof(response));
	if (result != TPM_SUCCESS)
		return result;

	const uint8_t* cursor = response + kTpmResponseHeaderLength;
	uint32_t size = ReadTpmUint32(&cursor);

	/* Verify size >= sizeof(TPM_CAP_VERSION_INFO). */
	const uint32_t kSizeofCapVersionInfo = 15;
	if (size < kSizeofCapVersionInfo ||
	    kTpmResponseHeaderLength + sizeof(size) + size >
			TPM_LARGE_ENOUGH_COMMAND_SIZE) {
		return TPM_E_IOERROR;
	}

	cursor += sizeof(uint16_t);  /* tag */
	cursor += sizeof(uint16_t);  /* spec version */

	*firmware_version = ReadTpmUint16(&cursor);

	cursor += sizeof(uint16_t);  /* specLevel */
	cursor += sizeof(uint8_t);  /* errataRev */

	*vendor = ReadTpmUint32(&cursor);

	if (vendor_specific_buf_size) {
		uint16_t vendor_specific_size = ReadTpmUint16(&cursor);

		if (size < kSizeofCapVersionInfo + vendor_specific_size) {
			return TPM_E_IOERROR;
		}
		if (vendor_specific_buf) {
			if (vendor_specific_size > *vendor_specific_buf_size) {
				vendor_specific_size =
					*vendor_specific_buf_size;
			}
			memcpy(vendor_specific_buf, cursor,
			       vendor_specific_size);
			cursor += vendor_specific_size;
		}
		*vendor_specific_buf_size = vendor_specific_size;
	}

	return TPM_SUCCESS;
}

static void ParseIFXFirmwarePackage(const uint8_t** cursor,
				    TPM_IFX_FIRMWAREPACKAGE* firmware_package)
{
	firmware_package->FwPackageIdentifier = ReadTpmUint32(cursor);
	firmware_package->Version = ReadTpmUint32(cursor);
	firmware_package->StaleVersion = ReadTpmUint32(cursor);
}

uint32_t TlclIFXFieldUpgradeInfo(TPM_IFX_FIELDUPGRADEINFO* info)
{
	uint32_t vendor;
	uint64_t firmware_version;
	uint32_t result =
			TlclGetVersion(&vendor, &firmware_version, NULL, NULL);
	if (result != TPM_SUCCESS) {
		return result;
	}
	if (vendor != 0x49465800) {
		return TPM_E_BAD_ORDINAL;
	}

	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE];
	result = TlclSendReceive(tpm_ifx_fieldupgradeinforequest2_cmd.buffer,
				 response, sizeof(response));
	if (result != TPM_SUCCESS) {
		return result;
	}

	const uint8_t* cursor = response + kTpmResponseHeaderLength;
	uint16_t size = ReadTpmUint16(&cursor);

	/* Comments below indicate skipped fields of unknown purpose that are
	 * marked "internal" in the firmware updater source. */
	cursor += sizeof(uint16_t);  /* internal1 */
	info->wMaxDataSize = ReadTpmUint16(&cursor);
	cursor += sizeof(uint16_t);  /* sSecurityModuleLogic.internal1 */
	cursor += sizeof(uint32_t);  /* sSecurityModuleLogic.internal2 */
	cursor += sizeof(uint8_t[34]);  /* sSecurityModuleLogic.internal3 */
	ParseIFXFirmwarePackage(&cursor, &info->sBootloaderFirmwarePackage);
	uint16_t fw_entry_count = ReadTpmUint16(&cursor);
	if (fw_entry_count > ARRAY_SIZE(info->sFirmwarePackages)) {
		return TPM_E_INVALID_RESPONSE;
	}
	uint16_t i;
	for (i = 0; i < fw_entry_count; ++i) {
		ParseIFXFirmwarePackage(&cursor, &info->sFirmwarePackages[i]);
	}
	info->wSecurityModuleStatus = ReadTpmUint16(&cursor);
	ParseIFXFirmwarePackage(&cursor, &info->sProcessFirmwarePackage);
	cursor += sizeof(uint16_t);  /* internal6 */
	cursor += sizeof(uint8_t[6]);  /* internal7 */
	info->wFieldUpgradeCounter = ReadTpmUint16(&cursor);

	uint32_t parsed_bytes = cursor - response;
	VbAssert(parsed_bytes <= TPM_LARGE_ENOUGH_COMMAND_SIZE);
	if (parsed_bytes > kTpmResponseHeaderLength + sizeof(size) + size) {
		return TPM_E_INVALID_RESPONSE;
	}

	return result;
}

#ifdef CHROMEOS_ENVIRONMENT

static uint32_t ParseRsaKeyParms(const uint8_t* buffer,
				 const uint8_t* end,
				 uint32_t* key_len,
				 uint32_t* num_primes,
				 uint32_t* exponent)
{
	if (end - buffer < 3 * sizeof(uint32_t)) {
		return TPM_E_INVALID_RESPONSE;
	}
	*key_len = ReadTpmUint32(&buffer);
	*num_primes = ReadTpmUint32(&buffer);
	uint32_t exponent_size = ReadTpmUint32(&buffer);
	if (end - buffer < exponent_size) {
		return TPM_E_INVALID_RESPONSE;
	}

	if (exponent_size == 0) {
		*exponent = 0x10001;
	} else if (exponent_size <= sizeof(*exponent)) {
		*exponent = 0;
		int i;
		for (i = 0; i < exponent_size; ++i) {
			*exponent |= (*buffer++) << (8 * i);
		}
	} else {
		return TPM_E_INTERNAL_ERROR;
	}

	return TPM_SUCCESS;
}

static uint32_t ParseTpmPubKey(const uint8_t** buffer,
			       const uint8_t* end,
			       uint32_t* algorithm,
			       uint16_t* enc_scheme,
			       uint16_t* sig_scheme,
			       uint32_t* key_len,
			       uint32_t* num_primes,
			       uint32_t* exponent,
			       uint8_t* modulus,
			       uint32_t* modulus_size)
{
	uint32_t result = TPM_SUCCESS;

	if (end - *buffer < 2 * sizeof(uint32_t) + 2 * sizeof(uint16_t)) {
		return TPM_E_INVALID_RESPONSE;
	}

	*algorithm = ReadTpmUint32(buffer);
	*enc_scheme = ReadTpmUint16(buffer);
	*sig_scheme = ReadTpmUint16(buffer);

	uint32_t parm_size = ReadTpmUint32(buffer);
	if (end - *buffer < parm_size) {
		return TPM_E_INVALID_RESPONSE;
	}

	if (*algorithm == TPM_ALG_RSA) {
		result = ParseRsaKeyParms(*buffer, *buffer + parm_size,
					  key_len, num_primes, exponent);
		if (result != TPM_SUCCESS) {
			return result;
		}
	} else {
		return TPM_E_INTERNAL_ERROR;
	}

	*buffer += parm_size;

	if (end - *buffer < sizeof(uint32_t)) {
		return TPM_E_INVALID_RESPONSE;
	}

	uint32_t actual_modulus_size = ReadTpmUint32(buffer);
	if (end - *buffer < actual_modulus_size) {
		return TPM_E_INVALID_RESPONSE;
	}

	if (modulus && *modulus_size >= actual_modulus_size) {
		memcpy(modulus, *buffer, actual_modulus_size);
	} else {
		result = TPM_E_BUFFER_SIZE;
	}
	*modulus_size = actual_modulus_size;
	*buffer += actual_modulus_size;

	return result;
}

uint32_t TlclReadPubek(uint32_t* public_exponent,
		       uint8_t* modulus,
		       uint32_t* modulus_size)
{
	struct s_tpm_readpubek_cmd cmd;
	memcpy(&cmd, &tpm_readpubek_cmd, sizeof(cmd));
	if (VbExTpmGetRandom(cmd.buffer + tpm_readpubek_cmd.antiReplay,
			     sizeof(TPM_NONCE)) != VB2_SUCCESS) {
		return TPM_E_INTERNAL_ERROR;
	}

	/* The response contains the public endorsement key, so use a large
	 * response buffer. */
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE + TPM_RSA_2048_LEN];
	uint32_t result = TlclSendReceive(cmd.buffer, response,
					  sizeof(response));
	if (result != TPM_SUCCESS) {
		return result;
	}

	const uint8_t* cursor = response + kTpmResponseHeaderLength;
	const uint8_t* end = response + sizeof(response);

	/* Parse the key */
	uint32_t algorithm;
	uint16_t enc_scheme;
	uint16_t sig_scheme;
	uint32_t key_len;
	uint32_t num_primes;
	result = ParseTpmPubKey(&cursor, end, &algorithm, &enc_scheme,
				&sig_scheme, &key_len, &num_primes,
				public_exponent, modulus, modulus_size);
	if (result != TPM_SUCCESS) {
		return result;
	}

	/* Parse the checksum */
	if (end - cursor < TPM_SHA1_160_HASH_LEN) {
		return TPM_E_INVALID_RESPONSE;
	}
	const uint8_t* checksum = cursor;
	cursor += TPM_SHA1_160_HASH_LEN;

	/* Check the digest */
	struct vb2_sha1_context sha1_ctx;
	vb2_sha1_init(&sha1_ctx);
	vb2_sha1_update(&sha1_ctx, response + kTpmResponseHeaderLength,
			checksum - (response + kTpmResponseHeaderLength));
	vb2_sha1_update(&sha1_ctx, cmd.buffer + tpm_readpubek_cmd.antiReplay,
			sizeof(TPM_NONCE));
	uint8_t digest[TPM_SHA1_160_HASH_LEN];
	vb2_sha1_finalize(&sha1_ctx, digest);
	if (vb2_safe_memcmp(digest, checksum, sizeof(digest))) {
		return TPM_E_AUTHFAIL;
	}

	/* Validate expectations for the EK. */
	if (algorithm != TPM_ALG_RSA ||
	    enc_scheme != TPM_ES_RSAESOAEP_SHA1_MGF1 ||
	    sig_scheme != TPM_SS_NONE ||
	    key_len != 2048 ||
	    num_primes != 2) {
		return TPM_E_INVALID_RESPONSE;
	}

	return result;
}

uint32_t TlclTakeOwnership(uint8_t enc_owner_auth[TPM_RSA_2048_LEN],
			   uint8_t enc_srk_auth[TPM_RSA_2048_LEN],
			   uint8_t owner_auth[TPM_AUTH_DATA_LEN])
{
	/* Start an OAIP session. */
	struct auth_session auth_session;
	uint32_t result = StartOIAPSession(&auth_session, owner_auth);
	if (result != TPM_SUCCESS) {
		return result;
	}

	/* Build the TakeOwnership command. */
	struct s_tpm_takeownership_cmd cmd;
	memcpy(&cmd, &tpm_takeownership_cmd, sizeof(cmd));
	memcpy(cmd.buffer + tpm_takeownership_cmd.encOwnerAuth, enc_owner_auth,
	       TPM_RSA_2048_LEN);
	memcpy(cmd.buffer + tpm_takeownership_cmd.encSrkAuth, enc_srk_auth,
	       TPM_RSA_2048_LEN);
	result = AddRequestAuthBlock(&auth_session, cmd.buffer,
				     sizeof(cmd.buffer), 0);
	if (result != TPM_SUCCESS) {
		return result;
	}

	/* The response buffer needs to be large to hold the public half of the
	 * generated SRK. */
	uint8_t response[TPM_LARGE_ENOUGH_COMMAND_SIZE + TPM_RSA_2048_LEN];
	result = TlclSendReceive(cmd.buffer, response, sizeof(response));
	if (result != TPM_SUCCESS) {
		return result;
	}

	/* Check the auth tag on the response. */
	result = CheckResponseAuthBlock(&auth_session, TPM_ORD_TakeOwnership,
					response, sizeof(response));
	if (result != TPM_SUCCESS) {
		return result;
	}

	return TPM_SUCCESS;
}

#endif  /* CHROMEOS_ENVIRONMENT */
