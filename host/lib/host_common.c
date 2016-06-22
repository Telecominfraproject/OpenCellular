/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 *
 * TODO: change all 'return 0', 'return 1' into meaningful return codes.
 */

#include <string.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "host_common.h"
#include "host_key2.h"
#include "cryptolib.h"
#include "utility.h"
#include "vboot_common.h"

VbKernelPreambleHeader *CreateKernelPreamble(
	uint64_t kernel_version,
	uint64_t body_load_address,
	uint64_t bootloader_address,
	uint64_t bootloader_size,
	const VbSignature *body_signature,
	uint64_t vmlinuz_header_address,
	uint64_t vmlinuz_header_size,
	uint32_t flags,
	uint64_t desired_size,
	const struct vb2_private_key *signing_key)
{
	VbKernelPreambleHeader *h;
	uint64_t signed_size = (sizeof(VbKernelPreambleHeader) +
				body_signature->sig_size);
	uint32_t sig_size = vb2_rsa_sig_size(signing_key->sig_alg);
	uint64_t block_size = signed_size + sig_size;
	uint8_t *body_sig_dest;
	uint8_t *block_sig_dest;

	/* If the block size is smaller than the desired size, pad it */
	if (block_size < desired_size)
		block_size = desired_size;

	/* Allocate key block */
	h = (VbKernelPreambleHeader *)calloc(block_size, 1);
	if (!h)
		return NULL;

	body_sig_dest = (uint8_t *)(h + 1);
	block_sig_dest = body_sig_dest + body_signature->sig_size;

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
	SignatureInit(&h->body_signature, body_sig_dest,
		      body_signature->sig_size, 0);
	SignatureCopy(&h->body_signature, body_signature);

	/* Set up signature struct so we can calculate the signature */
	SignatureInit(&h->preamble_signature, block_sig_dest,
		      sig_size, signed_size);

	/* Calculate signature */
	struct vb2_signature *sigtmp =
		vb2_calculate_signature((uint8_t *)h, signed_size, signing_key);
	SignatureCopy(&h->preamble_signature, (VbSignature *)sigtmp);
	free(sigtmp);

	/* Return the header */
	return h;
}
