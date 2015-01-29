/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Externally-callable APIs
 * (Firmware portion)
 */

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2secdata.h"
#include "2sha.h"
#include "2rsa.h"
#include "2tpm_bootmode.h"

int vb2api_secdata_check(const struct vb2_context *ctx)
{
	return vb2_secdata_check_crc(ctx);
}

int vb2api_secdata_create(struct vb2_context *ctx)
{
	return vb2_secdata_create(ctx);
}

void vb2api_fail(struct vb2_context *ctx, uint8_t reason, uint8_t subcode)
{
	/* Initialize the vboot context if it hasn't been yet */
	vb2_init_context(ctx);

	vb2_fail(ctx, reason, subcode);
}

int vb2api_fw_phase1(struct vb2_context *ctx)
{
	int rv;

	/* Initialize the vboot context if it hasn't been yet */
	vb2_init_context(ctx);

	/* Initialize NV context */
	vb2_nv_init(ctx);

	/* Initialize secure data */
	rv = vb2_secdata_init(ctx);
	if (rv)
		vb2_fail(ctx, VB2_RECOVERY_SECDATA_INIT, rv);

	/* Load and parse the GBB header */
	rv = vb2_fw_parse_gbb(ctx);
	if (rv)
		vb2_fail(ctx, VB2_RECOVERY_GBB_HEADER, rv);

	/* Check for dev switch */
	rv = vb2_check_dev_switch(ctx);
	if (rv)
		vb2_fail(ctx, VB2_RECOVERY_DEV_SWITCH, rv);

	/*
	 * Check for recovery.  Note that this function returns void, since
	 * any errors result in requesting recovery.
	 */
	vb2_check_recovery(ctx);

	/* Return error if recovery is needed */
	if (ctx->flags & VB2_CONTEXT_RECOVERY_MODE) {
		/* Always clear RAM when entering recovery mode */
		ctx->flags |= VB2_CONTEXT_CLEAR_RAM;

		return VB2_ERROR_API_PHASE1_RECOVERY;
	}

	return VB2_SUCCESS;
}

int vb2api_fw_phase2(struct vb2_context *ctx)
{
	int rv;

	/* Always clear RAM when entering developer mode */
	if (ctx->flags & VB2_CONTEXT_DEVELOPER_MODE)
		ctx->flags |= VB2_CONTEXT_CLEAR_RAM;

	/* Check for explicit request to clear TPM */
	rv = vb2_check_tpm_clear(ctx);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_TPM_CLEAR_OWNER, rv);
		return rv;
	}

	/* Decide which firmware slot to try this boot */
	rv = vb2_select_fw_slot(ctx);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_FW_SLOT, rv);
		return rv;
	}

	return VB2_SUCCESS;
}

int vb2api_extend_hash(struct vb2_context *ctx,
		       const void *buf,
		       uint32_t size)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_digest_context *dc = (struct vb2_digest_context *)
		(ctx->workbuf + sd->workbuf_hash_offset);

	/* Must have initialized hash digest work area */
	if (!sd->workbuf_hash_size)
		return VB2_ERROR_API_EXTEND_HASH_WORKBUF;

	/* Don't extend past the data we expect to hash */
	if (!size || size > sd->hash_remaining_size)
		return VB2_ERROR_API_EXTEND_HASH_SIZE;

	sd->hash_remaining_size -= size;

	if (dc->using_hwcrypto)
		return vb2ex_hwcrypto_digest_extend(buf, size);
	else
		return vb2_digest_extend(dc, buf, size);
}

int vb2api_get_pcr_digest(struct vb2_context *ctx,
			  enum vb2_pcr_digest which_digest,
			  uint8_t *dest,
			  uint32_t *dest_size)
{
	const uint8_t *digest;
	uint32_t digest_size;

	switch (which_digest) {
	case BOOT_MODE_PCR:
		digest = vb2_get_boot_state_digest(ctx);
		digest_size = VB2_SHA1_DIGEST_SIZE;
		break;
	case HWID_DIGEST_PCR:
		digest = vb2_get_sd(ctx)->gbb_hwid_digest;
		digest_size = VB2_GBB_HWID_DIGEST_SIZE;
		break;
	default:
		return VB2_ERROR_API_PCR_DIGEST;
	}

	if (digest == NULL || *dest_size < digest_size)
		return VB2_ERROR_API_PCR_DIGEST_BUF;

	memcpy(dest, digest, digest_size);
	*dest_size = digest_size;

	return VB2_SUCCESS;
}
