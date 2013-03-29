/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "host_common.h"
#include "timer_utils.h"

#define FILE_NAME_SIZE 128
#define NUM_OPERATIONS 100 /* Number of signature operations to time. */

int SpeedTestAlgorithm(int algorithm) {
  int i, key_size;
  int error_code = 0;
  double speed, msecs;
  char file_name[FILE_NAME_SIZE];
  uint8_t* digest = NULL;
  uint8_t* signature = NULL;
  uint64_t digest_len, sig_len;
  RSAPublicKey* key = NULL;
  ClockTimerState ct;
  char* sha_strings[] = {  /* Maps algorithm->SHA algorithm. */
    "sha1", "sha256", "sha512",  /* RSA-1024 */
    "sha1", "sha256", "sha512",  /* RSA-2048 */
    "sha1", "sha256", "sha512",  /* RSA-4096 */
    "sha1", "sha256", "sha512",  /* RSA-8192 */
  };

  key_size = siglen_map[algorithm] * 8;  /* in bits. */
  /* Get key. */
  snprintf(file_name, FILE_NAME_SIZE, "testkeys/key_rsa%d.keyb", key_size);
  key = RSAPublicKeyFromFile(file_name);
  if (!key) {
    VBDEBUG(("Couldn't read RSA Public key from file: %s\n", file_name));
    error_code = 1;
    goto failure;
  }

  /* Get expected digest. */
  snprintf(file_name, FILE_NAME_SIZE, "testcases/test_file.%s.digest",
           sha_strings[algorithm]);
  digest = BufferFromFile(file_name, &digest_len);
  if (!digest) {
    VBDEBUG(("Couldn't read digest file.\n"));
    error_code = 1;
    goto failure;
  }

  /* Get signature to verify against. */
  snprintf(file_name, FILE_NAME_SIZE, "testcases/test_file.rsa%d_%s.sig",
           key_size, sha_strings[algorithm]);
  signature = BufferFromFile(file_name, &sig_len);
  if (!signature) {
    VBDEBUG(("Couldn't read signature file.\n"));
    error_code = 1;
    goto failure;
  }

  StartTimer(&ct);
  for (i = 0; i < NUM_OPERATIONS; i++) {
    if (!RSAVerify(key, signature, sig_len, algorithm, digest))
      VBDEBUG(("Warning: Signature Check Failed.\n"));
  }
  StopTimer(&ct);

  msecs = (float) GetDurationMsecs(&ct) / NUM_OPERATIONS;
  speed = 1000.0 / msecs ;
  fprintf(stderr, "# rsa%d/%s:\tTime taken per verification = %.02f ms,"
          " Speed = %.02f verifications/s\n", key_size, sha_strings[algorithm],
          msecs, speed);
  fprintf(stdout, "ms_rsa%d_%s:%.02f\n", key_size, sha_strings[algorithm],
          msecs);

failure:
  free(signature);
  free(digest);
  RSAPublicKeyFree(key);
  return error_code;
}

int main(int argc, char* argv[]) {
  int i;
  int error_code = 0;
  for (i = 0; i < kNumAlgorithms; ++i) {
    if(SpeedTestAlgorithm(i))
      error_code = 1;
  }
  return error_code;
}
