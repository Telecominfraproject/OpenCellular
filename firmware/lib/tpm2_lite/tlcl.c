/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Some TPM constants and type definitions for standalone compilation for use
 * in the firmware
 */

#include "2sysincludes.h"
#include "2common.h"

#include "rollback_index.h"
#include "tpm2_marshaling.h"
#include "utility.h"
#include "tlcl.h"

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
		VB2_DEBUG("command %#x, cr size %d\n", command, out_size);
		return NULL;
	}

	in_size = sizeof(cr_buffer);
	if (VbExTpmSendReceive(cr_buffer, out_size,
			       cr_buffer, &in_size) != TPM_SUCCESS) {
		VB2_DEBUG("tpm transaction failed for %#x\n", command);
		return NULL;
	}

	response = tpm_unmarshal_response(command, cr_buffer, in_size);

	VB2_DEBUG("command %#x, return code %#x\n", command,
		  response ? response->hdr.tpm_code : -1);

	return response;
}

static uint32_t tlcl_read_ph_disabled(void)
{
	uint32_t rv;
	TPM_STCLEAR_FLAGS flags;

	rv = TlclGetSTClearFlags(&flags);
	if (rv != TPM_SUCCESS)
		return rv;

	tpm_set_ph_disabled(!flags.phEnable);

	return TPM_SUCCESS;
}

uint32_t TlclLibInit(void)
{
	uint32_t rv;

	rv = VbExTpmInit();
	if (rv != TPM_SUCCESS)
		return rv;

	rv = tlcl_read_ph_disabled();
	if (rv != TPM_SUCCESS) {
		TlclLibClose();
		return rv;
	}

	return TPM_SUCCESS;
}

uint32_t TlclLibClose(void)
{
	return VbExTpmClose();
}

uint32_t TlclSendReceive(const uint8_t *request, uint8_t *response,
                         int max_length)
{
	uint32_t rv, resp_size;

	resp_size = max_length;
	rv = VbExTpmSendReceive(request, tpm_get_packet_size(request),
				response, &resp_size);

	return rv ? rv : tpm_get_packet_response_code(response);
}

int TlclPacketSize(const uint8_t *packet)
{
	return tpm_get_packet_size(packet);
}

uint32_t TlclStartup(void)
{
	struct tpm2_response *response;
	struct tpm2_startup_cmd startup;

	startup.startup_type = TPM_SU_CLEAR;

	response = tpm_process_command(TPM2_Startup, &startup);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;

	return TPM_SUCCESS;
}

uint32_t TlclSaveState(void)
{
	struct tpm2_response *response;
	struct tpm2_shutdown_cmd shutdown;

	shutdown.shutdown_type = TPM_SU_STATE;

	response = tpm_process_command(TPM2_Shutdown, &shutdown);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;

	return TPM_SUCCESS;
}

uint32_t TlclResume(void)
{
	struct tpm2_response *response;
	struct tpm2_startup_cmd startup;

	startup.startup_type = TPM_SU_STATE;

	response = tpm_process_command(TPM2_Startup, &startup);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;

	return TPM_SUCCESS;
}

uint32_t TlclSelfTestFull(void)
{
	struct tpm2_response *response;
	struct tpm2_self_test_cmd self_test;

	self_test.full_test = 1;

	response = tpm_process_command(TPM2_SelfTest, &self_test);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;

	return TPM_SUCCESS;
}

uint32_t TlclContinueSelfTest(void)
{
	struct tpm2_response *response;
	struct tpm2_self_test_cmd self_test;

	self_test.full_test = 0;

	response = tpm_process_command(TPM2_SelfTest, &self_test);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;

	return TPM_SUCCESS;
}

uint32_t TlclDefineSpace(uint32_t index, uint32_t perm, uint32_t size)
{
	struct tpm2_response *response;
	struct tpm2_nv_define_space_cmd define_space;

	/* For backwards-compatibility, if no READ or WRITE permissions are set,
	 * assume readable/writeable with empty auth value.
	 */
	if (!(perm & TPMA_NV_MASK_WRITE))
		perm |= TPMA_NV_AUTHWRITE;
	if (!(perm & TPMA_NV_MASK_READ))
		perm |= TPMA_NV_AUTHREAD;

	memset(&define_space, 0, sizeof(define_space));
	define_space.publicInfo.nvIndex = HR_NV_INDEX + index;
	define_space.publicInfo.dataSize = size;
	define_space.publicInfo.attributes = perm;
	define_space.publicInfo.nameAlg = TPM_ALG_SHA256;

	response = tpm_process_command(TPM2_NV_DefineSpace, &define_space);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;

	return TPM_SUCCESS;
}

/**
 * Issue a ForceClear.  The TPM error code is returned.
 */
uint32_t TlclForceClear(void)
{
	struct tpm2_response *response;

	response = tpm_process_command(TPM2_Clear, NULL);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;

	return TPM_SUCCESS;
}

