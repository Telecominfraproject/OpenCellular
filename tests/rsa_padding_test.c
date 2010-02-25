/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "rsa_padding_test.h"

#include <stdio.h>

#include "file_keys.h"
#include "rsa_utility.h"

int main(int argc, char* argv[]) {
  int i;
  int error = 0;
  RSAPublicKey* key;
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <test public key>\n", argv[0]);
    return 1;
  }
  key = RSAPublicKeyFromFile(argv[1]);
  if (!key) {
    fprintf(stderr, "Couldn't read RSA public key for the test.\n");
    return 1;
  }

  /* The first test signature is valid. */
  if (!RSA_verify(key, signatures[0], RSA1024NUMBYTES, 0,
                  test_message_sha1_hash)) {
    fprintf(stderr, "RSA Padding Test vector 0 FAILED!\n");
    error = 255;  /* Test failure. */
  }
  /* All other signatures should fail verification. */
  for (i = 1; i < sizeof(signatures) / sizeof(signatures[0]); i++) {
    if (RSA_verify(key, signatures[i], RSA1024NUMBYTES, 0,
                  test_message_sha1_hash)) {
      fprintf(stderr, "RSA Padding Test vector %d FAILED!\n", i);
      error = 255;  /* Test failure. */
    }
  }
  if (!error)
    fprintf(stderr, "RSA Padding Test PASSED for all test vectors.");

  return error;
}
