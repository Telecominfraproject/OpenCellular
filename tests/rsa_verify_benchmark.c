/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>

#include "file_keys.h"
#include "padding.h"
#include "rsa.h"
#include "timer_utils.h"
#include "utility.h"

#define FILE_NAME_SIZE 128
#define NUM_OPERATIONS 100 /* Number of signature operations to time. */

void SpeedTestAlgorithm(int algorithm) {
  int i, key_size;
  double speed, msecs;
  char file_name[FILE_NAME_SIZE];
  uint8_t* digest = NULL;
  uint8_t* signature = NULL;
  int digest_len;
  int sig_len;
  RSAPublicKey* key = NULL;
  ClockTimerState ct;
  char* sha_strings[] = {  /* Maps algorithm->SHA algorithm. */
    "sha1", "sha256", "sha512",  /* RSA-1024 */
    "sha1", "sha256", "sha512",  /* RSA-2048 */
    "sha1", "sha256", "sha512",  /* RSA-4096 */
    "sha1", "sha256", "sha512",  /* RSA-8192 */
  };

  key_size = siglen_map[algorithm] * sizeof(uint32_t) * 8;  /* in bits. */
  /* Get key. */
  snprintf(file_name, FILE_NAME_SIZE, "testkeys/key_rsa%d.keyb", key_size);
  key = RSAPublicKeyFromFile(file_name);
  if (!key) {
    fprintf(stderr, "Couldn't read key from file.\n");
    goto failure;
  }

  /* Get expected digest. */
  snprintf(file_name, FILE_NAME_SIZE, "testcases/test_file.%s.digest",
           sha_strings[algorithm]);
  digest = BufferFromFile(file_name, &digest_len);
  if (!digest) {
    fprintf(stderr, "Couldn't read digest file.\n");
    goto failure;
  }

  /* Get signature to verify against. */
  snprintf(file_name, FILE_NAME_SIZE, "testcases/test_file.rsa%d_%s.sig",
           key_size, sha_strings[algorithm]);
  signature = BufferFromFile(file_name, &sig_len);
  if (!signature) {
    fprintf(stderr, "Couldn't read signature file.\n");
    goto failure;
  }

  StartTimer(&ct);
  for (i = 0; i < NUM_OPERATIONS; i++) {
    if (!RSA_verify(key, signature, sig_len, algorithm, digest))
      fprintf(stderr, "Warning: Signature Check Failed.\n");
  }
  StopTimer(&ct);

  msecs = (float) GetDurationMsecs(&ct) / NUM_OPERATIONS;
  speed = 1000.0 / msecs ;
  fprintf(stderr, "rsa%d/%s bits:\tTime taken per verification = %.02f ms,"
          " Speed = %.02f verifications/s\n", key_size, sha_strings[algorithm],
          msecs, speed);

failure:
  Free(signature);
  Free(digest);
  Free(key);
}

int main(int argc, char* argv[]) {
  int i;
  for (i = 0; i < kNumAlgorithms; ++i) {
    SpeedTestAlgorithm(i);
  }
  return 0;
}
