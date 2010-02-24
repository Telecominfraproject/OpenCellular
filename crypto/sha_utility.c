/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for message digest functions.
 */

#include "sha_utility.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sha.h"
#include "utility.h"

int digest_type_map[] = {
  SHA1_DIGEST_ALGORITHM,  /* RSA 1024 */
  SHA256_DIGEST_ALGORITHM,
  SHA512_DIGEST_ALGORITHM,
  SHA1_DIGEST_ALGORITHM,  /* RSA 2048 */
  SHA256_DIGEST_ALGORITHM,
  SHA512_DIGEST_ALGORITHM,
  SHA1_DIGEST_ALGORITHM,  /* RSA 4096 */
  SHA256_DIGEST_ALGORITHM,
  SHA512_DIGEST_ALGORITHM,
  SHA1_DIGEST_ALGORITHM,  /* RSA 8192 */
  SHA256_DIGEST_ALGORITHM,
  SHA512_DIGEST_ALGORITHM,
};

void DigestInit(DigestContext* ctx, int sig_algorithm) {
  ctx->algorithm = digest_type_map[sig_algorithm];
  switch(ctx->algorithm) {
    case SHA1_DIGEST_ALGORITHM:
      ctx->sha1_ctx = (SHA1_CTX*) Malloc(sizeof(SHA1_CTX));
      SHA1_init(ctx->sha1_ctx);
      break;
    case SHA256_DIGEST_ALGORITHM:
      ctx->sha256_ctx = (SHA256_CTX*) Malloc(sizeof(SHA256_CTX));
      SHA256_init(ctx->sha256_ctx);
      break;
    case SHA512_DIGEST_ALGORITHM:
      ctx->sha512_ctx = (SHA512_CTX*) Malloc(sizeof(SHA512_CTX));
      SHA512_init(ctx->sha512_ctx);
      break;
  };
}

void DigestUpdate(DigestContext* ctx, const uint8_t* data, int len) {
  switch(ctx->algorithm) {
    case SHA1_DIGEST_ALGORITHM:
      SHA1_update(ctx->sha1_ctx, data, len);
      break;
    case SHA256_DIGEST_ALGORITHM:
      SHA256_update(ctx->sha256_ctx, data, len);
      break;
    case SHA512_DIGEST_ALGORITHM:
      SHA512_update(ctx->sha512_ctx, data, len);
      break;
  };
}

uint8_t* DigestFinal(DigestContext* ctx) {
  uint8_t* digest = NULL;
  switch(ctx->algorithm) {
    case SHA1_DIGEST_ALGORITHM:
      digest = (uint8_t*) Malloc(SHA1_DIGEST_SIZE);
      Memcpy(digest, SHA1_final(ctx->sha1_ctx), SHA1_DIGEST_SIZE);
      Free(ctx->sha1_ctx);
      break;
    case SHA256_DIGEST_ALGORITHM:
      digest = (uint8_t*) Malloc(SHA256_DIGEST_SIZE);
      Memcpy(digest, SHA256_final(ctx->sha256_ctx), SHA256_DIGEST_SIZE);
      Free(ctx->sha256_ctx);
      break;
    case SHA512_DIGEST_ALGORITHM:
      digest = (uint8_t*) Malloc(SHA512_DIGEST_SIZE);
      Memcpy(digest, SHA512_final(ctx->sha512_ctx), SHA512_DIGEST_SIZE);
      Free(ctx->sha512_ctx);
      break;
  };
  return digest;
}

uint8_t* DigestFile(char* input_file, int sig_algorithm) {
  int input_fd, len;
  uint8_t data[SHA1_BLOCK_SIZE];
  uint8_t* digest = NULL;
  DigestContext ctx;

  if( (input_fd = open(input_file, O_RDONLY)) == -1 ) {
    fprintf(stderr, "Couldn't open input file.\n");
    return NULL;
  }
  DigestInit(&ctx, sig_algorithm);
  while ( (len = read(input_fd, data, SHA1_BLOCK_SIZE)) ==
          SHA1_BLOCK_SIZE)
    DigestUpdate(&ctx, data, len);
  if (len != -1)
    DigestUpdate(&ctx, data, len);
  digest = DigestFinal(&ctx);
  close(input_fd);
  return digest;
}

uint8_t* DigestBuf(const uint8_t* buf, int len, int sig_algorithm) {
  uint8_t* digest = (uint8_t*) Malloc(SHA512_DIGEST_SIZE); /* Use the max. */
  /* Define an array mapping [sig_algorithm] to function pointers to the
   * SHA{1|256|512} functions.
   */
  typedef uint8_t* (*Hash_ptr) (const uint8_t*, int, uint8_t*);
  Hash_ptr hash[] = {
    SHA1,  /* RSA 1024 */
    SHA256,
    SHA512,
    SHA1,  /* RSA 2048 */
    SHA256,
    SHA512,
    SHA1,  /* RSA 4096 */
    SHA256,
    SHA512,
    SHA1,  /* RSA 8192 */
    SHA256,
    SHA512,
  };
  /* Call the appropriate hash function. */
  return hash[sig_algorithm](buf, len, digest);
}
