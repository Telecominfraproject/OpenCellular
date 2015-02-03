/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 * (Firmware portion)
 */

#include "sysincludes.h"

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
	key->algorithm = kNumAlgorithms; /* Key not present yet */
	key->key_version = 0;
}

int PublicKeyCopy(VbPublicKey *dest, const VbPublicKey *src)
{
	if (dest->key_size < src->key_size)
		return 1;

	dest->key_size = src->key_size;
	dest->algorithm = src->algorithm;
	dest->key_version = src->key_version;
	Memcpy(GetPublicKeyData(dest), GetPublicKeyDataC(src), src->key_size);
	return 0;
}

RSAPublicKey *PublicKeyToRSA(const VbPublicKey *key)
{
	RSAPublicKey *rsa;
	uint64_t key_size;

	if (kNumAlgorithms <= key->algorithm) {
		VBDEBUG(("Invalid algorithm.\n"));
		return NULL;
	}
	if (!RSAProcessedKeySize(key->algorithm, &key_size) ||
	    key_size != key->key_size) {
		VBDEBUG(("Wrong key size for algorithm\n"));
		return NULL;
	}

	rsa = RSAPublicKeyFromBuf(GetPublicKeyDataC(key), key->key_size);
	if (!rsa)
		return NULL;

	rsa->algorithm = (unsigned int)key->algorithm;
	return rsa;
}

int VerifyData(const uint8_t *data, uint64_t size, const VbSignature *sig,
               const RSAPublicKey *key)
{
	VBDEBUG(("   - sig_size=%d, expecting %d for algorithm %d\n",
		 (unsigned)sig->sig_size, siglen_map[key->algorithm],
		 key->algorithm));
	if (sig->sig_size != siglen_map[key->algorithm]) {
		VBDEBUG(("Wrong data signature size for algorithm, "
			 "sig_size=%d, expected %d for algorithm %d.\n",
			 (int)sig->sig_size, siglen_map[key->algorithm],
			 key->algorithm));
		return 1;
	}
	if (sig->data_size > size) {
		VBDEBUG(("Data buffer smaller than length of signed data.\n"));
		return 1;
	}

	if (!RSAVerifyBinary_f(NULL, key, data, sig->data_size,
			       GetSignatureDataC(sig), key->algorithm))
		return 1;

	return 0;
}

int VerifyDigest(const uint8_t *digest, const VbSignature *sig,
                 const RSAPublicKey *key)
{
	if (sig->sig_size != siglen_map[key->algorithm]) {
		VBDEBUG(("Wrong digest signature size for algorithm.\n"));
		return 1;
	}

	if (!RSAVerifyBinaryWithDigest_f(NULL, key, digest,
					 GetSignatureDataC(sig),
					 key->algorithm))
		return 1;

	return 0;
}

