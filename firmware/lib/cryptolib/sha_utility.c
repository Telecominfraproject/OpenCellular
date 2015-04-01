/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for message digest functions.
 */

#include "sysincludes.h"

#include "cryptolib.h"
#include "utility.h"
#include "vboot_api.h"

void DigestInit(DigestContext* ctx, int sig_algorithm) {
  ctx->algorithm = hash_type_map[sig_algorithm];
  switch(ctx->algorithm) {
#ifndef CHROMEOS_EC
    case SHA1_DIGEST_ALGORITHM:
      ctx->sha1_ctx = (SHA1_CTX*) VbExMalloc(sizeof(SHA1_CTX));
      SHA1_init(ctx->sha1_ctx);
      break;
#endif
    case SHA256_DIGEST_ALGORITHM:
      ctx->sha256_ctx = (VB_SHA256_CTX*) VbExMalloc(sizeof(VB_SHA256_CTX));
      SHA256_init(ctx->sha256_ctx);
      break;
#ifndef CHROMEOS_EC
    case SHA512_DIGEST_ALGORITHM:
      ctx->sha512_ctx = (VB_SHA512_CTX*) VbExMalloc(sizeof(VB_SHA512_CTX));
      SHA512_init(ctx->sha512_ctx);
      break;
#endif
  };
}

void DigestUpdate(DigestContext* ctx, const uint8_t* data, uint32_t len) {
  switch(ctx->algorithm) {
#ifndef CHROMEOS_EC
    case SHA1_DIGEST_ALGORITHM:
      SHA1_update(ctx->sha1_ctx, data, len);
      break;
#endif
    case SHA256_DIGEST_ALGORITHM:
      SHA256_update(ctx->sha256_ctx, data, len);
      break;
#ifndef CHROMEOS_EC
    case SHA512_DIGEST_ALGORITHM:
      SHA512_update(ctx->sha512_ctx, data, len);
      break;
#endif
  };
}

uint8_t* DigestFinal(DigestContext* ctx) {
  uint8_t* digest = NULL;
  switch(ctx->algorithm) {
#ifndef CHROMEOS_EC
    case SHA1_DIGEST_ALGORITHM:
      digest = (uint8_t*) VbExMalloc(SHA1_DIGEST_SIZE);
      Memcpy(digest, SHA1_final(ctx->sha1_ctx), SHA1_DIGEST_SIZE);
      VbExFree(ctx->sha1_ctx);
      break;
#endif
    case SHA256_DIGEST_ALGORITHM:
      digest = (uint8_t*) VbExMalloc(SHA256_DIGEST_SIZE);
      Memcpy(digest, SHA256_final(ctx->sha256_ctx), SHA256_DIGEST_SIZE);
      VbExFree(ctx->sha256_ctx);
      break;
#ifndef CHROMEOS_EC
    case SHA512_DIGEST_ALGORITHM:
      digest = (uint8_t*) VbExMalloc(SHA512_DIGEST_SIZE);
      Memcpy(digest, SHA512_final(ctx->sha512_ctx), SHA512_DIGEST_SIZE);
      VbExFree(ctx->sha512_ctx);
      break;
#endif
  };
  return digest;
}

uint8_t* DigestBuf(const uint8_t* buf, uint64_t len, int sig_algorithm) {
  /* Allocate enough space for the largest digest */
  uint8_t* digest = (uint8_t*) VbExMalloc(SHA512_DIGEST_SIZE);
  /* Define an array mapping [sig_algorithm] to function pointers to the
   * SHA{1|256|512} functions.
   */
  typedef uint8_t* (*Hash_ptr) (const uint8_t*, uint64_t, uint8_t*);
  Hash_ptr hash[] = {
#ifdef CHROMEOS_EC
    0,  /* RSA 1024 */
    0,
    0,
    0,  /* RSA 2048 */
    0,
    0,
    0,  /* RSA 4096 */
    internal_SHA256,
    0,
    0,  /* RSA 8192 */
    0,
    0,
#else
    internal_SHA1,  /* RSA 1024 */
    internal_SHA256,
    internal_SHA512,
    internal_SHA1,  /* RSA 2048 */
    internal_SHA256,
    internal_SHA512,
    internal_SHA1,  /* RSA 4096 */
    internal_SHA256,
    internal_SHA512,
    internal_SHA1,  /* RSA 8192 */
    internal_SHA256,
    internal_SHA512,
#endif
  };
  /* Call the appropriate hash function. */
  return hash[sig_algorithm](buf, len, digest);
}
