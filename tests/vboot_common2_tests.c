/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "host_common.h"
#include "test_common.h"
#include "vboot_common.h"


static void VerifyPublicKeyToRSA(const VbPublicKey* orig_key) {

  RSAPublicKey *rsa;
  VbPublicKey *key = PublicKeyAlloc(orig_key->key_size, 0, 0);

  PublicKeyCopy(key, orig_key);
  key->algorithm = kNumAlgorithms;
  TEST_EQ((size_t)PublicKeyToRSA(key), 0,
          "PublicKeyToRSA() invalid algorithm");

  PublicKeyCopy(key, orig_key);
  key->key_size -= 1;
  TEST_EQ((size_t)PublicKeyToRSA(key), 0,
          "PublicKeyToRSA() invalid size");

  rsa = PublicKeyToRSA(orig_key);
  TEST_NEQ((size_t)rsa, 0, "PublicKeyToRSA() ok");
  if (rsa) {
    TEST_EQ((int)rsa->algorithm, (int)key->algorithm,
            "PublicKeyToRSA() algorithm");
    RSAPublicKeyFree(rsa);
  }
}


static void VerifyDataTest(const VbPublicKey* public_key,
                           const VbPrivateKey* private_key) {

  const uint8_t test_data[] = "This is some test data to sign.";
  const uint64_t test_size = sizeof(test_data);
  VbSignature* sig;
  RSAPublicKey* rsa;

  sig = CalculateSignature(test_data, test_size, private_key);
  rsa = PublicKeyToRSA(public_key);
  TEST_NEQ(sig && rsa, 0, "VerifyData() prerequisites");
  if (!sig || !rsa)
    return;

  TEST_EQ(VerifyData(test_data, test_size, sig, rsa), 0, "VerifyData() ok");

  sig->sig_size -= 16;
  TEST_EQ(VerifyData(test_data, test_size, sig, rsa), 1,
          "VerifyData() wrong sig size");
  sig->sig_size += 16;

  TEST_EQ(VerifyData(test_data, test_size - 1, sig, rsa), 1,
          "VerifyData() input buffer too small");

  GetSignatureData(sig)[0] ^= 0x5A;
  TEST_EQ(VerifyData(test_data, test_size, sig, rsa), 1,
          "VerifyData() wrong sig");

  RSAPublicKeyFree(rsa);
  free(sig);
}


static void VerifyDigestTest(const VbPublicKey* public_key,
                             const VbPrivateKey* private_key) {

  const uint8_t test_data[] = "This is some other test data to sign.";
  VbSignature* sig;
  RSAPublicKey* rsa;
  uint8_t* digest;

  sig = CalculateSignature(test_data, sizeof(test_data), private_key);
  rsa = PublicKeyToRSA(public_key);
  digest = DigestBuf(test_data, sizeof(test_data), (int)public_key->algorithm);
  TEST_NEQ(sig && rsa && digest, 0, "VerifyData() prerequisites");
  if (!sig || !rsa || !digest)
    return;

  TEST_EQ(VerifyDigest(digest, sig, rsa), 0, "VerifyDigest() ok");

  GetSignatureData(sig)[0] ^= 0x5A;
  TEST_EQ(VerifyDigest(digest, sig, rsa), 1, "VerifyDigest() wrong sig");

  RSAPublicKeyFree(rsa);
  free(sig);
  free(digest);
}


static void ReSignKernelPreamble(VbKernelPreambleHeader *h,
                                 const VbPrivateKey *key) {
  VbSignature *sig = CalculateSignature((const uint8_t*)h,
                                        h->preamble_signature.data_size, key);

  SignatureCopy(&h->preamble_signature, sig);
  free(sig);
}


