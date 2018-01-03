/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Kernel verified boot functions
 */

#include "2sysincludes.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "2secdata.h"
#include "2sha.h"
#include "vb2_common.h"

static const uint8_t *vb2_signature_data_const(const struct vb2_signature *sig)
{
	return (uint8_t *)sig + sig->sig_offset;
}

/**
 * Returns non-zero if the kernel needs to have a valid signature, instead of
 * just a valid hash.
 */
static int vb2_need_signed_kernel(struct vb2_context *ctx)
{
	/* Recovery kernels are always signed */
	if (ctx->flags & VB2_CONTEXT_RECOVERY_MODE)
		return 1;

	/* Normal mode kernels are always signed */
	if (!(ctx->flags & VB2_CONTEXT_DEVELOPER_MODE))
		return 1;

	/* Developers may require signed kernels */
	if (vb2_nv_get(ctx, VB2_NV_DEV_BOOT_SIGNED_ONLY))
		return 1;

	return 0;
}

int vb2_verify_keyblock_hash(const struct vb2_keyblock *block,
			     uint32_t size,
			     const struct vb2_workbuf *wb)
{
	const struct vb2_signature *sig = &block->keyblock_hash;
	struct vb2_workbuf wblocal = *wb;
	struct vb2_digest_context *dc;
	uint8_t *digest;
	uint32_t digest_size;
	int rv;

	/* Sanity check keyblock before attempting hash check of data */
	rv = vb2_check_keyblock(block, size, sig);
	if (rv)
		return rv;

	VB2_DEBUG("Checking key block hash...\n");

	/* Digest goes at start of work buffer */
	digest_size = vb2_digest_size(VB2_HASH_SHA512);
	digest = vb2_workbuf_alloc(&wblocal, digest_size);
	if (!digest)
		return VB2_ERROR_VDATA_WORKBUF_DIGEST;

	/* Hashing requires temp space for the context */
	dc = vb2_workbuf_alloc(&wblocal, sizeof(*dc));
	if (!dc)
		return VB2_ERROR_VDATA_WORKBUF_HASHING;

	rv = vb2_digest_init(dc, VB2_HASH_SHA512);
	if (rv)
		return rv;

	rv = vb2_digest_extend(dc, (const uint8_t *)block, sig->data_size);
	if (rv)
		return rv;

	rv = vb2_digest_finalize(dc, digest, digest_size);
	if (rv)
		return rv;

	if (vb2_safe_memcmp(vb2_signature_data_const(sig), digest,
			    digest_size) != 0) {
		VB2_DEBUG("Invalid key block hash.\n");
		return VB2_ERROR_KEYBLOCK_SIG_INVALID;
	}

	/* Success */
	return VB2_SUCCESS;
}