int KeyBlockVerify(const VbKeyBlockHeader *block, uint64_t size,
                   const VbPublicKey *key, int hash_only)
{
	const VbSignature *sig;

	/* Sanity checks before attempting signature of data */
	if(size < sizeof(VbKeyBlockHeader)) {
		VBDEBUG(("Not enough space for key block header.\n"));
		return VBOOT_KEY_BLOCK_INVALID;
	}
	if (SafeMemcmp(block->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE)) {
		VBDEBUG(("Not a valid verified boot key block.\n"));
		return VBOOT_KEY_BLOCK_INVALID;
	}
	if (block->header_version_major != KEY_BLOCK_HEADER_VERSION_MAJOR) {
		VBDEBUG(("Incompatible key block header version.\n"));
		return VBOOT_KEY_BLOCK_INVALID;
	}
	if (size < block->key_block_size) {
		VBDEBUG(("Not enough data for key block.\n"));
		return VBOOT_KEY_BLOCK_INVALID;
	}
	if (!hash_only && !key) {
		VBDEBUG(("Missing required public key.\n"));
		return VBOOT_PUBLIC_KEY_INVALID;
	}

	/*
	 * Check signature or hash, depending on the hash_only parameter. Note
	 * that we don't require a key even if the keyblock has a signature,
	 * because the caller may not care if the keyblock itself is signed
	 * (for example, booting a Google-signed kernel in developer mode).
	 */
	if (hash_only) {
		/* Check hash */
		uint8_t *header_checksum = NULL;
		int rv;

		sig = &block->key_block_checksum;

		if (VerifySignatureInside(block, block->key_block_size, sig)) {
			VBDEBUG(("Key block hash off end of block\n"));
			return VBOOT_KEY_BLOCK_INVALID;
		}
		if (sig->sig_size != SHA512_DIGEST_SIZE) {
			VBDEBUG(("Wrong hash size for key block.\n"));
			return VBOOT_KEY_BLOCK_INVALID;
		}

		/* Make sure advertised signature data sizes are sane. */
		if (block->key_block_size < sig->data_size) {
			VBDEBUG(("Signature calculated past end of block\n"));
			return VBOOT_KEY_BLOCK_INVALID;
		}

		VBDEBUG(("Checking key block hash only...\n"));
		header_checksum = DigestBuf((const uint8_t *)block,
					    sig->data_size,
					    SHA512_DIGEST_ALGORITHM);
		rv = SafeMemcmp(header_checksum, GetSignatureDataC(sig),
				SHA512_DIGEST_SIZE);
		VbExFree(header_checksum);
		if (rv) {
			VBDEBUG(("Invalid key block hash.\n"));
			return VBOOT_KEY_BLOCK_HASH;
		}
	} else {
		/* Check signature */
		RSAPublicKey *rsa;
		int rv;

		sig = &block->key_block_signature;

		if (VerifySignatureInside(block, block->key_block_size, sig)) {
			VBDEBUG(("Key block signature off end of block\n"));
			return VBOOT_KEY_BLOCK_INVALID;
		}

		rsa = PublicKeyToRSA(key);
		if (!rsa) {
			VBDEBUG(("Invalid public key\n"));
			return VBOOT_PUBLIC_KEY_INVALID;
		}

		/* Make sure advertised signature data sizes are sane. */
		if (block->key_block_size < sig->data_size) {
			VBDEBUG(("Signature calculated past end of block\n"));
			RSAPublicKeyFree(rsa);
			return VBOOT_KEY_BLOCK_INVALID;
		}

		VBDEBUG(("Checking key block signature...\n"));
		rv = VerifyData((const uint8_t *)block, size, sig, rsa);
		RSAPublicKeyFree(rsa);
		if (rv) {
			VBDEBUG(("Invalid key block signature.\n"));
			return VBOOT_KEY_BLOCK_SIGNATURE;
		}
	}

	/* Verify we signed enough data */
	if (sig->data_size < sizeof(VbKeyBlockHeader)) {
		VBDEBUG(("Didn't sign enough data\n"));
		return VBOOT_KEY_BLOCK_INVALID;
	}

	/* Verify data key is inside the block and inside signed data */
	if (VerifyPublicKeyInside(block, block->key_block_size,
				  &block->data_key)) {
		VBDEBUG(("Data key off end of key block\n"));
		return VBOOT_KEY_BLOCK_INVALID;
	}
	if (VerifyPublicKeyInside(block, sig->data_size, &block->data_key)) {
		VBDEBUG(("Data key off end of signed data\n"));
		return VBOOT_KEY_BLOCK_INVALID;
	}

	/* Success */
	return VBOOT_SUCCESS;
}

