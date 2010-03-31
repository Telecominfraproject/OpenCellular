/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Routines for verifying a file's signature. Useful in testing the core
 * RSA verification implementation.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "verify_data.h"

/* ANSI Color coding sequences. */
#define COL_GREEN "\e[1;32m"
#define COL_RED "\e[0;31m"
#define COL_STOP "\e[m"

uint8_t* read_signature(char* input_file, int len) {
  int i, sigfd;
  uint8_t* signature = NULL;
  if ((sigfd = open(input_file, O_RDONLY)) == -1) {
    fprintf(stderr, "Couldn't open signature file\n");
    return NULL;
  }

  /* Read the signature into a buffer*/
  signature = (uint8_t*) malloc(len);
  if (!signature)
    return NULL;

  if( (i = read(sigfd, signature, len)) != len ) {
    fprintf(stderr, "Wrong signature length - Expected = %d, Received = %d\n",
            len, i);
    close(sigfd);
    return NULL;
  }

  close(sigfd);
  return signature;
}

int main(int argc, char* argv[]) {
  int i, algorithm, sig_len;
  int return_code = 1;  /* Default to error. */
  uint8_t* digest = NULL;
  uint8_t* signature = NULL;
  RSAPublicKey* key = NULL;

  if (argc!=5) {
    fprintf(stderr, "Usage: %s <algorithm> <key file> <signature file>"
            " <input file>\n\n", argv[0]);
    fprintf(stderr, "where <algorithm> depends on the signature algorithm"
            " used:\n");
    for(i = 0; i<kNumAlgorithms; i++)
      fprintf(stderr, "\t%d for %s\n", i, algo_strings[i]);
    return -1;
  }

  algorithm = atoi(argv[1]);
  if (algorithm >= kNumAlgorithms) {
    fprintf(stderr, "Invalid Algorithm!\n");
    return 0;
  }
  /* Length of the RSA Signature/RSA Key */
  sig_len = siglen_map[algorithm];
  if ((key = RSAPublicKeyFromFile(argv[2])) &&
      (signature = read_signature(argv[3], sig_len)) &&
      (digest = DigestFile(argv[4], algorithm))) {
    if (RSAVerify(key, signature, sig_len, algorithm, digest)) {
      return_code = 0;
      fprintf(stderr, "Signature Verification "
              COL_GREEN "SUCCEEDED" COL_STOP "\n");
    } else {
      fprintf(stderr, "Signature Verification "
              COL_RED "FAILED" COL_STOP "\n");
    }
  }
  else
    return_code = -1;

  free(key);
  free(signature);
  free(digest);

  return return_code;
}
