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
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	int rv;

	/* Initialize the vboot context if it hasn't been yet */
	vb2_init_context(ctx);

	/* Initialize NV context */
	vb2_nv_init(ctx);

	/* Initialize secure data */
	rv = vb2_secdata_init(ctx);
	if (rv)
		sd->recovery_reason = VB2_RECOVERY_SECDATA_INIT;

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

	/* Load and parse the GBB header */
	rv = vb2_fw_parse_gbb(ctx);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_GBB_HEADER, rv);
		return rv;
	}

	/* Check for dev switch */
	rv = vb2_check_dev_switch(ctx);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_DEV_SWITCH, rv);
		return rv;
	}

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

int vb2api_fw_phase3(struct vb2_context *ctx)
{
	int rv;

	/* Verify firmware keyblock */
	rv = vb2_verify_fw_keyblock(ctx);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_RO_INVALID_RW, rv);
		return rv;
	}

	/* Verify firmware preamble */
	rv = vb2_verify_fw_preamble2(ctx);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_RO_INVALID_RW, rv);
		return rv;
	}

	return VB2_SUCCESS;
}

int vb2api_init_hash(struct vb2_context *ctx, uint32_t tag, uint32_t *size)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	const struct vb2_fw_preamble *pre;
	struct vb2_digest_context *dc;
	struct vb2_public_key key;
	struct vb2_workbuf wb;
	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	if (tag == VB2_HASH_TAG_INVALID)
		return VB2_ERROR_API_INIT_HASH_TAG;

	/* Get preamble pointer */
	if (!sd->workbuf_preamble_size)
		return VB2_ERROR_API_INIT_HASH_PREAMBLE;
	pre = (const struct vb2_fw_preamble *)
		(ctx->workbuf + sd->workbuf_preamble_offset);

	/* For now, we only support the firmware body tag */
	if (tag != VB2_HASH_TAG_FW_BODY)
		return VB2_ERROR_API_INIT_HASH_TAG;

	/* Allocate workbuf space for the hash */
	if (sd->workbuf_hash_size) {
		dc = (struct vb2_digest_context *)
			(ctx->workbuf + sd->workbuf_hash_offset);
	} else {
		uint32_t dig_size = sizeof(*dc);

		dc = vb2_workbuf_alloc(&wb, dig_size);
		if (!dc)
			return VB2_ERROR_API_INIT_HASH_WORKBUF;

		sd->workbuf_hash_offset = vb2_offset_of(ctx->workbuf, dc);
		sd->workbuf_hash_size = dig_size;
		ctx->workbuf_used = sd->workbuf_hash_offset + dig_size;
	}

	/*
	 * Unpack the firmware data key to see which hashing algorithm we
	 * should use.
	 *
	 * TODO: really, the firmware body should be hashed, and not signed,
	 * because the signature we're checking is already signed as part of
	 * the firmware preamble.  But until we can change the signing scripts,
	 * we're stuck with a signature here instead of a hash.
	 */
	if (!sd->workbuf_data_key_size)
		return VB2_ERROR_API_INIT_HASH_DATA_KEY;

	rv = vb2_unpack_key(&key,
			    ctx->workbuf + sd->workbuf_data_key_offset,
			    sd->workbuf_data_key_size);
	if (rv)
		return rv;

	sd->hash_tag = tag;
	sd->hash_remaining_size = pre->body_signature.data_size;

	if (size)
		*size = pre->body_signature.data_size;

	return vb2_digest_init(dc, key.algorithm);
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

	return vb2_digest_extend(dc, buf, size);
}

int vb2api_check_hash(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_digest_context *dc = (struct vb2_digest_context *)
		(ctx->workbuf + sd->workbuf_hash_offset);
	struct vb2_workbuf wb;

	uint8_t *digest;
	uint32_t digest_size = vb2_digest_size(dc->algorithm);

	struct vb2_fw_preamble *pre;
	struct vb2_public_key key;
	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Get preamble pointer */
	if (!sd->workbuf_preamble_size)
		return VB2_ERROR_API_CHECK_HASH_PREAMBLE;
	pre = (struct vb2_fw_preamble *)
		(ctx->workbuf + sd->workbuf_preamble_offset);

	/* Must have initialized hash digest work area */
	if (!sd->workbuf_hash_size)
		return VB2_ERROR_API_CHECK_HASH_WORKBUF;

	/* Should have hashed the right amount of data */
	if (sd->hash_remaining_size)
		return VB2_ERROR_API_CHECK_HASH_SIZE;

	/* Allocate the digest */
	digest = vb2_workbuf_alloc(&wb, digest_size);
	if (!digest)
		return VB2_ERROR_API_CHECK_HASH_WORKBUF_DIGEST;

	/* Finalize the digest */
	rv = vb2_digest_finalize(dc, digest, digest_size);
	if (rv)
		return rv;

	/* The code below is specific to the body signature */
	if (sd->hash_tag != VB2_HASH_TAG_FW_BODY)
		return VB2_ERROR_API_CHECK_HASH_TAG;

	/*
	 * The body signature is currently a *signature* of the body data, not
	 * just its hash.  So we need to verify the signature.
	 */

	/* Unpack the data key */
	if (!sd->workbuf_data_key_size)
		return VB2_ERROR_API_CHECK_HASH_DATA_KEY;

	rv = vb2_unpack_key(&key,
			    ctx->workbuf + sd->workbuf_data_key_offset,
			    sd->workbuf_data_key_size);
	if (rv)
		return rv;

	/* Make sure body signature is the right size */
	if (pre->body_signature.sig_size != vb2_rsa_sig_size(key.algorithm)) {
		VB2_DEBUG("Wrong data signature size for algorithm, "
			  "sig_size=%d, expected %d for algorithm %d.\n",
			 (int)pre->body_signature.sig_size,
			  vb2_rsa_sig_size(key.algorithm),
			  key.algorithm);
		return VB2_ERROR_API_CHECK_HASH_SIG_SIZE;
	}

	/*
	 * Check digest vs. signature.  Note that this destroys the signature.
	 * That's ok, because we only check each signature once per boot.
	 */
	rv = vb2_verify_digest(&key,
			       vb2_signature_data(&pre->body_signature),
			       digest,
			       &wb);
	if (rv)
		vb2_fail(ctx, VB2_RECOVERY_RO_INVALID_RW, rv);

	return rv;
}
