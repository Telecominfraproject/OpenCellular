/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for EC firmware vboot stuff.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "host_common.h"
#include "test_common.h"
#include "vboot_common.h"

static void ReSignECPreamble(VbECPreambleHeader* h,
                                   const VbPrivateKey* key) {
  VbSignature *sig = CalculateSignature((const uint8_t*)h,
                                        h->preamble_signature.data_size, key);

  SignatureCopy(&h->preamble_signature, sig);
  free(sig);
}


static void VerifyECPreambleTest(const VbPublicKey* public_key,
                                 const VbPrivateKey* private_key) {
  VbECPreambleHeader* hdr;
  VbECPreambleHeader* h;
  RSAPublicKey* rsa;
  unsigned hsize;

  /* Create a dummy signature */
  VbSignature* body_sig = SignatureAlloc(56, 78);

  rsa = PublicKeyToRSA(public_key);
  hdr = CreateECPreamble(0x1234, body_sig, private_key,
                         0x5678, "Foo bar");
  TEST_NEQ(hdr && rsa, 0, "VerifyECPreamble() prerequisites");
  if (!hdr)
    return;

  hsize = (unsigned) hdr->preamble_size;
  h = (VbECPreambleHeader*)malloc(hsize + 16384);

  TEST_EQ(VerifyECPreamble(hdr, hsize, rsa), 0,
          "VerifyECPreamble() ok using key");
  TEST_NEQ(VerifyECPreamble(hdr, hsize - 1, rsa), 0,
           "VerifyECPreamble() size--");
  TEST_EQ(VerifyECPreamble(hdr, hsize + 1, rsa), 0,
           "VerifyECPreamble() size++");

  TEST_EQ(hdr->firmware_version, 0x1234,
          "VerifyECPreamble() firmware version");
  TEST_EQ(hdr->flags, 0x5678,
          "VerifyECPreamble() flags");
  TEST_EQ(strncmp(hdr->name, "Foo bar", sizeof(hdr->name)), 0,
          "VerifyECPreamble() name");

  /* Care about major version but not minor */
  Memcpy(h, hdr, hsize);
  h->header_version_major++;
  ReSignECPreamble(h, private_key);
  TEST_NEQ(VerifyECPreamble(h, hsize, rsa), 0,
           "VerifyECPreamble() major++");

  Memcpy(h, hdr, hsize);
  h->header_version_major--;
  ReSignECPreamble(h, private_key);
  TEST_NEQ(VerifyECPreamble(h, hsize, rsa), 0,
           "VerifyECPreamble() major--");

  Memcpy(h, hdr, hsize);
  h->header_version_minor++;
  ReSignECPreamble(h, private_key);
  TEST_EQ(VerifyECPreamble(h, hsize, rsa), 0,
          "VerifyECPreamble() minor++");

  Memcpy(h, hdr, hsize);
  h->header_version_minor--;
  ReSignECPreamble(h, private_key);
  TEST_EQ(VerifyECPreamble(h, hsize, rsa), 0,
          "VerifyECPreamble() minor--");

  /* Check signature */
  Memcpy(h, hdr, hsize);
  h->preamble_signature.sig_offset = hsize;
  ReSignECPreamble(h, private_key);
  TEST_NEQ(VerifyECPreamble(h, hsize, rsa), 0,
           "VerifyECPreamble() sig off end");

  Memcpy(h, hdr, hsize);
  h->preamble_signature.sig_size--;
  ReSignECPreamble(h, private_key);
  TEST_NEQ(VerifyECPreamble(h, hsize, rsa), 0,
           "VerifyECPreamble() sig too small");

  Memcpy(h, hdr, hsize);
  GetSignatureData(&h->body_digest)[0] ^= 0x34;
  TEST_NEQ(VerifyECPreamble(h, hsize, rsa), 0,
           "VerifyECPreamble() sig mismatch");

  /* Check that we signed header and body sig */
  Memcpy(h, hdr, hsize);
  h->preamble_signature.data_size = 4;
  h->body_digest.sig_offset = 0;
  h->body_digest.sig_size = 0;
  ReSignECPreamble(h, private_key);
  TEST_NEQ(VerifyECPreamble(h, hsize, rsa), 0,
           "VerifyECPreamble() didn't sign header");

  Memcpy(h, hdr, hsize);
  h->body_digest.sig_offset = hsize;
  ReSignECPreamble(h, private_key);
  TEST_NEQ(VerifyECPreamble(h, hsize, rsa), 0,
           "VerifyECPreamble() body sig off end");

  /* TODO: verify with extra padding at end of header. */

  free(h);
  RSAPublicKeyFree(rsa);
  free(hdr);
}


int main(int argc, char* argv[]) {
  VbPrivateKey* signing_private_key = NULL;
  VbPublicKey* signing_public_key = NULL;

  int error_code = 0;

  if(argc != 3) {
    fprintf(stderr, "Usage: %s <signing privkey> <signing pubkey>", argv[0]);
    return -1;
  }

  signing_private_key = PrivateKeyRead(argv[1]);
  if (!signing_private_key) {
    fprintf(stderr, "Error reading signing_private_key\n");
    return 1;
  }

  signing_public_key = PublicKeyRead(argv[2]);
  if (!signing_public_key) {
    fprintf(stderr, "Error reading signing_public_key\n");
    return 1;
  }

  VerifyECPreambleTest(signing_public_key, signing_private_key);


  if (signing_public_key)
    free(signing_public_key);
  if (signing_private_key)
    free(signing_private_key);

  return error_code;
}
