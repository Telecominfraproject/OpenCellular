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

#include "padding.h"
#include "rsa.h"
#include "sha.h"
#include "verify_data.h"


RSAPublicKey* read_RSAkey(char *input_file, int len) {
  int key_fd;
  RSAPublicKey *key = NULL;

  if ((key_fd = open(input_file, O_RDONLY)) == -1) {
    fprintf(stderr, "Couldn't open pre-processed key file\n");
    return NULL;
  }

  key = (RSAPublicKey *) malloc(sizeof(RSAPublicKey));
  if (!key)
    return NULL;

  /* Read the pre-processed RSA key into a RSAPublicKey structure */
  /* TODO(gauravsh): Add error checking here? */

  read(key_fd, &key->len, sizeof(key->len));
  read(key_fd, &key->n0inv, sizeof(key->n0inv));

#ifndef NDEBUG
  fprintf(stderr, "%d\n", key->len);
  fprintf(stderr, "%d\n", key->n0inv);
#endif

  key->n = (uint32_t *) malloc(len);
  read(key_fd, key->n, len);

  key->rr = (uint32_t *) malloc(len);
  read(key_fd, key->rr, len);

#ifndef NDEBUG
  {
    int i;
    for(i=0; i<key->len; i++) {
      fprintf(stderr, "%d,", key->n[i]);
    }
    fprintf(stderr, "\n");

    for(i=0; i<key->len; i++) {
      fprintf(stderr, "%d,", key->rr[i]);
    }
    fprintf(stderr, "\n");
  }
#endif

  close(key_fd);
  return key;
}

uint8_t* SHA1_file(char *input_file) {
  int i, input_fd, len;
  uint8_t data[SHA1_BLOCK_SIZE], *digest = NULL, *p = NULL;
  SHA1_CTX ctx;

  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    fprintf(stderr, "Couldn't open input file.\n");
    return NULL;
  }

  /* Calculate SHA1 hash of input blocks, reading one block at a time. */
  SHA1_init(&ctx);
  while ( (len = read(input_fd, data, SHA1_BLOCK_SIZE)) == SHA1_BLOCK_SIZE)
    SHA1_update(&ctx, data, len);
  if (len != -1)
    SHA1_update(&ctx, data, len);
  p = SHA1_final(&ctx);
  close(input_fd);

  digest = (uint8_t*) malloc(SHA1_DIGEST_SIZE);
  if (!digest)
    return NULL;
  for (i=0; i < SHA1_DIGEST_SIZE; i++)
    digest[i] = *p++;

  return digest;
}

uint8_t* SHA256_file(char *input_file) {
  int i, input_fd, len;
  uint8_t data[SHA256_BLOCK_SIZE], *digest = NULL, *p = NULL;
  SHA256_CTX ctx;

  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    fprintf(stderr, "Couldn't open input file.\n");
    return NULL;
  }

  /* Calculate SHA256 hash of file, reading one block at a time. */
  SHA256_init(&ctx);
  while ( (len = read(input_fd, data, SHA256_BLOCK_SIZE)) == SHA256_BLOCK_SIZE)
    SHA256_update(&ctx, data, len);
  if (len != -1)
    SHA256_update(&ctx, data, len);
  p = SHA256_final(&ctx);
  close(input_fd);

  digest = (uint8_t*) malloc(SHA256_DIGEST_SIZE);
  if (!digest)
    return NULL;
  for (i=0; i < SHA256_DIGEST_SIZE; i++)
    digest[i] = *p++;

  return digest;
}

uint8_t* SHA512_file(char* input_file) {
  int input_fd;
  uint8_t data[SHA512_BLOCK_SIZE], *digest = NULL, *p = NULL;
  int i, len;
  SHA512_CTX ctx;

  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    fprintf(stderr, "Couldn't open input file.\n");
    return NULL;
  }

  /* Calculate SHA512 hash of file, reading one block at a time. */
  SHA512_init(&ctx);
  while ( (len = read(input_fd, data, SHA512_BLOCK_SIZE)) == SHA512_BLOCK_SIZE)
    SHA512_update(&ctx, data, len);
  if (len != -1)
    SHA512_update(&ctx, data, len);
  p = SHA512_final(&ctx);
  close(input_fd);

  digest = (uint8_t*) malloc(SHA512_DIGEST_SIZE);
  if (!digest)
    return NULL;
  for (i=0; i < SHA512_DIGEST_SIZE; i++)
    digest[i] = *p++;

  return digest;
}


uint8_t* calculate_digest(char *input_file, int algorithm) {
  typedef uint8_t* (*Hash_file_ptr) (char*);
  Hash_file_ptr hash_file[] = {
    SHA1_file, /* RSA 1024 */
    SHA256_file,
    SHA512_file,
    SHA1_file, /* RSA 2048 */
    SHA256_file,
    SHA512_file,
    SHA1_file, /* RSA 4096 */
    SHA256_file,
    SHA512_file,
    SHA1_file, /* RSA 8192 */
    SHA256_file,
    SHA512_file,
  };
  return hash_file[algorithm](input_file);
}

uint8_t* read_signature(char *input_file, int len) {
  int i, sigfd;
  uint8_t *signature = NULL;
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
  uint8_t *digest = NULL, *signature = NULL;
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
  sig_len = siglen_map[algorithm] * sizeof(uint32_t);

  if (!(key = read_RSAkey(argv[2], sig_len)))
    goto failure;
  if (!(signature = read_signature(argv[3], sig_len)))
    goto failure;
  if (!(digest = calculate_digest(argv[4], algorithm)))
    goto failure;
  if(RSA_verify(key, signature, sig_len, algorithm, digest))
    fprintf(stderr, "Signature Verification SUCCEEDED.\n");
  else
    fprintf(stderr, "Signature Verification FAILED!\n");

failure:
  free(key);
  free(signature);
  free(digest);

  return 0;
}
