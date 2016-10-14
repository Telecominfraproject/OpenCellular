/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2sysincludes.h"

#include "2common.h"
#include "2sha.h"
#include "file_keys.h"
#include "host_common.h"
#include "padding.h"
#include "signature_digest.h"

static void usage(char* argv[]) {
  fprintf(stderr,
          "Usage: %s <alg_id> <digest_file>\n"
          "\n"
          "Generate a padded hash suitable for generating PKCS#1.5 "
          "signatures.\n",
          basename(argv[0]));
}

int main(int argc, char* argv[]) {
  int algorithm = -1;
  int error_code = 0;
  uint8_t* digest = NULL;
  uint8_t* padded_digest = NULL;
  uint32_t len;
  uint32_t padded_digest_len;

  if (argc != 3) {
    usage(argv);
    return -1;
  }
  algorithm = atoi(argv[1]);
  if (algorithm < 0 || algorithm >= kNumAlgorithms) {
    fprintf(stderr, "Invalid Algorithm!\n");
    return -1;
  }

  if (VB2_SUCCESS != vb2_read_file(argv[2], &digest, &len)) {
    fprintf(stderr, "Could not read file: %s\n", argv[2]);
    return -1;
  }

  padded_digest = PrependDigestInfo(algorithm, digest);
  const int digest_size = vb2_digest_size(vb2_crypto_to_hash(algorithm));
  padded_digest_len = (digest_size + digestinfo_size_map[algorithm]);

  if (!padded_digest)
    error_code = -1;
  if(padded_digest &&
     1 != fwrite(padded_digest, padded_digest_len, 1, stdout))
    error_code = -1;
  free(padded_digest);
  free(digest);
  return error_code;
}
