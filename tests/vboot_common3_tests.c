/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "firmware_image.h"
#include "host_common.h"
#include "test_common.h"
#include "utility.h"
#include "vboot_common.h"


static void ReChecksumKeyBlock(VbKeyBlockHeader *h) {
  uint8_t* newchk = DigestBuf((const uint8_t*)h,
                              h->key_block_checksum.data_size,
                              SHA512_DIGEST_ALGORITHM);
  Memcpy(GetSignatureData(&h->key_block_checksum), newchk, SHA512_DIGEST_SIZE);
  Free(newchk);
}


static void VerifyKeyBlockTest(const VbPublicKey* public_key,
                               const VbPrivateKey* private_key,
                               const VbPublicKey* data_key) {

  VbKeyBlockHeader *hdr;
  VbKeyBlockHeader *h;
  uint64_t hsize;

  hdr = CreateKeyBlock(data_key, private_key, 0x1234);
  TEST_NEQ((size_t)hdr, 0, "VerifyKeyBlock() prerequisites");
  if (!hdr)
    return;
  hsize = hdr->key_block_size;
  h = (VbKeyBlockHeader*)Malloc(hsize + 1024);

  TEST_EQ(VerifyKeyBlock(hdr, hsize, NULL), 0,
          "VerifyKeyBlock() ok using checksum");
  TEST_EQ(VerifyKeyBlock(hdr, hsize, public_key), 0,
          "VerifyKeyBlock() ok using key");

  TEST_NEQ(VerifyKeyBlock(hdr, hsize - 1, NULL), 0, "VerifyKeyBlock() size--");
  TEST_EQ(VerifyKeyBlock(hdr, hsize + 1, NULL), 0, "VerifyKeyBlock() size++");

  Memcpy(h, hdr, hsize);
  h->magic[0] &= 0x12;
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0, "VerifyKeyBlock() magic");

  /* Care about major version but not minor */
  Memcpy(h, hdr, hsize);
  h->header_version_major++;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0, "VerifyKeyBlock() major++");

  Memcpy(h, hdr, hsize);
  h->header_version_major--;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0, "VerifyKeyBlock() major--");

  Memcpy(h, hdr, hsize);
  h->header_version_minor++;
  ReChecksumKeyBlock(h);
  TEST_EQ(VerifyKeyBlock(h, hsize, NULL), 0, "VerifyKeyBlock() minor++");

  Memcpy(h, hdr, hsize);
  h->header_version_minor--;
  ReChecksumKeyBlock(h);
  TEST_EQ(VerifyKeyBlock(h, hsize, NULL), 0, "VerifyKeyBlock() minor--");

  /* Check hash */
  Memcpy(h, hdr, hsize);
  h->key_block_checksum.sig_offset = hsize;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0,
           "VerifyKeyBlock() checksum off end");

  Memcpy(h, hdr, hsize);
  h->key_block_checksum.sig_size /= 2;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0,
           "VerifyKeyBlock() checksum too small");

  Memcpy(h, hdr, hsize);
  GetPublicKeyData(&h->data_key)[0] ^= 0x34;
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0,
           "VerifyKeyBlock() checksum mismatch");

  /* Check signature */
  Memcpy(h, hdr, hsize);
  h->key_block_signature.sig_offset = hsize;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, public_key), 0,
           "VerifyKeyBlock() sig off end");

  Memcpy(h, hdr, hsize);
  h->key_block_signature.sig_size--;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, public_key), 0,
           "VerifyKeyBlock() sig too small");

  Memcpy(h, hdr, hsize);
  GetPublicKeyData(&h->data_key)[0] ^= 0x34;
  TEST_NEQ(VerifyKeyBlock(h, hsize, public_key), 0,
           "VerifyKeyBlock() sig mismatch");

  /* Check that we signed header and data key */
  Memcpy(h, hdr, hsize);
  h->key_block_checksum.data_size = 4;
  h->data_key.key_offset = 0;
  h->data_key.key_size = 0;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0,
           "VerifyKeyBlock() didn't sign header");

  Memcpy(h, hdr, hsize);
  h->data_key.key_offset = hsize;
  ReChecksumKeyBlock(h);
  TEST_NEQ(VerifyKeyBlock(h, hsize, NULL), 0,
           "VerifyKeyBlock() data key off end");

  /* TODO: verify parser can support a bigger header (i.e., one where
   * data_key.key_offset is bigger than expected). */

  Free(h);
  Free(hdr);
}


static void ReSignFirmwarePreamble(VbFirmwarePreambleHeader *h,
                                   const VbPrivateKey *key) {
  VbSignature *sig = CalculateSignature((const uint8_t*)h,
                                        h->preamble_signature.data_size, key);

  SignatureCopy(&h->preamble_signature, sig);
  Free(sig);
}


