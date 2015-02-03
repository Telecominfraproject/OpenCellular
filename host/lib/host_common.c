/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 *
 * TODO: change all 'return 0', 'return 1' into meaningful return codes.
 */

#include <string.h>

#include "host_common.h"
#include "cryptolib.h"
#include "utility.h"
#include "vboot_common.h"

VbFirmwarePreambleHeader *CreateFirmwarePreamble(
	uint64_t firmware_version,
	const VbPublicKey *kernel_subkey,
	const VbSignature *body_signature,
	const VbPrivateKey *signing_key,
	uint32_t flags)
{
	VbFirmwarePreambleHeader *h;
	uint64_t signed_size = (sizeof(VbFirmwarePreambleHeader) +
				kernel_subkey->key_size +
				body_signature->sig_size);
	uint64_t block_size = signed_size + siglen_map[signing_key->algorithm];
	uint8_t *kernel_subkey_dest;
	uint8_t *body_sig_dest;
	uint8_t *block_sig_dest;
	VbSignature *sigtmp;

	/* Allocate key block */
	h = (VbFirmwarePreambleHeader *)malloc(block_size);
	if (!h)
		return NULL;

	Memset(h, 0, block_size);
	kernel_subkey_dest = (uint8_t *)(h + 1);
	body_sig_dest = kernel_subkey_dest + kernel_subkey->key_size;
	block_sig_dest = body_sig_dest + body_signature->sig_size;

	h->header_version_major = FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR;
	h->header_version_minor = FIRMWARE_PREAMBLE_HEADER_VERSION_MINOR;
	h->preamble_size = block_size;
	h->firmware_version = firmware_version;
	h->flags = flags;

	/* Copy data key */
	PublicKeyInit(&h->kernel_subkey, kernel_subkey_dest,
		      kernel_subkey->key_size);
	PublicKeyCopy(&h->kernel_subkey, kernel_subkey);

	/* Copy body signature */
	SignatureInit(&h->body_signature, body_sig_dest,
		      body_signature->sig_size, 0);
	SignatureCopy(&h->body_signature, body_signature);

	/* Set up signature struct so we can calculate the signature */
	SignatureInit(&h->preamble_signature, block_sig_dest,
		      siglen_map[signing_key->algorithm], signed_size);

	/* Calculate signature */
	sigtmp = CalculateSignature((uint8_t *)h, signed_size, signing_key);
	SignatureCopy(&h->preamble_signature, sigtmp);
	free(sigtmp);

	/* Return the header */
	return h;
}

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
	const VbPrivateKey *signing_key)
{
	VbKernelPreambleHeader *h;
	uint64_t signed_size = (sizeof(VbKernelPreambleHeader) +
				body_signature->sig_size);
	uint64_t block_size = signed_size + siglen_map[signing_key->algorithm];
	uint8_t *body_sig_dest;
	uint8_t *block_sig_dest;
	VbSignature *sigtmp;

	/* If the block size is smaller than the desired size, pad it */
	if (block_size < desired_size)
		block_size = desired_size;

	/* Allocate key block */
	h = (VbKernelPreambleHeader *)malloc(block_size);
	if (!h)
		return NULL;

	Memset(h, 0, block_size);
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
		      siglen_map[signing_key->algorithm], signed_size);

	/* Calculate signature */
	sigtmp = CalculateSignature((uint8_t *)h, signed_size, signing_key);
	SignatureCopy(&h->preamble_signature, sigtmp);
	free(sigtmp);

	/* Return the header */
	return h;
}
