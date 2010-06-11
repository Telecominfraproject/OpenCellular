/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common functions between firmware and kernel verified boot.
 * (Firmware portion)
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include "vboot_common.h"
#include "utility.h"

#include <stdio.h> /* TODO: FOR TESTING */

char* kVbootErrors[VBOOT_ERROR_MAX] = {
  "Success.",
  "Invalid Image.",
  "Kernel Key Signature Failed.",
  "Invalid Kernel Verification Algorithm.",
  "Preamble Signature Failed.",
  "Kernel Signature Failed.",
  "Wrong Kernel Magic.",
};


uint64_t OffsetOf(const void *base, const void *ptr) {
  return (uint64_t)(size_t)ptr - (uint64_t)(size_t)base;
}


/* Helper functions to get data pointed to by a public key or signature. */
uint8_t* GetPublicKeyData(VbPublicKey* key) {
  return (uint8_t*)key + key->key_offset;
}

const uint8_t* GetPublicKeyDataC(const VbPublicKey* key) {
  return (const uint8_t*)key + key->key_offset;
}

uint8_t* GetSignatureData(VbSignature* sig) {
  return (uint8_t*)sig + sig->sig_offset;
}

const uint8_t* GetSignatureDataC(const VbSignature* sig) {
  return (const uint8_t*)sig + sig->sig_offset;
}


/* Helper functions to verify the data pointed to by a subfield is inside
 * the parent data.  Returns 0 if inside, 1 if error. */
int VerifyMemberInside(const void* parent, uint64_t parent_size,
                       const void* member, uint64_t member_size,
                       uint64_t member_data_offset,
                       uint64_t member_data_size) {
  uint64_t end = OffsetOf(parent, member);

  if (end > parent_size)
    return 1;

  if (end + member_size > parent_size)
    return 1;

  end += member_data_offset;
  if (end > parent_size)
    return 1;
  if (end + member_data_size > parent_size)
    return 1;

  return 0;
}


int VerifyPublicKeyInside(const void* parent, uint64_t parent_size,
                          const VbPublicKey* key) {
  return VerifyMemberInside(parent, parent_size,
                            key, sizeof(VbPublicKey),
                            key->key_offset, key->key_size);
}


int VerifySignatureInside(const void* parent, uint64_t parent_size,
                          const VbSignature* sig) {
  return VerifyMemberInside(parent, parent_size,
                            sig, sizeof(VbSignature),
                            sig->sig_offset, sig->sig_size);
}


RSAPublicKey* PublicKeyToRSA(const VbPublicKey* key) {
  RSAPublicKey *rsa;

  if (kNumAlgorithms <= key->algorithm) {
    debug("Invalid algorithm.\n");
    return NULL;
  }
  if (RSAProcessedKeySize(key->algorithm) != key->key_size) {
    debug("Wrong key size for algorithm\n");
    return NULL;
  }

  rsa = RSAPublicKeyFromBuf(GetPublicKeyDataC(key), key->key_size);
  if (!rsa)
    return NULL;

  rsa->algorithm = key->algorithm;
  return rsa;
}


int VerifyData(const uint8_t* data, const VbSignature *sig,
               const RSAPublicKey* key) {

  if (sig->sig_size != siglen_map[key->algorithm]) {
    debug("Wrong signature size for algorithm.\n");
    return 1;
  }

  if (!RSAVerifyBinary_f(NULL, key, data, sig->data_size,
                         GetSignatureDataC(sig), key->algorithm))
    return 1;

  return 0;
}


