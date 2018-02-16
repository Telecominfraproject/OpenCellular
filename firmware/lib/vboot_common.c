/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 * (Firmware portion)
 */

#include "sysincludes.h"
#include "2sysincludes.h"

#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "utility.h"

const char *kVbootErrors[VBOOT_ERROR_MAX] = {
	"Success.",
	"Key block invalid.",
	"Key block signature failed.",
	"Key block hash failed.",
	"Public key invalid.",
	"Preamble invalid.",
	"Preamble signature check failed.",
	"Shared data invalid."
};

uint64_t OffsetOf(const void *base, const void *ptr)
{
	return (uint64_t)(size_t)ptr - (uint64_t)(size_t)base;
}

/* Helper functions to get data pointed to by a public key or signature. */

uint8_t *GetPublicKeyData(VbPublicKey *key)
{
	return (uint8_t *)key + key->key_offset;
}

const uint8_t *GetPublicKeyDataC(const VbPublicKey *key)
{
	return (const uint8_t *)key + key->key_offset;
}

uint8_t *GetSignatureData(VbSignature *sig)
{
	return (uint8_t *)sig + sig->sig_offset;
}

const uint8_t *GetSignatureDataC(const VbSignature *sig)
{
	return (const uint8_t *)sig + sig->sig_offset;
}

/*
 * Helper functions to verify the data pointed to by a subfield is inside
 * the parent data.  Returns 0 if inside, 1 if error.
 */

int VerifyMemberInside(const void *parent, uint64_t parent_size,
                       const void *member, uint64_t member_size,
                       uint64_t member_data_offset,
                       uint64_t member_data_size)
{
	uint64_t end = OffsetOf(parent, member);

	if (end > parent_size)
		return 1;

	if (UINT64_MAX - end < member_size)
		return 1;  /* Detect wraparound in integer math */
	if (end + member_size > parent_size)
		return 1;

	if (UINT64_MAX - end < member_data_offset)
		return 1;
	end += member_data_offset;
	if (end > parent_size)
		return 1;

	if (UINT64_MAX - end < member_data_size)
		return 1;
	if (end + member_data_size > parent_size)
		return 1;

	return 0;
}

int VerifyPublicKeyInside(const void *parent, uint64_t parent_size,
                          const VbPublicKey *key)
{
	return VerifyMemberInside(parent, parent_size,
				  key, sizeof(VbPublicKey),
				  key->key_offset, key->key_size);
}

int VerifySignatureInside(const void *parent, uint64_t parent_size,
                          const VbSignature *sig)
{
	return VerifyMemberInside(parent, parent_size,
				  sig, sizeof(VbSignature),
				  sig->sig_offset, sig->sig_size);
}

void PublicKeyInit(VbPublicKey *key, uint8_t *key_data, uint64_t key_size)
{
	key->key_offset = OffsetOf(key, key_data);
	key->key_size = key_size;
	key->algorithm = VB2_ALG_COUNT; /* Key not present yet */
	key->key_version = 0;
}

int PublicKeyCopy(VbPublicKey *dest, const VbPublicKey *src)
{
	if (dest->key_size < src->key_size)
		return 1;

	dest->key_size = src->key_size;
	dest->algorithm = src->algorithm;
	dest->key_version = src->key_version;
	memcpy(GetPublicKeyData(dest), GetPublicKeyDataC(src), src->key_size);
	return 0;
}

int VbGetKernelVmlinuzHeader(const VbKernelPreambleHeader *preamble,
			     uint64_t *vmlinuz_header_address,
			     uint64_t *vmlinuz_header_size)
{
	*vmlinuz_header_address = 0;
	*vmlinuz_header_size = 0;
	if (preamble->header_version_minor > 0) {
		/*
		 * Set header and size only if the preamble header version is >
		 * 2.1 as they don't exist in version 2.0 (Note that we don't
		 * need to check header_version_major; if that's not 2 then
		 * VerifyKernelPreamble() would have already failed.
		 */
		*vmlinuz_header_address = preamble->vmlinuz_header_address;
		*vmlinuz_header_size = preamble->vmlinuz_header_size;
	}
	return VBOOT_SUCCESS;
}

int VbKernelHasFlags(const VbKernelPreambleHeader *preamble)
{
	if (preamble->header_version_minor > 1)
		return VBOOT_SUCCESS;

	return VBOOT_KERNEL_PREAMBLE_NO_FLAGS;
}

int VerifyVmlinuzInsideKBlob(uint64_t kblob, uint64_t kblob_size,
			     uint64_t header, uint64_t header_size)
{
	uint64_t end = header-kblob;
	if (end > kblob_size)
		return VBOOT_PREAMBLE_INVALID;
	if (UINT64_MAX - end < header_size)
		return VBOOT_PREAMBLE_INVALID;
	if (end + header_size > kblob_size)
		return VBOOT_PREAMBLE_INVALID;

	return VBOOT_SUCCESS;
}

uint64_t VbSharedDataReserve(VbSharedDataHeader *header, uint64_t size)
{
	if (!header || size > header->data_size - header->data_used) {
		VB2_DEBUG("VbSharedData buffer out of space.\n");
		return 0;  /* Not initialized, or not enough space left. */
	}

	uint64_t offs = header->data_used;
	VB2_DEBUG("VbSharedDataReserve %d bytes at %d\n", (int)size, (int)offs);

	header->data_used += size;
	return offs;
}

int VbSharedDataSetKernelKey(VbSharedDataHeader *header, const VbPublicKey *src)
{
	VbPublicKey *kdest;

	if (!header)
		return VBOOT_SHARED_DATA_INVALID;
	if (!src)
		return VBOOT_PUBLIC_KEY_INVALID;

	kdest = &header->kernel_subkey;

	VB2_DEBUG("Saving kernel subkey to shared data: size %d, algo %d\n",
		  vb2_rsa_sig_size(vb2_crypto_to_signature(src->algorithm)),
		  (int)src->algorithm);

	/* Attempt to allocate space for key, if it hasn't been allocated yet */
	if (!header->kernel_subkey_data_offset) {
		header->kernel_subkey_data_offset =
			VbSharedDataReserve(header, src->key_size);
		if (!header->kernel_subkey_data_offset)
			return VBOOT_SHARED_DATA_INVALID;
		header->kernel_subkey_data_size = src->key_size;
	}

	/* Copy the kernel sign key blob into the destination buffer */
	PublicKeyInit(kdest,
		      (uint8_t *)header + header->kernel_subkey_data_offset,
		      header->kernel_subkey_data_size);

	return PublicKeyCopy(kdest, src);
}

int vb2_allow_recovery(uint32_t flags)
{
	/*
	 * If EC is in RW, it implies recovery wasn't manually requested.
	 * On some platforms, EC_IN_RW can't be reset by the EC, thus, this may
	 * return false (=RW). That's ok because if recovery is manual, we will
	 * get the right signal and that's the case we care about.
	 */
	if (!VbExTrustEC(0))
		return 0;

	/* Now we confidently check the recovery switch state at boot */
	return !!(flags & VBSD_BOOT_REC_SWITCH_ON);
}
