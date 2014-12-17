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

int vb2api_init_hash2(struct vb2_context *ctx,
		      const struct vb2_guid *guid,
		      uint32_t *size)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	const struct vb2_fw_preamble *pre;
	const struct vb2_signature *sig = NULL;
	struct vb2_digest_context *dc;
	struct vb2_workbuf wb;
	uint32_t hash_offset;
	int i, rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Get preamble pointer */
	if (!sd->workbuf_preamble_size)
		return VB2_ERROR_API_INIT_HASH_PREAMBLE;
	pre = (const struct vb2_fw_preamble *)
		(ctx->workbuf + sd->workbuf_preamble_offset);

	/* Find the matching signature */
	hash_offset = pre->hash_offset;
	for (i = 0; i < pre->hash_count; i++) {
		sig = (const struct vb2_signature *)
			((uint8_t *)pre + hash_offset);

		if (!memcmp(guid, &sig->guid, sizeof(*guid)))
			break;

		hash_offset += sig->c.total_size;
	}
	if (i >= pre->hash_count)
		return VB2_ERROR_API_INIT_HASH_GUID;  /* No match */

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

	sd->hash_tag = vb2_offset_of(ctx->workbuf, sig);
	sd->hash_remaining_size = sig->data_size;

	if (size)
		*size = sig->data_size;

	if (!(pre->flags & VB2_FIRMWARE_PREAMBLE_DISALLOW_HWCRYPTO)) {
		rv = vb2ex_hwcrypto_digest_init(sig->hash_alg, sig->data_size);
		if (!rv) {
			VB2_DEBUG("Using HW crypto engine for hash_alg %d\n",
				  sig->hash_alg);
			dc->hash_alg = sig->hash_alg;
			dc->using_hwcrypto = 1;
			return VB2_SUCCESS;
		}
		if (rv != VB2_ERROR_EX_HWCRYPTO_UNSUPPORTED)
			return rv;
		VB2_DEBUG("HW crypto for hash_alg %d not supported, using SW\n",
			  sig->hash_alg);
	} else {
		VB2_DEBUG("HW crypto forbidden by preamble, using SW\n");
	}

	return vb2_digest_init(dc, sig->hash_alg);
}

int vb2api_check_hash(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_digest_context *dc = (struct vb2_digest_context *)
		(ctx->workbuf + sd->workbuf_hash_offset);
	struct vb2_workbuf wb;

	uint8_t *digest;
	uint32_t digest_size = vb2_digest_size(dc->hash_alg);

	const struct vb2_signature *sig;

	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Get signature pointer */
	if (!sd->hash_tag)
		return VB2_ERROR_API_CHECK_HASH_TAG;
	sig = (const struct vb2_signature *)(ctx->workbuf + sd->hash_tag);

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

	/* Compare with the signature */
	if (vb2_safe_memcmp(digest, (const uint8_t *)sig + sig->sig_offset,
			    digest_size))
		return VB2_ERROR_API_CHECK_HASH_SIG;

	// TODO: the old check-hash function called vb2_fail() on any mismatch.
	// I don't think it should do that; the caller should.

	return VB2_SUCCESS;
}
