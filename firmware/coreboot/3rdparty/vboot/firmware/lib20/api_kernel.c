/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Externally-callable APIs
 * (Kernel portion)
 */

#include "2sysincludes.h"
#include "2api.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2secdata.h"
#include "2sha.h"
#include "2rsa.h"
#include "vb2_common.h"

int vb2api_kernel_phase1(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_workbuf wb;
	uint8_t *key_data;
	uint32_t key_size;
	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Initialize secure kernel data and read version */
	rv = vb2_secdatak_init(ctx);
	if (!rv) {
		rv = vb2_secdatak_get(ctx, VB2_SECDATAK_VERSIONS,
				      &sd->kernel_version_secdatak);
	}

	if (rv) {
		if (ctx->flags & VB2_CONTEXT_RECOVERY_MODE) {
			/* Ignore failure to get kernel version in recovery */
			sd->kernel_version_secdatak = 0;
		} else {
			vb2_fail(ctx, VB2_RECOVERY_SECDATAK_INIT, rv);
			return rv;
		}
	}

	/* Find the key to use to verify the kernel keyblock */
	if (ctx->flags & VB2_CONTEXT_RECOVERY_MODE) {
		/* Recovery key from GBB */
		struct vb2_gbb_header *gbb;
		uint32_t key_offset;

		/* Read GBB header into next chunk of work buffer */
		gbb = vb2_workbuf_alloc(&wb, sizeof(*gbb));
		if (!gbb)
			return VB2_ERROR_GBB_WORKBUF;

		rv = vb2_read_gbb_header(ctx, gbb);
		if (rv)
			return rv;

		/* Only need the recovery key position and size */
		key_offset = gbb->recovery_key_offset;
		key_size = gbb->recovery_key_size;

		/* Free the GBB header */
		vb2_workbuf_free(&wb, sizeof(*gbb));

		/* Load the recovery key itself */
		key_data = vb2_workbuf_alloc(&wb, key_size);
		if (!key_data)
			return VB2_ERROR_API_KPHASE1_WORKBUF_REC_KEY;

		rv = vb2ex_read_resource(ctx, VB2_RES_GBB, key_offset,
					 key_data, key_size);
		if (rv)
			return rv;

		sd->workbuf_kernel_key_offset =
				vb2_offset_of(ctx->workbuf, key_data);
	} else {
		/* Kernel subkey from firmware preamble */
		struct vb2_fw_preamble *pre;
		struct vb2_packed_key *pre_key, *packed_key;

		/* Make sure we have a firmware preamble loaded */
		if (!sd->workbuf_preamble_size)
			return VB2_ERROR_API_KPHASE1_PREAMBLE;

		pre = (struct vb2_fw_preamble *)
			(ctx->workbuf + sd->workbuf_preamble_offset);
		pre_key = &pre->kernel_subkey;

		/*
		 * At this point, we no longer need the packed firmware
		 * data key, firmware preamble, or hash data.  So move the
		 * kernel key from the preamble down after the shared data.
		 */
		sd->workbuf_kernel_key_offset = vb2_wb_round_up(sizeof(*sd));
		key_data = ctx->workbuf + sd->workbuf_kernel_key_offset;
		packed_key = (struct vb2_packed_key *)key_data;
		memmove(packed_key, pre_key, sizeof(*packed_key));
		packed_key->key_offset = sizeof(*packed_key);
		memmove(key_data + packed_key->key_offset,
			(uint8_t *)pre_key + pre_key->key_offset,
			pre_key->key_size);

		key_size = packed_key->key_offset + packed_key->key_size;
	}

	/* Firmware stage structs are no longer present */
	sd->workbuf_data_key_size = 0;
	sd->workbuf_preamble_size = 0;
	sd->workbuf_hash_size = 0;

	/*
	 * Kernel key will persist in the workbuf after we return.
	 *
	 * Work buffer now contains:
	 *   - vb2_shared_data
	 *   - kernel key
	 */
	sd->workbuf_kernel_key_size = key_size;
	vb2_set_workbuf_used(ctx, sd->workbuf_kernel_key_offset +
			     sd->workbuf_kernel_key_size);

	return VB2_SUCCESS;
}

int vb2api_load_kernel_vblock(struct vb2_context *ctx)
{
	int rv;

	/* Verify kernel keyblock */
	rv = vb2_load_kernel_keyblock(ctx);
	if (rv)
		return rv;

	/* Verify kernel preamble */
	rv = vb2_load_kernel_preamble(ctx);
	if (rv)
		return rv;

	return VB2_SUCCESS;
}

