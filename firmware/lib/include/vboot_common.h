/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 */

#ifndef VBOOT_REFERENCE_VBOOT_COMMON_H_
#define VBOOT_REFERENCE_VBOOT_COMMON_H_

#include "cryptolib.h"
#include "vboot_struct.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#endif

/* Test an important condition at compile time, not run time */
#ifndef BUILD_ASSERT
#define _BA1_(cond, line) \
	extern int __build_assertion_ ## line[1 - 2*!(cond)] \
	__attribute__ ((unused))
#define _BA0_(c, x) _BA1_(c, x)
#define BUILD_ASSERT(cond) _BA0_(cond, __LINE__)
#endif

/* Error Codes for all common functions. */
enum {
	VBOOT_SUCCESS = 0,
	/* Key block internal structure is invalid, or not a key block */
	VBOOT_KEY_BLOCK_INVALID,
	/* Key block signature check failed */
	VBOOT_KEY_BLOCK_SIGNATURE,
	/* Key block hash check failed */
	VBOOT_KEY_BLOCK_HASH,
	/* Invalid public key passed to a signature verficiation function. */
	VBOOT_PUBLIC_KEY_INVALID,
	/* Preamble internal structure is invalid */
	VBOOT_PREAMBLE_INVALID,
	/* Preamble signature check failed */
	VBOOT_PREAMBLE_SIGNATURE,
	/* Shared data is invalid. */
	VBOOT_SHARED_DATA_INVALID,
	/* Kernel Preamble does not contain flags */
	VBOOT_KERNEL_PREAMBLE_NO_FLAGS,
	VBOOT_ERROR_MAX,
};
extern const char *kVbootErrors[VBOOT_ERROR_MAX];

/**
 * Return offset of ptr from base.
 */
uint64_t OffsetOf(const void *base, const void *ptr);

/*
 * Helper functions to get data pointed to by a public key or signature.
 */

uint8_t *GetPublicKeyData(VbPublicKey *key);
const uint8_t *GetPublicKeyDataC(const VbPublicKey *key);
uint8_t *GetSignatureData(VbSignature *sig);
const uint8_t *GetSignatureDataC(const VbSignature *sig);

/*
 * Helper functions to verify the data pointed to by a subfield is inside the
 * parent data.  Returns 0 if inside, 1 if error.
 */

int VerifyMemberInside(const void *parent, uint64_t parent_size,
		       const void *member, uint64_t member_size,
		       uint64_t member_data_offset,
		       uint64_t member_data_size);

int VerifyPublicKeyInside(const void *parent, uint64_t parent_size,
			  const VbPublicKey *key);

int VerifySignatureInside(const void *parent, uint64_t parent_size,
			  const VbSignature *sig);

/**
 * Initialize a public key to refer to [key_data].
 */
void PublicKeyInit(VbPublicKey *key, uint8_t *key_data, uint64_t key_size);

/**
 * Copy a public key from [src] to [dest].
 *
 * Returns 0 if success, non-zero if error.
 */
int PublicKeyCopy(VbPublicKey *dest, const VbPublicKey *src);

/**
 * Convert a public key to RsaPublicKey format.  The returned key must be freed
 * using RSAPublicKeyFree().
 *
 * Returns NULL if error.
 */
RSAPublicKey *PublicKeyToRSA(const VbPublicKey *key);

/**
 * Verify [data] matches signature [sig] using [key].  [size] is the size of
 * the data buffer; the amount of data to be validated is contained in
 * sig->data_size.
 */
int VerifyData(const uint8_t *data, uint64_t size, const VbSignature *sig,
	       const RSAPublicKey *key);

/**
 * Verify a secure hash digest from DigestBuf() or DigestFinal(), using
 * [key]. Returns 0 on success.
 */
int VerifyDigest(const uint8_t *digest, const VbSignature *sig,
		 const RSAPublicKey *key);

/**
 * Check the sanity of a key block of size [size] bytes, using public key
 * [key].  If hash_only is non-zero, uses only the block checksum to verify the
 * key block.  Header fields are also checked for sanity.  Does not verify key
 * index or key block flags.
 */
int KeyBlockVerify(const VbKeyBlockHeader *block, uint64_t size,
		   const VbPublicKey *key, int hash_only);


/**
 * Check the sanity of a firmware preamble of size [size] bytes, using public
 * key [key].
 *
 * Returns VBOOT_SUCCESS if successful.
 */
int VerifyFirmwarePreamble(const VbFirmwarePreambleHeader *preamble,
			   uint64_t size, const RSAPublicKey *key);


/**
 * Return the flags from a firmware preamble, or a default value for older
 * preamble versions which didn't contain flags.  Use this function to ensure
 * compatibility with older preamble versions (2.0).  Assumes the preamble has
 * already been verified via VerifyFirmwarePreamble().
 */
uint32_t VbGetFirmwarePreambleFlags(const VbFirmwarePreambleHeader *preamble);

/**
 * Check the sanity of a kernel preamble of size [size] bytes, using public key
 * [key].
 *
 * Returns VBOOT_SUCCESS if successful.
 */
int VerifyKernelPreamble(const VbKernelPreambleHeader *preamble,
			 uint64_t size, const RSAPublicKey *key);


/**
 * Retrieve the 16-bit vmlinuz header address and size from the kernel preamble
 * if there is one.  These are only available in Kernel Preamble Header version
 * >= 2.1.  If given a header 2.0 or lower, will set address and size to 0 (this
 * is not considered an error).
 *
 * Returns VBOOT_SUCCESS if successful.
 */
int VbGetKernelVmlinuzHeader(const VbKernelPreambleHeader *preamble,
			     uint64_t *vmlinuz_header_address,
			     uint64_t *vmlinuz_header_size);

/**
 * Checks if the kernel preamble has flags field. This is available only if the
 * Kernel Preamble Header version >=2.2. If give a header of 2.1 or lower, it
 * will return VBOOT_KERNEL_PREAMBLE_NO_FLAGS.
 *
 * Returns VBOOT_SUCCESS if version is >=2.2.
 */
int VbKernelHasFlags(const VbKernelPreambleHeader *preamble);

/**
 * Verify that the Vmlinuz Header is contained inside of the kernel blob.
 *
 * Returns VBOOT_SUCCESS or VBOOT_PREAMBLE_INVALID on error
 */
int VerifyVmlinuzInsideKBlob(uint64_t kblob, uint64_t kblob_size,
			     uint64_t header, uint64_t header_size);
/**
 * Initialize a verified boot shared data structure.
 *
 * Returns 0 if success, non-zero if error.
 */
int VbSharedDataInit(VbSharedDataHeader *header, uint64_t size);

/**
 * Reserve [size] bytes of the shared data area.  Returns the offset of the
 * reserved data from the start of the shared data buffer, or 0 if error.
 */
uint64_t VbSharedDataReserve(VbSharedDataHeader *header, uint64_t size);

/**
 * Copy the kernel subkey into the shared data.
 *
 * Returns 0 if success, non-zero if error.
 */
int VbSharedDataSetKernelKey(VbSharedDataHeader *header,
                             const VbPublicKey *src);

#endif  /* VBOOT_REFERENCE_VBOOT_COMMON_H_ */
