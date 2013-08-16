/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */


#include "cryptolib.h"
#include "host_common.h"
#include "host_keyblock.h"
#include "vboot_common.h"


VbKeyBlockHeader* KeyBlockCreate(const VbPublicKey* data_key,
                                 const VbPrivateKey* signing_key,
                                 uint64_t flags) {

  VbKeyBlockHeader* h;
  uint64_t signed_size = sizeof(VbKeyBlockHeader) + data_key->key_size;
  uint64_t block_size = (signed_size + SHA512_DIGEST_SIZE +
                         (signing_key ?
                          siglen_map[signing_key->algorithm] : 0));
  uint8_t* data_key_dest;
  uint8_t* block_sig_dest;
  uint8_t* block_chk_dest;
  VbSignature *sigtmp;

  /* Allocate key block */
  h = (VbKeyBlockHeader*)malloc(block_size);
  if (!h)
    return NULL;
  data_key_dest = (uint8_t*)(h + 1);
  block_chk_dest = data_key_dest + data_key->key_size;
  block_sig_dest = block_chk_dest + SHA512_DIGEST_SIZE;

  Memcpy(h->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE);
  h->header_version_major = KEY_BLOCK_HEADER_VERSION_MAJOR;
  h->header_version_minor = KEY_BLOCK_HEADER_VERSION_MINOR;
  h->key_block_size = block_size;
  h->key_block_flags = flags;

  /* Copy data key */
  PublicKeyInit(&h->data_key, data_key_dest, data_key->key_size);
  PublicKeyCopy(&h->data_key, data_key);

  /* Set up signature structs so we can calculate the signatures */
  SignatureInit(&h->key_block_checksum, block_chk_dest,
                SHA512_DIGEST_SIZE, signed_size);
  if (signing_key)
    SignatureInit(&h->key_block_signature, block_sig_dest,
                  siglen_map[signing_key->algorithm], signed_size);
  else
    Memset(&h->key_block_signature, 0, sizeof(VbSignature));

  /* Calculate checksum */
  sigtmp = CalculateChecksum((uint8_t*)h, signed_size);
  SignatureCopy(&h->key_block_checksum, sigtmp);
  free(sigtmp);

  /* Calculate signature */
  if (signing_key) {
    sigtmp = CalculateSignature((uint8_t*)h, signed_size, signing_key);
    SignatureCopy(&h->key_block_signature, sigtmp);
    free(sigtmp);
  }

  /* Return the header */
  return h;
}

/* TODO(gauravsh): This could easily be integrated into KeyBlockCreate()
 * since the code is almost a mirror - I have kept it as such to avoid changing
 * the existing interface. */
VbKeyBlockHeader* KeyBlockCreate_external(const VbPublicKey* data_key,
                                          const char* signing_key_pem_file,
                                          uint64_t algorithm,
                                          uint64_t flags,
                                          const char* external_signer) {
  VbKeyBlockHeader* h;
  uint64_t signed_size = sizeof(VbKeyBlockHeader) + data_key->key_size;
  uint64_t block_size = (signed_size + SHA512_DIGEST_SIZE +
                         siglen_map[algorithm]);
  uint8_t* data_key_dest;
  uint8_t* block_sig_dest;
  uint8_t* block_chk_dest;
  VbSignature *sigtmp;

  /* Allocate key block */
  h = (VbKeyBlockHeader*)malloc(block_size);
  if (!h)
    return NULL;
  if (!signing_key_pem_file || !data_key || !external_signer)
    return NULL;

  data_key_dest = (uint8_t*)(h + 1);
  block_chk_dest = data_key_dest + data_key->key_size;
  block_sig_dest = block_chk_dest + SHA512_DIGEST_SIZE;

  Memcpy(h->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE);
  h->header_version_major = KEY_BLOCK_HEADER_VERSION_MAJOR;
  h->header_version_minor = KEY_BLOCK_HEADER_VERSION_MINOR;
  h->key_block_size = block_size;
  h->key_block_flags = flags;

  /* Copy data key */
  PublicKeyInit(&h->data_key, data_key_dest, data_key->key_size);
  PublicKeyCopy(&h->data_key, data_key);

  /* Set up signature structs so we can calculate the signatures */
  SignatureInit(&h->key_block_checksum, block_chk_dest,
                SHA512_DIGEST_SIZE, signed_size);
  SignatureInit(&h->key_block_signature, block_sig_dest,
                siglen_map[algorithm], signed_size);

  /* Calculate checksum */
  sigtmp = CalculateChecksum((uint8_t*)h, signed_size);
  SignatureCopy(&h->key_block_checksum, sigtmp);
  free(sigtmp);

  /* Calculate signature */
  sigtmp = CalculateSignature_external((uint8_t*)h, signed_size,
                                       signing_key_pem_file, algorithm,
                                       external_signer);
  SignatureCopy(&h->key_block_signature, sigtmp);
  free(sigtmp);

  /* Return the header */
  return h;
}

/* Read a key block from a .keyblock file.  Caller owns the returned
 * pointer, and must free it with free().
 *
 * Returns NULL if error. */
VbKeyBlockHeader* KeyBlockRead(const char* filename) {

  VbKeyBlockHeader* block;
  uint64_t file_size;

  block = (VbKeyBlockHeader*)ReadFile(filename, &file_size);
  if (!block) {
    VBDEBUG(("Error reading key block file: %s\n", filename));
    return NULL;
  }

  /* Verify the hash of the key block, since we can do that without
   * the public signing key. */
  if (0 != KeyBlockVerify(block, file_size, NULL, 1)) {
    VBDEBUG(("Invalid key block file: %s\n", filename));
    free(block);
    return NULL;
  }

  return block;
}


/* Write a key block to a file in .keyblock format. */
int KeyBlockWrite(const char* filename, const VbKeyBlockHeader* key_block) {

  if (0 != WriteFile(filename, key_block, key_block->key_block_size)) {
    VBDEBUG(("KeyBlockWrite() error writing key block\n"));
    return 1;
  }

  return 0;
}
