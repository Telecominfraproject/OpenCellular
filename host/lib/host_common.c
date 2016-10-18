/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */

#include <string.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "host_common.h"
#include "host_key2.h"
#include "utility.h"
#include "vb2_common.h"
#include "vboot_common.h"

struct vb2_fw_preamble *vb2_create_fw_preamble(
	uint32_t firmware_version,
	const struct vb2_packed_key *kernel_subkey,
	const struct vb2_signature *body_signature,
	const struct vb2_private_key *signing_key,
	uint32_t flags)
{
	uint32_t signed_size = (sizeof(struct vb2_fw_preamble) +
				kernel_subkey->key_size +
				body_signature->sig_size);
	uint32_t block_size = signed_size +
		vb2_rsa_sig_size(signing_key->sig_alg);

	/* Allocate key block */
	struct vb2_fw_preamble *h =
		(struct vb2_fw_preamble *)calloc(block_size, 1);
	if (!h)
		return NULL;

	uint8_t *kernel_subkey_dest = (uint8_t *)(h + 1);
	uint8_t *body_sig_dest = kernel_subkey_dest + kernel_subkey->key_size;
	uint8_t *block_sig_dest = body_sig_dest + body_signature->sig_size;

	h->header_version_major = FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR;
	h->header_version_minor = FIRMWARE_PREAMBLE_HEADER_VERSION_MINOR;
	h->preamble_size = block_size;
	h->firmware_version = firmware_version;
	h->flags = flags;

	/* Copy data key */
	vb2_init_packed_key(&h->kernel_subkey, kernel_subkey_dest,
			    kernel_subkey->key_size);
	if (VB2_SUCCESS !=
	    vb2_copy_packed_key(&h->kernel_subkey, kernel_subkey)) {
		free(h);
		return NULL;
	}

	/* Copy body signature */
	vb2_init_signature(&h->body_signature,
			   body_sig_dest, body_signature->sig_size, 0);
	if (VB2_SUCCESS !=
	    vb2_copy_signature(&h->body_signature, body_signature)) {
		free(h);
		return NULL;
	}

	/* Set up signature struct so we can calculate the signature */
	vb2_init_signature(&h->preamble_signature, block_sig_dest,
			   vb2_rsa_sig_size(signing_key->sig_alg), signed_size);

	/* Calculate signature */
	struct vb2_signature *sig =
		vb2_calculate_signature((uint8_t *)h, signed_size, signing_key);
	vb2_copy_signature(&h->preamble_signature, sig);
	free(sig);

	/* Return the header */
	return h;
}

struct vb2_kernel_preamble *vb2_create_kernel_preamble(
	uint32_t kernel_version,
	uint64_t body_load_address,
	uint64_t bootloader_address,
	uint32_t bootloader_size,
	const struct vb2_signature *body_signature,
	uint64_t vmlinuz_header_address,
	uint32_t vmlinuz_header_size,
	uint32_t flags,
	uint32_t desired_size,
	const struct vb2_private_key *signing_key)
{
	uint64_t signed_size = (sizeof(struct vb2_kernel_preamble) +
				body_signature->sig_size);
	uint32_t sig_size = vb2_rsa_sig_size(signing_key->sig_alg);
	uint32_t block_size = signed_size + sig_size;

	/* If the block size is smaller than the desired size, pad it */
	if (block_size < desired_size)
		block_size = desired_size;

	/* Allocate key block */
	struct vb2_kernel_preamble *h =
		(struct vb2_kernel_preamble *)calloc(block_size, 1);
	if (!h)
		return NULL;

	uint8_t *body_sig_dest = (uint8_t *)(h + 1);
	uint8_t *block_sig_dest = body_sig_dest + body_signature->sig_size;

	h->header_version_major = KERNEL_PREAMBLE_HEADER_VERSION_MAJOR;
	h->header_version_minor = KERNEL_PREAMBLE_HEADER_VERSION_MINOR;
	h->preamble_size = block_size;
	h->kernel_version = kernel_version;
	h->body_load_address = body_load_address;
	h->bootloader_address = bootloader_address;
	h->bootloader_size = bootloader_size;
	h->vmlinuz_header_address = vmlinuz_header_address;
	h->vmlinuz_header_size = vmlinuz_header_size;
	h->flags = flags;

	/* Copy body signature */
	vb2_init_signature(&h->body_signature, body_sig_dest,
			   body_signature->sig_size, 0);
	vb2_copy_signature(&h->body_signature, body_signature);

	/* Set up signature struct so we can calculate the signature */
	vb2_init_signature(&h->preamble_signature, block_sig_dest,
			   sig_size, signed_size);

	/* Calculate signature */
	struct vb2_signature *sigtmp =
		vb2_calculate_signature((uint8_t *)h, signed_size, signing_key);
	vb2_copy_signature(&h->preamble_signature, sigtmp);
	free(sigtmp);

	/* Return the header */
	return h;
}
