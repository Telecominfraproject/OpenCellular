/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for to generate a padded hash suitable for generating
 * PKCS#1.5 signatures.
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
  uint8_t* digest = NULL;
  uint8_t* padded_digest = NULL;
  uint64_t len;
  uint32_t padded_digest_len;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <alg_id> <digest_file>", argv[0]);
    return -1;
  }
  algorithm = atoi(argv[1]);
  if (algorithm < 0 || algorithm >= kNumAlgorithms) {
    fprintf(stderr, "Invalid Algorithm!\n");
    return -1;
  }

  digest = BufferFromFile(argv[2], &len);
  if (!digest) {
    fprintf(stderr, "Could not read file: %s\n", argv[2]);
    return -1;
  }

  padded_digest = PrependDigestInfo(algorithm, digest);
  padded_digest_len = (hash_size_map[algorithm] +
                       digestinfo_size_map[algorithm]);

  if (!padded_digest)
    error_code = -1;
  if(padded_digest &&
     1 != fwrite(padded_digest, padded_digest_len, 1, stdout))
    error_code = -1;
  free(padded_digest);
  free(digest);
  return error_code;
}