int vb2api_get_kernel_size(struct vb2_context *ctx,
			   uint32_t *offset_ptr,
			   uint32_t *size_ptr)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	const struct vb2_kernel_preamble *pre;

	/* Get preamble pointer */
	if (!sd->workbuf_preamble_size)
		return VB2_ERROR_API_GET_KERNEL_SIZE_PREAMBLE;

	pre = (const struct vb2_kernel_preamble *)
		(ctx->workbuf + sd->workbuf_preamble_offset);

	if (offset_ptr) {
		/* The kernel implicitly follows the preamble */
		*offset_ptr = sd->vblock_preamble_offset +
			sd->workbuf_preamble_size;
	}

	if (size_ptr) {
		/* Expect the kernel to be the size of data we signed */
		*size_ptr = pre->body_signature.data_size;
	}

	return VB2_SUCCESS;
}

int vb2api_verify_kernel_data(struct vb2_context *ctx,
			      const void *buf,
			      uint32_t size)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_kernel_preamble *pre;
	struct vb2_digest_context *dc;
	struct vb2_public_key key;
	struct vb2_workbuf wb;

	uint8_t *digest;
	uint32_t digest_size;

	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Get preamble pointer */
	if (!sd->workbuf_preamble_size)
		return VB2_ERROR_API_VERIFY_KDATA_PREAMBLE;

	pre = (struct vb2_kernel_preamble *)
		(ctx->workbuf + sd->workbuf_preamble_offset);

	/* Make sure we were passed the right amount of data */
	if (size != pre->body_signature.data_size)
		return VB2_ERROR_API_VERIFY_KDATA_SIZE;

	/* Allocate workbuf space for the hash */
	dc = vb2_workbuf_alloc(&wb, sizeof(*dc));
	if (!dc)
		return VB2_ERROR_API_VERIFY_KDATA_WORKBUF;

	/*
	 * Unpack the kernel data key to see which hashing algorithm we
	 * should use.
	 *
	 * TODO: really, the kernel body should be hashed, and not signed,
	 * because the signature we're checking is already signed as part of
	 * the kernel preamble.  But until we can change the signing scripts,
	 * we're stuck with a signature here instead of a hash.
	 */
	if (!sd->workbuf_data_key_size)
		return VB2_ERROR_API_VERIFY_KDATA_KEY;

	rv = vb2_unpack_key_buffer(&key,
			    ctx->workbuf + sd->workbuf_data_key_offset,
			    sd->workbuf_data_key_size);
	if (rv)
		return rv;

	rv = vb2_digest_init(dc, key.hash_alg);
	if (rv)
		return rv;

	rv = vb2_digest_extend(dc, buf, size);
	if (rv)
		return rv;

	digest_size = vb2_digest_size(key.hash_alg);
	digest = vb2_workbuf_alloc(&wb, digest_size);
	if (!digest)
		return VB2_ERROR_API_CHECK_HASH_WORKBUF_DIGEST;

	rv = vb2_digest_finalize(dc, digest, digest_size);
	if (rv)
		return rv;

	/*
	 * The body signature is currently a *signature* of the body data, not
	 * just its hash.  So we need to verify the signature.
	 */

	/*
	 * Check digest vs. signature.  Note that this destroys the signature.
	 * That's ok, because we only check each signature once per boot.
	 */
	return vb2_verify_digest(&key, &pre->body_signature, digest, &wb);
}

int vb2api_kernel_phase3(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	int rv;

	/*
	 * If the kernel is a newer version than in secure storage, and the
	 * kernel signature is valid, and we're not in recovery mode, and we're
	 * allowed to, roll forward the version in secure storage.
	 */
	if (sd->kernel_version > sd->kernel_version_secdatak &&
	    (sd->flags & VB2_SD_FLAG_KERNEL_SIGNED) &&
	    !(ctx->flags & VB2_CONTEXT_RECOVERY_MODE) &&
	    (ctx->flags & VB2_CONTEXT_ALLOW_KERNEL_ROLL_FORWARD)) {
		rv = vb2_secdatak_set(ctx, VB2_SECDATAK_VERSIONS,
				      sd->kernel_version);
		if (rv)
			return rv;
		sd->kernel_version_secdatak = sd->kernel_version;
	}

	return VB2_SUCCESS;
}
