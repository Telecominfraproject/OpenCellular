/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Some TPM constants and type definitions for standalone compilation for use
 * in the firmware
 */

#include "tpm2_marshaling.h"
#include "utility.h"

static void *tpm_process_command(TPM_CC command, void *command_body)
{
	/* Command/response buffer. */
	static uint8_t cr_buffer[TPM_BUFFER_SIZE];
	uint32_t out_size, in_size;

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
		VBDEBUG(("tpm transaction failed\n"));
		return NULL;
	}

	return tpm_unmarshal_response(command, cr_buffer, in_size);
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


/**
 * Get the permission bits for the NVRAM space with |index|.
 */
uint32_t TlclGetPermissions(uint32_t index, uint32_t *permissions)
{
	*permissions = 0;
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
}

/**
 * Turn off physical presence and locks it off until next reboot.  The TPM
 * error code is returned.
 */
uint32_t TlclLockPhysicalPresence(void)
{
	VBDEBUG(("%s called, NOT YET IMPLEMENTED\n", __func__));
	return TPM_SUCCESS;
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

	VBDEBUG(("%s:%d index %#x return code %x\n",
		 __FILE__, __LINE__, index, response->hdr.tpm_code));
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

	VBDEBUG(("%s:%d return code %x\n", __func__, __LINE__,
		 response->hdr.tpm_code));

	return TPM_SUCCESS;
}
