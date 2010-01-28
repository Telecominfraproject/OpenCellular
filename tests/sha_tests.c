/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* FIPS 180-2 Tests for message digest functions. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sha.h"

#include "sha_test_vectors.h"

int SHA1_tests(void) {
  int i, success = 1;
  uint8_t sha1_digest[SHA1_DIGEST_SIZE];
  uint8_t* test_inputs[3];
  test_inputs[0] = (uint8_t *) oneblock_msg;
  test_inputs[1] = (uint8_t *) multiblock_msg1;
  test_inputs[2] = (uint8_t *) long_msg;

  for (i = 0; i < 3; i++) {
    SHA1(test_inputs[i], strlen((char *)test_inputs[i]),
         sha1_digest);
    if (!memcmp(sha1_digest, sha1_results[i], SHA1_DIGEST_SIZE)) {
      fprintf(stderr, "Test vector %d PASSED for SHA-1\n", i+1);
    }
    else {
      fprintf(stderr, "Test vector %d FAILED for SHA-1\n", i+1);
      success = 0;
    }
  }
  return success;
}

int SHA256_tests(void) {
  int i, success = 1;
  uint8_t sha256_digest[SHA256_DIGEST_SIZE];
  uint8_t* test_inputs[3];
  test_inputs[0] = (uint8_t *) oneblock_msg;
  test_inputs[1] = (uint8_t *) multiblock_msg1;
  test_inputs[2] = (uint8_t *) long_msg;

  for (i = 0; i < 3; i++) {
    SHA256(test_inputs[i], strlen((char *)test_inputs[i]),
           sha256_digest);
    if (!memcmp(sha256_digest, sha256_results[i], SHA256_DIGEST_SIZE)) {
      fprintf(stderr, "Test vector %d PASSED for SHA-256\n", i+1);
    }
    else {
      fprintf(stderr, "Test vector %d FAILED for SHA-256\n", i+1);
      success = 0;
    }
  }
  return success;
}

int SHA512_tests(void) {
  int i, success = 1;
  uint8_t sha512_digest[SHA512_DIGEST_SIZE];
  uint8_t* test_inputs[3];
  test_inputs[0] = (uint8_t *) oneblock_msg;
  test_inputs[1] = (uint8_t *) multiblock_msg2;
  test_inputs[2] = (uint8_t *) long_msg;

  for (i = 0; i < 3; i++) {
    SHA512(test_inputs[i], strlen((char *)test_inputs[i]),
           sha512_digest);
    if (!memcmp(sha512_digest, sha512_results[i], SHA512_DIGEST_SIZE)) {
      fprintf(stderr, "Test vector %d PASSED for SHA-512\n", i+1);
    }
    else {
      fprintf(stderr, "Test vector %d FAILED for SHA-512\n", i+1);
      success = 0;
    }
  }
  return success;
}

int main(int argc, char* argv[]) {
  int success = 1;
  /* Initialize long_msg with 'a' x 1,000,000 */
  long_msg = (char *) malloc(1000001);
  memset(long_msg, 'a', 1000000);
  long_msg[1000000]=0;

  if (!SHA1_tests())
    success = 0;
  if (!SHA256_tests())
    success = 0;
  if (!SHA512_tests())
    success = 0;

  free(long_msg);

  return !success;
}
