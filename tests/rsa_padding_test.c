/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdint.h>
#include <stdio.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "rsa_padding_test.h"
#include "test_common.h"
#include "utility.h"

/* Test valid and invalid signatures */
static void TestSignatures(RSAPublicKey* key) {
  int unexpected_success;
  int i;

  /* The first test signature is valid. */
  TEST_EQ(RSAVerify(key, signatures[0], RSA1024NUMBYTES, 0,
                    test_message_sha1_hash), 1, "RSA Padding Test valid sig");

  /* All other signatures should fail verification. */
  unexpected_success = 0;
  for (i = 1; i < sizeof(signatures) / sizeof(signatures[0]); i++) {
    if (RSAVerify(key, signatures[i], RSA1024NUMBYTES, 0,
                  test_message_sha1_hash)) {
      fprintf(stderr, "RSA Padding Test vector %d FAILED!\n", i);
      unexpected_success++;
    }
  }
  TEST_EQ(unexpected_success, 0, "RSA Padding Test invalid sigs");

}


/* Test other error conditions in RSAVerify() */
static void TestRSAVerify(RSAPublicKey* key) {
  uint8_t sig[RSA1024NUMBYTES];

  TEST_EQ(RSAVerify(key, signatures[0], RSA1024NUMBYTES, 0,
                    test_message_sha1_hash), 1, "RSAVerify() good");
  TEST_EQ(RSAVerify(key, signatures[0], RSA1024NUMBYTES - 1, 0,
                    test_message_sha1_hash), 0, "RSAVerify() sig len");
  TEST_EQ(RSAVerify(key, signatures[0], RSA1024NUMBYTES, kNumAlgorithms,
                    test_message_sha1_hash), 0, "RSAVerify() invalid alg");
  TEST_EQ(RSAVerify(key, signatures[0], RSA1024NUMBYTES, 3,
                    test_message_sha1_hash), 0, "RSAVerify() wrong alg");

  /* Corrupt the signature near start and end */
  Memcpy(sig, signatures[0], RSA1024NUMBYTES);
  sig[3] ^= 0x42;
  TEST_EQ(RSAVerify(key, sig, RSA1024NUMBYTES, 0, test_message_sha1_hash), 0,
          "RSAVerify() bad sig");

  Memcpy(sig, signatures[0], RSA1024NUMBYTES);
  sig[RSA1024NUMBYTES - 3] ^= 0x56;
  TEST_EQ(RSAVerify(key, sig, RSA1024NUMBYTES, 0, test_message_sha1_hash), 0,
          "RSAVerify() bad sig end");
}


int main(int argc, char* argv[]) {
  int error = 0;
  RSAPublicKey* key;

  /* Read test key */
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <test public key>\n", argv[0]);
    return 1;
  }
  key = RSAPublicKeyFromFile(argv[1]);
  if (!key) {
    fprintf(stderr, "Couldn't read RSA public key for the test.\n");
    return 1;
  }

  /* Run tests */
  TestSignatures(key);
  TestRSAVerify(key);

  /* Clean up and exit */
  RSAPublicKeyFree(key);

  if (!gTestSuccess)
    error = 255;

  return error;
}