int vb2_load_kernel_keyblock(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_workbuf wb;

	uint8_t *key_data;
	uint32_t key_size;
	struct vb2_packed_key *packed_key;
	struct vb2_public_key kernel_key;

	struct vb2_keyblock *kb;
	uint32_t block_size;

	int rec_switch = (ctx->flags & VB2_CONTEXT_RECOVERY_MODE) != 0;
	int dev_switch = (ctx->flags & VB2_CONTEXT_DEVELOPER_MODE) != 0;
	int need_keyblock_valid = vb2_need_signed_kernel(ctx);
	int keyblock_is_valid = 1;

	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/*
	 * Clear any previous keyblock-valid flag (for example, from a previous
	 * kernel where the keyblock was signed but the preamble failed
	 * verification).
	 */
	sd->flags &= ~VB2_SD_FLAG_KERNEL_SIGNED;

	/* Unpack the kernel key */
	key_data = ctx->workbuf + sd->workbuf_kernel_key_offset;
	key_size = sd->workbuf_kernel_key_size;
	rv = vb2_unpack_key_buffer(&kernel_key, key_data, key_size);
	if (rv)
		return rv;

	/* Load the kernel keyblock header after the root key */
	kb = vb2_workbuf_alloc(&wb, sizeof(*kb));
	if (!kb)
		return VB2_ERROR_KERNEL_KEYBLOCK_WORKBUF_HEADER;

	rv = vb2ex_read_resource(ctx, VB2_RES_KERNEL_VBLOCK, 0, kb,
				 sizeof(*kb));
	if (rv)
		return rv;

	block_size = kb->keyblock_size;

	/*
	 * Load the entire keyblock, now that we know how big it is.  Note that
	 * we're loading the entire keyblock instead of just the piece after
	 * the header.  That means we re-read the header.  But that's a tiny
	 * amount of data, and it makes the code much more straightforward.
	 */
	kb = vb2_workbuf_realloc(&wb, sizeof(*kb), block_size);
	if (!kb)
		return VB2_ERROR_KERNEL_KEYBLOCK_WORKBUF;

	rv = vb2ex_read_resource(ctx, VB2_RES_KERNEL_VBLOCK, 0, kb, block_size);
	if (rv)
		return rv;

	/* Verify the keyblock */
	rv = vb2_verify_keyblock(kb, block_size, &kernel_key, &wb);
	if (rv) {
		keyblock_is_valid = 0;
		if (need_keyblock_valid)
			return rv;

		/* Signature is invalid, but hash may be fine */
		rv = vb2_verify_keyblock_hash(kb, block_size, &wb);
		if (rv)
			return rv;
	}

	/* Check the key block flags against the current boot mode */
	if (!(kb->keyblock_flags &
	      (dev_switch ? VB2_KEY_BLOCK_FLAG_DEVELOPER_1 :
	       VB2_KEY_BLOCK_FLAG_DEVELOPER_0))) {
		VB2_DEBUG("Key block developer flag mismatch.\n");
		keyblock_is_valid = 0;
		if (need_keyblock_valid)
			return VB2_ERROR_KERNEL_KEYBLOCK_DEV_FLAG;
	}
	if (!(kb->keyblock_flags &
	      (rec_switch ? VB2_KEY_BLOCK_FLAG_RECOVERY_1 :
	       VB2_KEY_BLOCK_FLAG_RECOVERY_0))) {
		VB2_DEBUG("Key block recovery flag mismatch.\n");
		keyblock_is_valid = 0;
		if (need_keyblock_valid)
			return VB2_ERROR_KERNEL_KEYBLOCK_REC_FLAG;
	}

	/* Check for keyblock rollback if not in recovery mode */
	/* Key version is the upper 16 bits of the composite version */
	if (!rec_switch && kb->data_key.key_version > VB2_MAX_KEY_VERSION) {
		keyblock_is_valid = 0;
		if (need_keyblock_valid)
			return VB2_ERROR_KERNEL_KEYBLOCK_VERSION_RANGE;
	}
	if (!rec_switch && kb->data_key.key_version <
	    (sd->kernel_version_secdatak >> 16)) {
		keyblock_is_valid = 0;
		if (need_keyblock_valid)
			return VB2_ERROR_KERNEL_KEYBLOCK_VERSION_ROLLBACK;
	}

	sd->kernel_version = kb->data_key.key_version << 16;

	/*
	 * At this point, we've checked everything.  The kernel keyblock is at
	 * least self-consistent, and has either a valid signature or a valid
	 * hash.  Track if it had a valid signature (that is, would we have
	 * been willing to boot it even if developer mode was off).
	 */
	if (keyblock_is_valid)
		sd->flags |= VB2_SD_FLAG_KERNEL_SIGNED;

	/* Preamble follows the keyblock in the vblock */
	sd->vblock_preamble_offset = kb->keyblock_size;

	/*
	 * Keep just the data key from the vblock.  This follows the kernel key
	 * (which we might still need to verify the next kernel, if the
	 * assoiciated kernel preamble and data don't verify).
	 */
	sd->workbuf_data_key_offset = ctx->workbuf_used;
	key_data = ctx->workbuf + sd->workbuf_data_key_offset;
	packed_key = (struct vb2_packed_key *)key_data;
	memmove(packed_key, &kb->data_key, sizeof(*packed_key));
	packed_key->key_offset = sizeof(*packed_key);
	memmove(key_data + packed_key->key_offset,
		(uint8_t*)&kb->data_key + kb->data_key.key_offset,
		packed_key->key_size);

	/* Save the packed key size */
	sd->workbuf_data_key_size =
		packed_key->key_offset + packed_key->key_size;

	/*
	 * Data key will persist in the workbuf after we return.
	 *
	 * Work buffer now contains:
	 *   - vb2_shared_data
	 *   - kernel key
	 *   - packed kernel data key
	 */
	vb2_set_workbuf_used(ctx, sd->workbuf_data_key_offset +
			     sd->workbuf_data_key_size);

	return VB2_SUCCESS;
}

