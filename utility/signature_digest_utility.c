/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility that outputs the cryptographic digest of a contents of a
 * file in a format that can be directly used to generate PKCS#1 v1.5
 * signatures via the "openssl" command line utility.
 */


#include <stdio.h>
#include <stdlib.h>

#include "file_keys.h"
#include "host_common.h"
#include "padding.h"
#include "signature_digest.h"


int main(int argc, char* argv[]) {
  int algorithm = -1;
  int error_code = 0;
  uint8_t* buf = NULL;
  uint8_t* signature_digest = NULL;
  uint64_t len;
  uint32_t signature_digest_len;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <alg_id> <file>", argv[0]);
    return -1;
  }
  algorithm = atoi(argv[1]);
  if (algorithm < 0 || algorithm >= kNumAlgorithms) {
    fprintf(stderr, "Invalid Algorithm!\n");
    return -1;
  }

  buf = BufferFromFile(argv[2], &len);
  if (!buf) {
    fprintf(stderr, "Could not read file: %s\n", argv[2]);
    return -1;
  }

  signature_digest = SignatureDigest(buf, len, algorithm);
  signature_digest_len = (hash_size_map[algorithm] +
                          digestinfo_size_map[algorithm]);
  if (!signature_digest)
    error_code = -1;
  if(signature_digest &&
     1 != fwrite(signature_digest, signature_digest_len, 1, stdout))
    error_code = -1;
  free(signature_digest);
  free(buf);
  return error_code;
}
