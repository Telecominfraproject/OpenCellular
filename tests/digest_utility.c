/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for message digest functions.
 */

#include "digest_utility.h"
#include "sha.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

uint8_t* SHA1_file(char* input_file) {
  int input_fd, len;
  uint8_t data[ SHA1_BLOCK_SIZE], *digest = NULL, *p = NULL;
  SHA1_CTX ctx;
  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    fprintf(stderr, "Couldn't open input file.\n");
    return NULL;
  }
  SHA1_init(&ctx);
  while ( (len = read(input_fd, data, SHA1_BLOCK_SIZE)) ==
          SHA1_BLOCK_SIZE)
    SHA1_update(&ctx, data, len);
  if (len != -1)
    SHA1_update(&ctx, data, len);
  p = SHA1_final(&ctx);
  close(input_fd);
  digest = (uint8_t*) malloc(SHA1_DIGEST_SIZE);
  if (!digest)
    return NULL;
  memcpy(digest, p, SHA1_DIGEST_SIZE);
  return digest;
}

uint8_t* SHA256_file(char* input_file) {
  int input_fd, len;
  uint8_t data[ SHA256_BLOCK_SIZE], *digest = NULL, *p = NULL;
  SHA256_CTX ctx;
  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    fprintf(stderr, "Couldn't open input file.\n");
    return NULL;
  }
  SHA256_init(&ctx);
  while ( (len = read(input_fd, data, SHA256_BLOCK_SIZE)) ==
          SHA256_BLOCK_SIZE)
    SHA256_update(&ctx, data, len);
  if (len != -1)
    SHA256_update(&ctx, data, len);
  p = SHA256_final(&ctx);
  close(input_fd);
  digest = (uint8_t*) malloc(SHA256_DIGEST_SIZE);
  if (!digest)
    return NULL;
  memcpy(digest, p, SHA256_DIGEST_SIZE);
  return digest;
}

uint8_t* SHA512_file(char* input_file) {
  int input_fd, len;
  uint8_t data[ SHA512_BLOCK_SIZE], *digest = NULL, *p = NULL;
  SHA512_CTX ctx;
  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    fprintf(stderr, "Couldn't open input file.\n");
    return NULL;
  }
  SHA512_init(&ctx);
  while ( (len = read(input_fd, data, SHA512_BLOCK_SIZE)) ==
          SHA512_BLOCK_SIZE)
    SHA512_update(&ctx, data, len);
  if (len != -1)
    SHA512_update(&ctx, data, len);
  p = SHA512_final(&ctx);
  close(input_fd);
  digest = (uint8_t*) malloc(SHA512_DIGEST_SIZE);
  if (!digest)
    return NULL;
  memcpy(digest, p, SHA512_DIGEST_SIZE);
  return digest;
}

uint8_t* calculate_digest(char *input_file, int algorithm) {
  typedef uint8_t* (*Hash_file_ptr) (char*);
  Hash_file_ptr hash_file[] = {
    SHA1_file,  /* RSA 1024 */
    SHA256_file,
    SHA512_file,
    SHA1_file,  /* RSA 2048 */
    SHA256_file,
    SHA512_file,
    SHA1_file,  /* RSA 4096 */
    SHA256_file,
    SHA512_file,
    SHA1_file,  /* RSA 8192 */
    SHA256_file,
    SHA512_file,
  };
  return hash_file[algorithm](input_file);
}