int vb2_verify_kernel_preamble(struct vb2_kernel_preamble *preamble,
			       uint32_t size,
			       const struct vb2_public_key *key,
			       const struct vb2_workbuf *wb)
{
	struct vb2_signature *sig = &preamble->preamble_signature;
	uint32_t min_size = EXPECTED_VB2_KERNEL_PREAMBLE_2_0_SIZE;

	VB2_DEBUG("Verifying kernel preamble.\n");

	/* Make sure it's even safe to look at the struct */
	if(size < min_size) {
		VB2_DEBUG("Not enough data for preamble header.\n");
		return VB2_ERROR_PREAMBLE_TOO_SMALL_FOR_HEADER;
	}
	if (preamble->header_version_major !=
	    KERNEL_PREAMBLE_HEADER_VERSION_MAJOR) {
		VB2_DEBUG("Incompatible kernel preamble header version.\n");
		return VB2_ERROR_PREAMBLE_HEADER_VERSION;
	}

	if (preamble->header_version_minor >= 2)
		min_size = EXPECTED_VB2_KERNEL_PREAMBLE_2_2_SIZE;
	else if (preamble->header_version_minor == 1)
		min_size = EXPECTED_VB2_KERNEL_PREAMBLE_2_1_SIZE;
	if(preamble->preamble_size < min_size) {
		VB2_DEBUG("Preamble size too small for header.\n");
		return VB2_ERROR_PREAMBLE_TOO_SMALL_FOR_HEADER;
	}
	if (size < preamble->preamble_size) {
		VB2_DEBUG("Not enough data for preamble.\n");
		return VB2_ERROR_PREAMBLE_SIZE;
	}

	/* Check signature */
	if (vb2_verify_signature_inside(preamble, preamble->preamble_size,
					sig)) {
		VB2_DEBUG("Preamble signature off end of preamble\n");
		return VB2_ERROR_PREAMBLE_SIG_OUTSIDE;
	}

	/* Make sure advertised signature data sizes are sane. */
	if (preamble->preamble_size < sig->data_size) {
		VB2_DEBUG("Signature calculated past end of the block\n");
		return VB2_ERROR_PREAMBLE_SIGNED_TOO_MUCH;
	}

	if (vb2_verify_data((const uint8_t *)preamble, size, sig, key, wb)) {
		VB2_DEBUG("Preamble signature validation failed\n");
		return VB2_ERROR_PREAMBLE_SIG_INVALID;
	}

	/* Verify we signed enough data */
	if (sig->data_size < sizeof(struct vb2_fw_preamble)) {
		VB2_DEBUG("Didn't sign enough data\n");
		return VB2_ERROR_PREAMBLE_SIGNED_TOO_LITTLE;
	}

	/* Verify body signature is inside the signed data */
	if (vb2_verify_signature_inside(preamble, sig->data_size,
					&preamble->body_signature)) {
		VB2_DEBUG("Body signature off end of preamble\n");
		return VB2_ERROR_PREAMBLE_BODY_SIG_OUTSIDE;
	}

	/*
	 * If bootloader is present, verify it's covered by the body
	 * signature.
	 */
	if (preamble->bootloader_size) {
		const void *body_ptr =
			(const void *)(uintptr_t)preamble->body_load_address;
		const void *bootloader_ptr =
			(const void *)(uintptr_t)preamble->bootloader_address;
		if (vb2_verify_member_inside(body_ptr,
					     preamble->body_signature.data_size,
					     bootloader_ptr,
					     preamble->bootloader_size,
					     0, 0)) {
			VB2_DEBUG("Bootloader off end of signed data\n");
			return VB2_ERROR_PREAMBLE_BOOTLOADER_OUTSIDE;
		}
	}

	/*
	 * If vmlinuz header is present, verify it's covered by the body
	 * signature.
	 */
	if (preamble->header_version_minor >= 1 &&
	    preamble->vmlinuz_header_size) {
		const void *body_ptr =
			(const void *)(uintptr_t)preamble->body_load_address;
		const void *vmlinuz_header_ptr = (const void *)
			(uintptr_t)preamble->vmlinuz_header_address;
		if (vb2_verify_member_inside(body_ptr,
					     preamble->body_signature.data_size,
					     vmlinuz_header_ptr,
					     preamble->vmlinuz_header_size,
					     0, 0)) {
			VB2_DEBUG("Vmlinuz header off end of signed data\n");
			return VB2_ERROR_PREAMBLE_VMLINUZ_HEADER_OUTSIDE;
		}
	}

	/* Success */
	return VB2_SUCCESS;
}