uint32_t TlclSetDeactivated(uint8_t flag)
{
	VB2_DEBUG("NOT YET IMPLEMENTED\n");
	return TPM_SUCCESS;
}

uint32_t TlclSetEnable(void)
{
	VB2_DEBUG("NOT YET IMPLEMENTED\n");
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
	VB2_DEBUG("NOT YET IMPLEMENTED\n");
	return 0;
}

uint32_t TlclExtend(int pcr_num, const uint8_t *in_digest, uint8_t *out_digest)
{
	VB2_DEBUG("NOT YET IMPLEMENTED\n");
	return TPM_SUCCESS;
}


static uint32_t tlcl_nv_read_public(uint32_t index,
				    struct nv_read_public_response **presp)
{
	struct tpm2_response *response;
	struct tpm2_nv_read_public_cmd read_pub;

	memset(&read_pub, 0, sizeof(read_pub));
	read_pub.nvIndex = HR_NV_INDEX + index;

	response = tpm_process_command(TPM2_NV_ReadPublic, &read_pub);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;
	*presp = &response->nv_read_public;

	return TPM_SUCCESS;
}

/**
 * Get the permission bits for the NVRAM space with |index|.
 */
uint32_t TlclGetPermissions(uint32_t index, uint32_t *permissions)
{
	uint32_t rv;
	struct nv_read_public_response *resp;

	rv = tlcl_nv_read_public(index, &resp);
	if (rv != TPM_SUCCESS)
		return rv;

	*permissions = resp->nvPublic.attributes;
	return TPM_SUCCESS;
}

static uint32_t tlcl_get_capability(TPM_CAP cap, TPM_PT property,
				    struct get_capability_response **presp)
{
	struct tpm2_response *response;
	struct tpm2_get_capability_cmd getcap;

	getcap.capability = cap;
	getcap.property = property;
	getcap.property_count = 1;

	response = tpm_process_command(TPM2_GetCapability, &getcap);
	if (!response || response->hdr.tpm_code)
		return TPM_E_IOERROR;
	*presp = &response->cap;

	return TPM_SUCCESS;
}

static uint32_t tlcl_get_tpm_property(TPM_PT property, uint32_t *pvalue)
{
	uint32_t rv;
	struct get_capability_response *resp;
	TPML_TAGGED_TPM_PROPERTY *tpm_prop;

	rv = tlcl_get_capability(TPM_CAP_TPM_PROPERTIES, property, &resp);
	if (rv != TPM_SUCCESS)
		return rv;

	if (resp->capability_data.capability != TPM_CAP_TPM_PROPERTIES)
		return TPM_E_IOERROR;

	tpm_prop = &resp->capability_data.data.tpm_properties;

	if ((tpm_prop->count != 1) ||
	    (tpm_prop->tpm_property[0].property != property))
		return TPM_E_IOERROR;

	*pvalue = tpm_prop->tpm_property[0].value;
	return TPM_SUCCESS;
}

uint32_t TlclGetPermanentFlags(TPM_PERMANENT_FLAGS *pflags)
{
	return tlcl_get_tpm_property(TPM_PT_PERMANENT,
				     (uint32_t *)pflags);
}

uint32_t TlclGetSTClearFlags(TPM_STCLEAR_FLAGS *pflags)
{
	return tlcl_get_tpm_property(TPM_PT_STARTUP_CLEAR,
				     (uint32_t *)pflags);
}

uint32_t TlclGetOwnership(uint8_t *owned)
{
	uint32_t rv;
	TPM_PERMANENT_FLAGS flags;
	*owned = 0;

	rv = TlclGetPermanentFlags(&flags);
	if (rv != TPM_SUCCESS)
		return rv;

	*owned = flags.ownerAuthSet;

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

	tpm_set_ph_disabled(1);
	return TPM_SUCCESS;
}

/**
 * The name of the function was kept to maintain the existing TPM API, but
 * TPM2.0 does not use the global lock to protect the FW rollback counter.
 * Instead it calls WriteLock for the FW NVRAM index to prevent future
 * writes to it.
 *
 * It first checks if the platform hierarchy is already disabled, and does
 * nothing, if so. Otherwise, WriteLock for the index obviously fails.
 */
uint32_t TlclSetGlobalLock(void)
{
	if (tpm_is_ph_disabled())
		return TPM_SUCCESS;
	else
		return tlcl_lock_nv_write(FIRMWARE_NV_INDEX);
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
	if (tpm_is_ph_disabled())
		return TPM_SUCCESS;

	return tlcl_disable_platform_hierarchy();
}

uint32_t TlclRead(uint32_t index, void* data, uint32_t length)
{
	struct tpm2_nv_read_cmd nv_readc;
	struct tpm2_response *response;

	memset(&nv_readc, 0, sizeof(nv_readc));

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

	memcpy(data, response->nvr.buffer.t.buffer, length);

	return TPM_SUCCESS;
}

