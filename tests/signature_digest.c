/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility that outputs the message digest of the contents of a file in a
 * format that can be used as input to OpenSSL for an RSA signature.
 * Needed until the stable OpenSSL release supports SHA-256/512 digests for
 * RSA signatures.
 * Outputs DigestInfo || Digest where DigestInfo is the OID depending on the
 * choice of the hash algorithm (see padding.c).
 *
 */

#include "signature_digest.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "digest_utility.h"
#include "padding.h"
#include "sha.h"

uint8_t* prepend_digestinfo(int algorithm, uint8_t* digest) {
  const int digest_size = hash_size_map[algorithm];
  const int digestinfo_size = digestinfo_size_map[algorithm];
  const uint8_t* digestinfo = hash_digestinfo_map[algorithm];
  uint8_t* p = malloc(digestinfo_size + digest_size);
  memcpy(p, digestinfo, digestinfo_size);
  memcpy(p + digestinfo_size, digest, digest_size);
  return p;
}

int main(int argc, char* argv[]) {
  int i, algorithm;
  uint8_t* digest = NULL;
  uint8_t* signature = NULL;
  uint8_t* info_digest  = NULL;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <algorithm> <input file>\n\n",
            argv[0]);
    fprintf(stderr, "where <algorithm> is the signature algorithm to use:\n");
    for(i = 0; i<kNumAlgorithms; i++)
      fprintf(stderr, "\t%d for %s\n", i, algo_strings[i]);
    return -1;
  }

  algorithm = atoi(argv[1]);
  if (algorithm >= kNumAlgorithms) {
    fprintf(stderr, "Invalid Algorithm!\n");
    goto failure;
  }

  if (!(digest = calculate_digest(argv[2], algorithm)))
    goto failure;

  info_digest = prepend_digestinfo(algorithm, digest);
  write(1, info_digest, hash_size_map[algorithm] +
        digestinfo_size_map[algorithm]);

failure:
  free(digest);
  free(info_digest);
  free(signature);

  return 0;
}
