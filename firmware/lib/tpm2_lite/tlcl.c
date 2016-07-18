/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Some TPM constants and type definitions for standalone compilation for use
 * in the firmware
 */

#include "rollback_index.h"
#include "tpm2_marshaling.h"
#include "utility.h"

static struct tpm2_response *tpm_process_command(TPM_CC command,
						 void *command_body)
{
	/* Command/response buffer. */
	static uint8_t cr_buffer[TPM_BUFFER_SIZE];
	uint32_t out_size, in_size;
	struct tpm2_response *response;

	out_size = tpm_marshal_command(command, command_body,
				       cr_buffer, sizeof(cr_buffer));
	if (out_size < 0) {
		VBDEBUG(("command %#x, cr size %d\n",
			 command, out_size));
		return NULL;
	}

	in_size = sizeof(cr_buffer);
	if (VbExTpmSendReceive(cr_buffer, out_size,
			       cr_buffer, &in_size) != TPM_SUCCESS) {
		VBDEBUG(("tpm transaction failed for %#x\n", command));
		return NULL;
	}

	response = tpm_unmarshal_response(command, cr_buffer, in_size);

	VBDEBUG(("%s: command %#x, return code %#x\n", __func__, command,
		 response ? response->hdr.tpm_code : -1));

	return response;
}

uint32_t TlclLibInit(void)
{
	return VbExTpmInit();
}

uint32_t TlclLibClose(void)
{
	return VbExTpmClose();
}

void TlclLibAccessAsUser(void)
{
	tpm_set_ph_disabled(1);
}

uint32_t TlclSendReceive(const uint8_t *request, uint8_t *response,
                         int max_length)
{
        VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
        return TPM_SUCCESS;
}

int TlclPacketSize(const uint8_t *packet)
{
        VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
        return 0;
}

uint32_t TlclStartup(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclSaveState(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclResume(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclSelfTestFull(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclContinueSelfTest(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

int32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

/**
 * Issue a ForceClear.  The TPM error code is returned.
 */
uint32_t TlclForceClear(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclSetDeactivated(uint8_t flag)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclSetEnable(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclGetFlags(uint8_t* disable,
                      uint8_t* deactivated,
                      uint8_t *nvlocked)
{
	/* For TPM2 the flags are always the same */
	if (disable)
		*disable = 0;
	if (deactivated)
		*deactivated = 0;
	if (nvlocked)
		*nvlocked = 1;
	return TPM_SUCCESS;
}

int TlclIsOwned(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return 0;
}

uint32_t TlclExtend(int pcr_num, const uint8_t *in_digest, uint8_t *out_digest)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

/**
 * Get the permission bits for the NVRAM space with |index|.
 */
uint32_t TlclGetPermissions(uint32_t index, uint32_t *permissions)
{
	*permissions = 0;
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS *pflags)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclGetSTClearFlags(TPM_STCLEAR_FLAGS *pflags)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclGetOwnership(uint8_t *owned)
{
	*owned = 0;
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

static uint32_t tlcl_lock_nv_write(uint32_t index)
{
	struct tpm2_response *response;
	struct tpm2_nv_write_lock_cmd nv_wl;

	nv_wl.nvIndex = HR_NV_INDEX + index;
	response = tpm_process_command(TPM2_NV_WriteLock, &nv_wl);

	if (!response || response->hdr.tpm_code)
		return TPM_E_INTERNAL_INCONSISTENCY;

	return TPM_SUCCESS;
}

static uint32_t tlcl_disable_platform_hierarchy(void)
{
	struct tpm2_response *response;
	struct tpm2_hierarchy_control_cmd hc;

	hc.enable = TPM_RH_PLATFORM;
	hc.state = 0;

	response = tpm_process_command(TPM2_Hierarchy_Control, &hc);

	if (!response || response->hdr.tpm_code)
		return TPM_E_INTERNAL_INCONSISTENCY;

	return TPM_SUCCESS;
}

/**
 * Turn off physical presence and locks it off until next reboot.  The TPM
 * error code is returned.
 *
 * The name of the function was kept to maintain the existing TPM API, but
 * TPM2.0 does not have to use the Physical Presence concept. Instead it just
 * removes platform authorization - this makes sure that firmware and kernel
 * rollback counter spaces can not be modified.
 *
 * It also explicitly locks the kernel rollback counter space (the FW rollback
 * counter space was locked before RW firmware started.)
 */
uint32_t TlclLockPhysicalPresence(void)
{
	uint32_t rv;

	rv = tlcl_lock_nv_write(KERNEL_NV_INDEX);
	if (rv == TPM_SUCCESS)
		rv = tlcl_disable_platform_hierarchy();

	return rv;
}

uint32_t TlclRead(uint32_t index, void* data, uint32_t length)
{
	struct tpm2_nv_read_cmd nv_readc;
	struct tpm2_response *response;

	Memset(&nv_readc, 0, sizeof(nv_readc));

	nv_readc.nvIndex = HR_NV_INDEX + index;
	nv_readc.size = length;

	response = tpm_process_command(TPM2_NV_Read, &nv_readc);

	/* Need to map tpm error codes into internal values. */
	if (!response)
		return TPM_E_READ_FAILURE;

	switch (response->hdr.tpm_code) {
	case 0:
		break;

	case 0x28b:
		return TPM_E_BADINDEX;

	default:
		return TPM_E_READ_FAILURE;
	}

	if (length > response->nvr.buffer.t.size)
		return TPM_E_RESPONSE_TOO_LARGE;

	if (length < response->nvr.buffer.t.size)
		return TPM_E_READ_EMPTY;

	Memcpy(data, response->nvr.buffer.t.buffer, length);

	return TPM_SUCCESS;
}

uint32_t TlclWrite(uint32_t index, const void *data, uint32_t length)
{
	struct tpm2_nv_write_cmd nv_writec;
	struct tpm2_response *response;

	Memset(&nv_writec, 0, sizeof(nv_writec));

	nv_writec.nvIndex = HR_NV_INDEX + index;
	nv_writec.data.t.size = length;
	nv_writec.data.t.buffer = data;

	response = tpm_process_command(TPM2_NV_Write, &nv_writec);

	/* Need to map tpm error codes into internal values. */
	if (!response)
		return TPM_E_WRITE_FAILURE;

	return TPM_SUCCESS;
}

int32_t TlclPCRRead(uint32_t index, void *data, uint32_t length)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclWriteLock(uint32_t index)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclReadLock(uint32_t index)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

uint32_t TlclGetRandom(uint8_t *data, uint32_t length, uint32_t *size)
{
	*size = 0;
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_E_IOERROR;
}