int KeyBlockVerify(const VbKeyBlockHeader* block, uint64_t size,
                   const VbPublicKey *key) {

  const VbSignature* sig;

  /* Sanity checks before attempting signature of data */
  if (SafeMemcmp(block->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE)) {
    debug("Not a valid verified boot key block.\n");
    return 1;
  }
  if (block->header_version_major != KEY_BLOCK_HEADER_VERSION_MAJOR) {
    debug("Incompatible key block header version.\n");
    return 1;
  }
  if (size < block->key_block_size) {
    debug("Not enough data for key block.\n");
    return 1;
  }

  /* Check signature or hash, depending on whether we have a key. */
  if (key) {
    /* Check signature */
    RSAPublicKey* rsa;
    int rv;

    sig = &block->key_block_signature;

    if (VerifySignatureInside(block, block->key_block_size, sig)) {
      debug("Key block signature off end of block\n");
      return 1;
    }

    if (!((rsa = PublicKeyToRSA(key)))) {
      debug("Invalid public key\n");
      return 1;
    }
    rv = VerifyData((const uint8_t*)block, sig, rsa);
    RSAPublicKeyFree(rsa);

    if (rv)
      return rv;

  } else {
    /* Check hash */
    uint8_t* header_checksum = NULL;
    int rv;

    sig = &block->key_block_checksum;

    if (VerifySignatureInside(block, block->key_block_size, sig)) {
      debug("Key block hash off end of block\n");
      return 1;
    }
    if (sig->sig_size != SHA512_DIGEST_SIZE) {
      debug("Wrong hash size for key block.\n");
      return 1;
    }

    header_checksum = DigestBuf((const uint8_t*)block, sig->data_size,
                                SHA512_DIGEST_ALGORITHM);
    rv = SafeMemcmp(header_checksum, GetSignatureDataC(sig),
                    SHA512_DIGEST_SIZE);
    Free(header_checksum);
    if (rv) {
      debug("Invalid key block hash.\n");
      return 1;
    }
  }

  /* Verify we signed enough data */
  if (sig->data_size < sizeof(VbKeyBlockHeader)) {
    debug("Didn't sign enough data\n");
    return 1;
  }

  /* Verify data key is inside the block and inside signed data */
  if (VerifyPublicKeyInside(block, block->key_block_size, &block->data_key)) {
    debug("Data key off end of key block\n");
    return 1;
  }
  if (VerifyPublicKeyInside(block, sig->data_size, &block->data_key)) {
    debug("Data key off end of signed data\n");
    return 1;
  }

  /* Success */
  return 0;
}


int VerifyFirmwarePreamble2(const VbFirmwarePreambleHeader* preamble,
                           uint64_t size, const RSAPublicKey* key) {

  const VbSignature* sig = &preamble->preamble_signature;

  /* TODO: caller needs to make sure key version is valid */

  /* Sanity checks before attempting signature of data */
  if (preamble->header_version_major !=
      FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR) {
    debug("Incompatible firmware preamble header version.\n");
    return 1;
  }
  if (size < preamble->preamble_size) {
    debug("Not enough data for preamble.\n");
    return 1;
  }

  /* Check signature */
  if (VerifySignatureInside(preamble, preamble->preamble_size, sig)) {
    debug("Preamble signature off end of preamble\n");
    return 1;
  }
  if (VerifyData((const uint8_t*)preamble, sig, key)) {
    debug("Preamble signature validation failed\n");
    return 1;
  }

  /* Verify we signed enough data */
  if (sig->data_size < sizeof(VbFirmwarePreambleHeader)) {
    debug("Didn't sign enough data\n");
    return 1;
  }

  /* Verify body signature is inside the block */
  if (VerifySignatureInside(preamble, preamble->preamble_size,
                            &preamble->body_signature)) {
    debug("Firmware body signature off end of preamble\n");
    return 1;
  }

  /* Verify kernel subkey is inside the block */
  if (VerifyPublicKeyInside(preamble, preamble->preamble_size,
                            &preamble->kernel_subkey)) {
    debug("Kernel subkey off end of preamble\n");
    return 1;
  }

  /* Success */
  return 0;
}


int VerifyKernelPreamble2(const VbKernelPreambleHeader* preamble,
                         uint64_t size, const RSAPublicKey* key) {

  const VbSignature* sig = &preamble->preamble_signature;

  /* TODO: caller needs to make sure key version is valid */

  /* Sanity checks before attempting signature of data */
  if (preamble->header_version_major != KERNEL_PREAMBLE_HEADER_VERSION_MAJOR) {
    debug("Incompatible kernel preamble header version.\n");
    return 1;
  }
  if (size < preamble->preamble_size) {
    debug("Not enough data for preamble.\n");
    return 1;
  }

  /* Check signature */
  if (VerifySignatureInside(preamble, preamble->preamble_size, sig)) {
    debug("Preamble signature off end of preamble\n");
    return 1;
  }
  if (VerifyData((const uint8_t*)preamble, sig, key)) {
    debug("Preamble signature validation failed\n");
    return 1;
  }

  /* Verify we signed enough data */
  if (sig->data_size < sizeof(VbKernelPreambleHeader)) {
    debug("Didn't sign enough data\n");
    return 1;
  }

  /* Verify body signature is inside the block */
  if (VerifySignatureInside(preamble, preamble->preamble_size,
                            &preamble->body_signature)) {
    debug("Kernel body signature off end of preamble\n");
    return 1;
  }

  /* Success */
  return 0;
}
