/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Externally-callable APIs
 * (Firmware portion)
 */

#include "2sysincludes.h"
#include "2api.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2secdata.h"
#include "2sha.h"
#include "2rsa.h"
#include "vb2_common.h"

int vb2api_fw_phase3(struct vb2_context *ctx)
{
	int rv;

	/* Verify firmware keyblock */
	rv = vb2_load_fw_keyblock(ctx);
	if (rv) {
		vb2_fail(ctx, VB2_RECOVERY_RO_INVALID_RW, rv);
		return rv;
	}

	/* Verify firmware preamble */
	rv = vb2_load_fw_preamble(ctx);
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

	if (!(pre->flags & VB2_FIRMWARE_PREAMBLE_DISALLOW_HWCRYPTO)) {
		rv = vb2ex_hwcrypto_digest_init(key.hash_alg,
						pre->body_signature.data_size);
		if (!rv) {
			VB2_DEBUG("Using HW crypto engine for hash_alg %d\n",
				  key.hash_alg);
			dc->hash_alg = key.hash_alg;
			dc->using_hwcrypto = 1;
			return VB2_SUCCESS;
		}
		if (rv != VB2_ERROR_EX_HWCRYPTO_UNSUPPORTED)
			return rv;
		VB2_DEBUG("HW crypto for hash_alg %d not supported, using SW\n",
			  key.hash_alg);
	} else {
		VB2_DEBUG("HW crypto forbidden by preamble, using SW\n");
	}

	return vb2_digest_init(dc, key.hash_alg);
}

int vb2api_check_hash(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_digest_context *dc = (struct vb2_digest_context *)
		(ctx->workbuf + sd->workbuf_hash_offset);
	struct vb2_workbuf wb;

	uint8_t *digest;
	uint32_t digest_size = vb2_digest_size(dc->hash_alg);

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
	if (dc->using_hwcrypto)
		rv = vb2ex_hwcrypto_digest_finalize(digest, digest_size);
	else
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

	/*
	 * Check digest vs. signature.  Note that this destroys the signature.
	 * That's ok, because we only check each signature once per boot.
	 */
	rv = vb2_verify_digest(&key, &pre->body_signature, digest, &wb);
	if (rv)
		vb2_fail(ctx, VB2_RECOVERY_FW_BODY, rv);

	return rv;
}