uint32_t TlclWrite(uint32_t index, const void *data, uint32_t length)
{
	struct tpm2_nv_write_cmd nv_writec;
	struct tpm2_response *response;

	memset(&nv_writec, 0, sizeof(nv_writec));

	nv_writec.nvIndex = HR_NV_INDEX + index;
	nv_writec.data.t.size = length;
	nv_writec.data.t.buffer = data;

	response = tpm_process_command(TPM2_NV_Write, &nv_writec);

	/* Need to map tpm error codes into internal values. */
	if (!response || response->hdr.tpm_code)
		return TPM_E_WRITE_FAILURE;

	return TPM_SUCCESS;
}

uint32_t TlclPCRRead(uint32_t index, void *data, uint32_t length)
{
	VB2_DEBUG("NOT YET IMPLEMENTED\n");
	return TPM_SUCCESS;
}

uint32_t TlclWriteLock(uint32_t index)
{
	struct tpm2_nv_write_lock_cmd nv_writelockc;
	struct tpm2_response *response;

	memset(&nv_writelockc, 0, sizeof(nv_writelockc));

	nv_writelockc.nvIndex = HR_NV_INDEX | index;

	response = tpm_process_command(TPM2_NV_WriteLock, &nv_writelockc);

	/* Need to map tpm error codes into internal values. */
	if (!response || response->hdr.tpm_code)
		return TPM_E_WRITE_FAILURE;

	return TPM_SUCCESS;
}

uint32_t TlclReadLock(uint32_t index)
{
	struct tpm2_nv_read_lock_cmd nv_readlockc;
	struct tpm2_response *response;

	memset(&nv_readlockc, 0, sizeof(nv_readlockc));

	nv_readlockc.nvIndex = HR_NV_INDEX | index;

	response = tpm_process_command(TPM2_NV_ReadLock, &nv_readlockc);

	/* Need to map tpm error codes into internal values. */
	if (!response || response->hdr.tpm_code)
		return TPM_E_READ_FAILURE;

	return TPM_SUCCESS;
}

uint32_t TlclGetRandom(uint8_t *data, uint32_t length, uint32_t *size)
{
	*size = 0;
	VB2_DEBUG("NOT YET IMPLEMENTED\n");
	return TPM_E_IOERROR;
}

// Converts TPM_PT_VENDOR_STRING_x |value| to an array of bytes in |buf|.
// Returns the number of bytes in the array.
// |buf| should be at least 4 bytes long.
size_t tlcl_vendor_string_parse(uint32_t value, uint8_t* buf)
{
	size_t len = 0;
	int shift = 24;
	for (; len < 4; shift -= 8) {
		uint8_t byte = (value >> shift) & 0xffu;
		if (!byte)
			break;
		buf[len++] = byte;
	}
	return len;
}

uint32_t TlclGetVersion(uint32_t* vendor, uint64_t* firmware_version,
                        uint8_t* vendor_specific_buf,
                        size_t* vendor_specific_buf_size)
{
	uint32_t result =  tlcl_get_tpm_property(TPM_PT_MANUFACTURER, vendor);
	if (result != TPM_SUCCESS)
		return result;

	uint32_t version_1;
	uint32_t version_2;
	result = tlcl_get_tpm_property(TPM_PT_FIRMWARE_VERSION_1, &version_1);
	if (result != TPM_SUCCESS)
		return result;
	result = tlcl_get_tpm_property(TPM_PT_FIRMWARE_VERSION_2, &version_2);
	if (result != TPM_SUCCESS)
		return result;

	*firmware_version = ((uint64_t) version_1 << 32) | version_2;

	if (!vendor_specific_buf_size)
		return TPM_SUCCESS;

	size_t total_size = 0;
	uint32_t prop_id;
	uint8_t prop_string[16];
	for (prop_id = TPM_PT_VENDOR_STRING_1;
	     prop_id <= TPM_PT_VENDOR_STRING_4;
	     ++prop_id) {
		uint32_t prop_value;
		result = tlcl_get_tpm_property(prop_id, &prop_value);
		if (result != TPM_SUCCESS)
			break;

		size_t prop_len = tlcl_vendor_string_parse(
				prop_value, prop_string + total_size);
		VbAssert(prop_len <= 4 &&
			 total_size + prop_len <= sizeof(prop_string));
		total_size += prop_len;
		if (prop_len < 4)
			break;
	}
	if (vendor_specific_buf) {
		if (total_size > *vendor_specific_buf_size)
			total_size = *vendor_specific_buf_size;
		memcpy(vendor_specific_buf, prop_string, total_size);
	}
	*vendor_specific_buf_size = total_size;
	return TPM_SUCCESS;
}

uint32_t TlclIFXFieldUpgradeInfo(TPM_IFX_FIELDUPGRADEINFO* info)
{
	VB2_DEBUG("NOT YET IMPLEMENTED\n");
	return TPM_E_IOERROR;
}