int vb2_load_kernel_preamble(struct vb2_context *ctx)
{
	struct vb2_shared_data *sd = vb2_get_sd(ctx);
	struct vb2_workbuf wb;

	uint8_t *key_data = ctx->workbuf + sd->workbuf_data_key_offset;
	uint32_t key_size = sd->workbuf_data_key_size;
	struct vb2_public_key data_key;

	/* Preamble goes in the next unused chunk of work buffer */
	/* TODO: what's the minimum workbuf size?  Kernel preamble is usually
	 * padded to around 64KB. */
	struct vb2_kernel_preamble *pre;
	uint32_t pre_size;

	int rv;

	vb2_workbuf_from_ctx(ctx, &wb);

	/* Unpack the kernel data key */
	if (!sd->workbuf_data_key_size)
		return VB2_ERROR_KERNEL_PREAMBLE2_DATA_KEY;

	rv = vb2_unpack_key_buffer(&data_key, key_data, key_size);
	if (rv)
		return rv;

	/* Load the kernel preamble header */
	pre = vb2_workbuf_alloc(&wb, sizeof(*pre));
	if (!pre)
		return VB2_ERROR_KERNEL_PREAMBLE2_WORKBUF_HEADER;

	rv = vb2ex_read_resource(ctx, VB2_RES_KERNEL_VBLOCK,
				 sd->vblock_preamble_offset,
				 pre, sizeof(*pre));
	if (rv)
		return rv;

	pre_size = pre->preamble_size;

	/* Load the entire preamble, now that we know how big it is */
	pre = vb2_workbuf_realloc(&wb, sizeof(*pre), pre_size);
	if (!pre)
		return VB2_ERROR_KERNEL_PREAMBLE2_WORKBUF;

	rv = vb2ex_read_resource(ctx, VB2_RES_KERNEL_VBLOCK,
				 sd->vblock_preamble_offset,
				 pre, pre_size);
	if (rv)
		return rv;

	/*
	 * Work buffer now contains:
	 *   - vb2_shared_data
	 *   - kernel key
	 *   - packed kernel data key
	 *   - kernel preamble
	 */

	/* Verify the preamble */
	rv = vb2_verify_kernel_preamble(pre, pre_size, &data_key, &wb);
	if (rv)
		return rv;

	/*
	 * Kernel preamble version is the lower 16 bits of the composite kernel
	 * version.
	 */
	if (pre->kernel_version > VB2_MAX_PREAMBLE_VERSION)
		return VB2_ERROR_KERNEL_PREAMBLE_VERSION_RANGE;

	/* Combine with the key version from vb2_load_kernel_keyblock() */
	sd->kernel_version |= pre->kernel_version;

	if (vb2_need_signed_kernel(ctx) &&
	    sd->kernel_version < sd->kernel_version_secdatak)
		return VB2_ERROR_KERNEL_PREAMBLE_VERSION_ROLLBACK;

	/* Keep track of where we put the preamble */
	sd->workbuf_preamble_offset = vb2_offset_of(ctx->workbuf, pre);
	sd->workbuf_preamble_size = pre_size;

	/*
	 * Preamble will persist in work buffer after we return.
	 *
	 * Work buffer now contains:
	 *   - vb2_shared_data
	 *   - kernel key
	 *   - packed kernel data key
	 *   - kernel preamble
	 *
	 * TODO: we could move the preamble down over the kernel data key
	 * since we don't need it anymore.
	 */
	vb2_set_workbuf_used(ctx, sd->workbuf_preamble_offset + pre_size);

	return VB2_SUCCESS;
}

void vb2_kernel_get_vmlinuz_header(const struct vb2_kernel_preamble *preamble,
				   uint64_t *vmlinuz_header_address,
				   uint32_t *vmlinuz_header_size)
{
	if (preamble->header_version_minor < 1) {
		*vmlinuz_header_address = 0;
		*vmlinuz_header_size = 0;
	} else {
		/*
		 * Set header and size only if the preamble header version is >
		 * 2.1 as they don't exist in version 2.0 (Note that we don't
		 * need to check header_version_major; if that's not 2 then
		 * vb2_verify_kernel_preamble() would have already failed.
		 */
		*vmlinuz_header_address = preamble->vmlinuz_header_address;
		*vmlinuz_header_size = preamble->vmlinuz_header_size;
	}
}

uint32_t vb2_kernel_get_flags(const struct vb2_kernel_preamble *preamble)
{
	if (preamble->header_version_minor < 2)
		return 0;

	return preamble->flags;
}