static void VerifyKernelPreambleTest(const VbPublicKey* public_key,
                                     const VbPrivateKey* private_key) {

  VbKernelPreambleHeader *hdr;
  VbKernelPreambleHeader *h;
  RSAPublicKey* rsa;
  unsigned hsize;

  /* Create a dummy signature */
  VbSignature *body_sig = SignatureAlloc(56, 78);

  rsa = PublicKeyToRSA(public_key);
  hdr = CreateKernelPreamble(0x1234, 0x100000, 0x300000, 0x4000, body_sig,
                             0, private_key);
  TEST_NEQ(hdr && rsa, 0, "VerifyKernelPreamble() prerequisites");
  if (!hdr)
    return;
  hsize = (unsigned) hdr->preamble_size;
  h = (VbKernelPreambleHeader*)malloc(hsize + 16384);

  TEST_EQ(VerifyKernelPreamble(hdr, hsize, rsa), 0,
          "VerifyKernelPreamble() ok using key");
  TEST_NEQ(VerifyKernelPreamble(hdr, hsize - 1, rsa), 0,
           "VerifyKernelPreamble() size--");
  TEST_EQ(VerifyKernelPreamble(hdr, hsize + 1, rsa), 0,
           "VerifyKernelPreamble() size++");

  /* Care about major version but not minor */
  Memcpy(h, hdr, hsize);
  h->header_version_major++;
  ReSignKernelPreamble(h, private_key);
  TEST_NEQ(VerifyKernelPreamble(h, hsize, rsa), 0,
           "VerifyKernelPreamble() major++");

  Memcpy(h, hdr, hsize);
  h->header_version_major--;
  ReSignKernelPreamble(h, private_key);
  TEST_NEQ(VerifyKernelPreamble(h, hsize, rsa), 0,
           "VerifyKernelPreamble() major--");

  Memcpy(h, hdr, hsize);
  h->header_version_minor++;
  ReSignKernelPreamble(h, private_key);
  TEST_EQ(VerifyKernelPreamble(h, hsize, rsa), 0,
          "VerifyKernelPreamble() minor++");

  Memcpy(h, hdr, hsize);
  h->header_version_minor--;
  ReSignKernelPreamble(h, private_key);
  TEST_EQ(VerifyKernelPreamble(h, hsize, rsa), 0,
          "VerifyKernelPreamble() minor--");

  /* Check signature */
  Memcpy(h, hdr, hsize);
  h->preamble_signature.sig_offset = hsize;
  ReSignKernelPreamble(h, private_key);
  TEST_NEQ(VerifyKernelPreamble(h, hsize, rsa), 0,
           "VerifyKernelPreamble() sig off end");

  Memcpy(h, hdr, hsize);
  h->preamble_signature.sig_size--;
  ReSignKernelPreamble(h, private_key);
  TEST_NEQ(VerifyKernelPreamble(h, hsize, rsa), 0,
           "VerifyKernelPreamble() sig too small");

  Memcpy(h, hdr, hsize);
  GetSignatureData(&h->body_signature)[0] ^= 0x34;
  TEST_NEQ(VerifyKernelPreamble(h, hsize, rsa), 0,
           "VerifyKernelPreamble() sig mismatch");

  /* Check that we signed header and body sig */
  Memcpy(h, hdr, hsize);
  h->preamble_signature.data_size = 4;
  h->body_signature.sig_offset = 0;
  h->body_signature.sig_size = 0;
  ReSignKernelPreamble(h, private_key);
  TEST_NEQ(VerifyKernelPreamble(h, hsize, rsa), 0,
           "VerifyKernelPreamble() didn't sign header");

  Memcpy(h, hdr, hsize);
  h->body_signature.sig_offset = hsize;
  ReSignKernelPreamble(h, private_key);
  TEST_NEQ(VerifyKernelPreamble(h, hsize, rsa), 0,
           "VerifyKernelPreamble() body sig off end");

  /* TODO: verify parser can support a bigger header. */

  free(h);
  RSAPublicKeyFree(rsa);
  free(hdr);
}


int main(int argc, char* argv[]) {
  VbPrivateKey* private_key = NULL;
  VbPublicKey* public_key = NULL;
  int key_algorithm;

  int error_code = 0;

  if(argc != 4) {
    fprintf(stderr, "Usage: %s <key_algorithm> <key> <processed pubkey>"
            " <signing key> <processed signing key>\n", argv[0]);
    return -1;
  }

  /* Read verification keys and create a test image. */
  key_algorithm = atoi(argv[1]);

  private_key = PrivateKeyReadPem(argv[2], key_algorithm);
  if (!private_key) {
    fprintf(stderr, "Error reading private_key");
    return 1;
  }

  public_key = PublicKeyReadKeyb(argv[3], key_algorithm, 1);
  if (!public_key) {
    fprintf(stderr, "Error reading public_key");
    return 1;
  }

  VerifyPublicKeyToRSA(public_key);
  VerifyDataTest(public_key, private_key);
  VerifyDigestTest(public_key, private_key);
  VerifyKernelPreambleTest(public_key, private_key);

  if (public_key)
    free(public_key);
  if (private_key)
    free(private_key);

  return error_code;
}
