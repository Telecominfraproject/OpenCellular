/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 */

#ifndef VBOOT_REFERENCE_VBOOT_COMMON_H_
#define VBOOT_REFERENCE_VBOOT_COMMON_H_

#include "cryptolib.h"
#include "vboot_struct.h"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

/* Error Codes for all common functions. */
enum {
  VBOOT_SUCCESS = 0,
  VBOOT_KEY_BLOCK_INVALID,    /* Key block internal structure is
                               * invalid, or not a key block */
  VBOOT_KEY_BLOCK_SIGNATURE,  /* Key block signature check failed */
  VBOOT_KEY_BLOCK_HASH,      /* Key block hash check failed */
  VBOOT_PUBLIC_KEY_INVALID,  /* Invalid public key passed to a
                              * signature verficiation function. */
  VBOOT_PREAMBLE_INVALID,    /* Preamble internal structure is
                              * invalid */
  VBOOT_PREAMBLE_SIGNATURE,  /* Preamble signature check failed */
  VBOOT_SHARED_DATA_INVALID, /* Shared data is invalid. */
  VBOOT_ERROR_MAX,
};
extern char* kVbootErrors[VBOOT_ERROR_MAX];


/* Return offset of ptr from base. */
uint64_t OffsetOf(const void* base, const void* ptr);


/* Helper functions to get data pointed to by a public key or signature. */
uint8_t* GetPublicKeyData(VbPublicKey* key);
const uint8_t* GetPublicKeyDataC(const VbPublicKey* key);
uint8_t* GetSignatureData(VbSignature* sig);
const uint8_t* GetSignatureDataC(const VbSignature* sig);


/* Helper functions to verify the data pointed to by a subfield is inside
 * the parent data.  Returns 0 if inside, 1 if error. */
int VerifyMemberInside(const void* parent, uint64_t parent_size,
                       const void* member, uint64_t member_size,
                       uint64_t member_data_offset,
                       uint64_t member_data_size);

int VerifyPublicKeyInside(const void* parent, uint64_t parent_size,
                          const VbPublicKey* key);

int VerifySignatureInside(const void* parent, uint64_t parent_size,
                          const VbSignature* sig);


/* Initialize a public key to refer to [key_data]. */
void PublicKeyInit(VbPublicKey* key, uint8_t* key_data, uint64_t key_size);


/* Copy a public key from [src] to [dest].
 *
 * Returns 0 if success, non-zero if error. */
int PublicKeyCopy(VbPublicKey* dest, const VbPublicKey* src);


/* Converts a public key to RsaPublicKey format.  The returned key must
 * be freed using RSAPublicKeyFree().
 *
 * Returns NULL if error. */
RSAPublicKey* PublicKeyToRSA(const VbPublicKey* key);


/* Verifies [data] matches signature [sig] using [key].  [size] is the size
 * of the data buffer; the amount of data to be validated is contained in
 * sig->data_size. */
int VerifyData(const uint8_t* data, uint64_t size, const VbSignature* sig,
               const RSAPublicKey* key);


/* Verifies a secure hash digest from DigestBuf() or DigestFinal(),
 * using [key]. */
int VerifyDigest(const uint8_t* digest, const VbSignature *sig,
                 const RSAPublicKey* key);


/* Checks the sanity of a key block of size [size] bytes, using public
 * key [key].  If hash_only is non-zero, uses only the block checksum
 * to verify the key block.  Header fields are also checked for
 * sanity.  Does not verify key index or key block flags. */
int KeyBlockVerify(const VbKeyBlockHeader* block, uint64_t size,
                   const VbPublicKey *key, int hash_only);


/* Checks the sanity of a firmware preamble of size [size] bytes,
 * using public key [key].
 *
 * Returns VBOOT_SUCCESS if successful. */
int VerifyFirmwarePreamble(const VbFirmwarePreambleHeader* preamble,
                           uint64_t size, const RSAPublicKey* key);


/* Returns the flags from a firmware preamble, or a default value for
 * older preamble versions which didn't contain flags.  Use this
 * function to ensure compatibility with older preamble versions
 * (2.0).  Assumes the preamble has already been verified via
 * VerifyFirmwarePreamble(). */
uint32_t VbGetFirmwarePreambleFlags(const VbFirmwarePreambleHeader* preamble);


/* Checks the sanity of a kernel preamble of size [size] bytes,
 * using public key [key].
 *
 * Returns VBOOT_SUCCESS if successful. */
int VerifyKernelPreamble(const VbKernelPreambleHeader* preamble,
                         uint64_t size, const RSAPublicKey* key);


/* Initialize a verified boot shared data structure.
 *
 * Returns 0 if success, non-zero if error. */
int VbSharedDataInit(VbSharedDataHeader* header, uint64_t size);

/* Reserve [size] bytes of the shared data area.  Returns the offset of the
 * reserved data from the start of the shared data buffer, or 0 if error. */
uint64_t VbSharedDataReserve(VbSharedDataHeader* header, uint64_t size);

/* Copy the kernel subkey into the shared data.
 *
 * Returns 0 if success, non-zero if error. */
int VbSharedDataSetKernelKey(VbSharedDataHeader* header,
                             const VbPublicKey* src);


#endif  /* VBOOT_REFERENCE_VBOOT_COMMON_H_ */
