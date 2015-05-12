/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Kernel verified boot functions
 */

#include "2sysincludes.h"
#include "2rsa.h"
#include "2sha.h"
#include "vb2_common.h"

int vb2_verify_kernel_preamble(struct vb2_kernel_preamble *preamble,
			       uint32_t size,
			       const struct vb2_public_key *key,
			       const struct vb2_workbuf *wb)
{
	struct vb2_signature *sig = &preamble->preamble_signature;

	VB2_DEBUG("Verifying kernel preamble.\n");

	/* Sanity checks before attempting signature of data */
	if(size < sizeof(*preamble)) {
		VB2_DEBUG("Not enough data for preamble header.\n");
		return VB2_ERROR_PREAMBLE_TOO_SMALL_FOR_HEADER;
	}
	if (preamble->header_version_major !=
	    KERNEL_PREAMBLE_HEADER_VERSION_MAJOR) {
		VB2_DEBUG("Incompatible kernel preamble header version.\n");
		return VB2_ERROR_PREAMBLE_HEADER_VERSION;
	}
	if (preamble->header_version_minor < 2) {
		VB2_DEBUG("Old preamble header format not supported\n");
		return VB2_ERROR_PREAMBLE_HEADER_OLD;
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
	if (preamble->vmlinuz_header_size) {
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