static void VerifyFirmwarePreambleTest(const VbPublicKey* public_key,
                                       const VbPrivateKey* private_key,
                                       const VbPublicKey* kernel_subkey) {

  VbFirmwarePreambleHeader *hdr;
  VbFirmwarePreambleHeader *h;
  RSAPublicKey* rsa;
  uint64_t hsize;

  /* Create a dummy signature */
  VbSignature *body_sig = SignatureAlloc(56, 78);

  rsa = PublicKeyToRSA(public_key);
  hdr = CreateFirmwarePreamble(0x1234, kernel_subkey, body_sig, private_key);
  TEST_NEQ(hdr && rsa, 0, "VerifyFirmwarePreamble2() prerequisites");
  if (!hdr)
    return;
  hsize = hdr->preamble_size;
  h = (VbFirmwarePreambleHeader*)Malloc(hsize + 16384);

  TEST_EQ(VerifyFirmwarePreamble2(hdr, hsize, rsa), 0,
          "VerifyFirmwarePreamble2() ok using key");
  TEST_NEQ(VerifyFirmwarePreamble2(hdr, hsize - 1, rsa), 0,
           "VerifyFirmwarePreamble2() size--");
  TEST_EQ(VerifyFirmwarePreamble2(hdr, hsize + 1, rsa), 0,
           "VerifyFirmwarePreamble2() size++");

  /* Care about major version but not minor */
  Memcpy(h, hdr, hsize);
  h->header_version_major++;
  ReSignFirmwarePreamble(h, private_key);
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() major++");

  Memcpy(h, hdr, hsize);
  h->header_version_major--;
  ReSignFirmwarePreamble(h, private_key);
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() major--");

  Memcpy(h, hdr, hsize);
  h->header_version_minor++;
  ReSignFirmwarePreamble(h, private_key);
  TEST_EQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
          "VerifyFirmwarePreamble2() minor++");

  Memcpy(h, hdr, hsize);
  h->header_version_minor--;
  ReSignFirmwarePreamble(h, private_key);
  TEST_EQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
          "VerifyFirmwarePreamble2() minor--");

  /* Check signature */
  Memcpy(h, hdr, hsize);
  h->preamble_signature.sig_offset = hsize;
  ReSignFirmwarePreamble(h, private_key);
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() sig off end");

  Memcpy(h, hdr, hsize);
  h->preamble_signature.sig_size--;
  ReSignFirmwarePreamble(h, private_key);
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() sig too small");

  Memcpy(h, hdr, hsize);
  GetPublicKeyData(&h->kernel_subkey)[0] ^= 0x34;
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() sig mismatch");

  /* Check that we signed header, kernel subkey, and body sig */
  Memcpy(h, hdr, hsize);
  h->preamble_signature.data_size = 4;
  h->kernel_subkey.key_offset = 0;
  h->kernel_subkey.key_size = 0;
  h->body_signature.sig_offset = 0;
  h->body_signature.sig_size = 0;
  ReSignFirmwarePreamble(h, private_key);
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() didn't sign header");

  Memcpy(h, hdr, hsize);
  h->kernel_subkey.key_offset = hsize;
  ReSignFirmwarePreamble(h, private_key);
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() kernel subkey off end");

  Memcpy(h, hdr, hsize);
  h->body_signature.sig_offset = hsize;
  ReSignFirmwarePreamble(h, private_key);
  TEST_NEQ(VerifyFirmwarePreamble2(h, hsize, rsa), 0,
           "VerifyFirmwarePreamble2() body sig off end");

  /* TODO: verify parser can support a bigger header. */

  Free(h);
  RSAPublicKeyFree(rsa);
  Free(hdr);
}


int main(int argc, char* argv[]) {
  VbPrivateKey* signing_private_key = NULL;
  VbPublicKey* signing_public_key = NULL;
  int signing_key_algorithm;

  VbPublicKey* data_public_key = NULL;
  int data_key_algorithm;

  int error_code = 0;

  if(argc != 7) {
    fprintf(stderr, "Usage: %s <signing_key_algorithm> <data_key_algorithm>"
            " <signing key> <processed signing pubkey>"
            " <data key> <processed data pubkey>\n", argv[0]);
    return -1;
  }

  /* Read verification keys and create a test image. */
  signing_key_algorithm = atoi(argv[1]);
  data_key_algorithm = atoi(argv[2]);

  signing_private_key = PrivateKeyRead(argv[3], signing_key_algorithm);
  if (!signing_private_key) {
    fprintf(stderr, "Error reading signing_private_key");
    return 1;
  }

  signing_public_key = PublicKeyReadKeyb(argv[4], signing_key_algorithm, 1);
  if (!signing_public_key) {
    fprintf(stderr, "Error reading signing_public_key");
    return 1;
  }

  data_public_key = PublicKeyReadKeyb(argv[6], data_key_algorithm, 1);
  if (!data_public_key) {
    fprintf(stderr, "Error reading data_public_key");
    return 1;
  }

  VerifyKeyBlockTest(signing_public_key, signing_private_key, data_public_key);
  VerifyFirmwarePreambleTest(signing_public_key, signing_private_key,
                             data_public_key);

  if (signing_public_key)
    Free(signing_public_key);
  if (signing_private_key)
    Free(signing_private_key);
  if (data_public_key)
    Free(data_public_key);

  return error_code;
}