int VerifyFirmwarePreamble(const VbFirmwarePreambleHeader *preamble,
                           uint64_t size, const RSAPublicKey *key)
{
	const VbSignature *sig = &preamble->preamble_signature;

	VBDEBUG(("Verifying preamble.\n"));
	/* Sanity checks before attempting signature of data */
	if(size < EXPECTED_VBFIRMWAREPREAMBLEHEADER2_0_SIZE) {
		VBDEBUG(("Not enough data for preamble header 2.0.\n"));
		return VBOOT_PREAMBLE_INVALID;
	}
	if (preamble->header_version_major !=
	    FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR) {
		VBDEBUG(("Incompatible firmware preamble header version.\n"));
		return VBOOT_PREAMBLE_INVALID;
	}
	if (size < preamble->preamble_size) {
		VBDEBUG(("Not enough data for preamble.\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/* Check signature */
	if (VerifySignatureInside(preamble, preamble->preamble_size, sig)) {
		VBDEBUG(("Preamble signature off end of preamble\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/* Make sure advertised signature data sizes are sane. */
	if (preamble->preamble_size < sig->data_size) {
		VBDEBUG(("Signature calculated past end of the block\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	if (VerifyData((const uint8_t *)preamble, size, sig, key)) {
		VBDEBUG(("Preamble signature validation failed\n"));
		return VBOOT_PREAMBLE_SIGNATURE;
	}

	/* Verify we signed enough data */
	if (sig->data_size < sizeof(VbFirmwarePreambleHeader)) {
		VBDEBUG(("Didn't sign enough data\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/* Verify body signature is inside the signed data */
	if (VerifySignatureInside(preamble, sig->data_size,
				  &preamble->body_signature)) {
		VBDEBUG(("Firmware body signature off end of preamble\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/* Verify kernel subkey is inside the signed data */
	if (VerifyPublicKeyInside(preamble, sig->data_size,
				  &preamble->kernel_subkey)) {
		VBDEBUG(("Kernel subkey off end of preamble\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/*
	 * If the preamble header version is at least 2.1, verify we have space
	 * for the added fields from 2.1.
	 */
	if (preamble->header_version_minor >= 1) {
		if(size < EXPECTED_VBFIRMWAREPREAMBLEHEADER2_1_SIZE) {
			VBDEBUG(("Not enough data for preamble header 2.1.\n"));
			return VBOOT_PREAMBLE_INVALID;
		}
	}

	/* Success */
	return VBOOT_SUCCESS;
}

uint32_t VbGetFirmwarePreambleFlags(const VbFirmwarePreambleHeader *preamble)
{
	if (preamble->header_version_minor < 1) {
		/*
		 * Old structure; return default flags.  (Note that we don't
		 * need to check header_version_major; if that's not 2 then
		 * VerifyFirmwarePreamble() would have already failed.
		 */
		return 0;
	}

	return preamble->flags;
}

int VerifyKernelPreamble(const VbKernelPreambleHeader *preamble,
                         uint64_t size, const RSAPublicKey *key)
{
	const VbSignature *sig = &preamble->preamble_signature;

	/* Sanity checks before attempting signature of data */
	if(size < sizeof(VbKernelPreambleHeader)) {
		VBDEBUG(("Not enough data for preamble header.\n"));
		return VBOOT_PREAMBLE_INVALID;
	}
	if (preamble->header_version_major !=
	    KERNEL_PREAMBLE_HEADER_VERSION_MAJOR) {
		VBDEBUG(("Incompatible kernel preamble header version.\n"));
		return VBOOT_PREAMBLE_INVALID;
	}
	if (size < preamble->preamble_size) {
		VBDEBUG(("Not enough data for preamble.\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/* Check signature */
	if (VerifySignatureInside(preamble, preamble->preamble_size, sig)) {
		VBDEBUG(("Preamble signature off end of preamble\n"));
		return VBOOT_PREAMBLE_INVALID;
	}
	if (VerifyData((const uint8_t *)preamble, size, sig, key)) {
		VBDEBUG(("Preamble signature validation failed\n"));
		return VBOOT_PREAMBLE_SIGNATURE;
	}

	/* Verify we signed enough data */
	if (sig->data_size < sizeof(VbKernelPreambleHeader)) {
		VBDEBUG(("Didn't sign enough data\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/* Verify body signature is inside the signed data */
	if (VerifySignatureInside(preamble, sig->data_size,
				  &preamble->body_signature)) {
		VBDEBUG(("Kernel body signature off end of preamble\n"));
		return VBOOT_PREAMBLE_INVALID;
	}

	/*
	 * If the preamble header version is at least 2.1, verify we have space
	 * for the added fields from >2.1.
	 */
	if (preamble->header_version_minor >= 1) {
		if((preamble->header_version_minor == 1) &&
		   (size < EXPECTED_VBKERNELPREAMBLEHEADER2_1_SIZE)) {
			VBDEBUG(("Not enough data for preamble header 2.1.\n"));
			return VBOOT_PREAMBLE_INVALID;
		}

		if((preamble->header_version_minor == 2) &&
		   (size < EXPECTED_VBKERNELPREAMBLEHEADER2_2_SIZE)) {
			VBDEBUG(("Not enough data for preamble header 2.2.\n"));
			return VBOOT_PREAMBLE_INVALID;
		}
	}

	/* Success */
	return VBOOT_SUCCESS;
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
	uint64_t offs = header->data_used;

	VBDEBUG(("VbSharedDataReserve %d bytes at %d\n", (int)size, (int)offs));

	if (!header || size > header->data_size - header->data_used) {
		VBDEBUG(("VbSharedData buffer out of space.\n"));
		return 0;  /* Not initialized, or not enough space left. */
	}
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

	VBDEBUG(("Saving kernel subkey to shared data: size %d, algo %d\n",
		 siglen_map[src->algorithm], (int)src->algorithm));

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
