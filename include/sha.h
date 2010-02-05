/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* SHA-1, 256 and 512 functions. */

#ifndef VBOOT_REFERENCE_SHA_H_
#define VBOOT_REFERENCE_SHA_H_

#include <inttypes.h>
#include <string.h>

#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE 64

#define SHA256_DIGEST_SIZE 32
#define SHA256_BLOCK_SIZE 64

#define SHA512_DIGEST_SIZE 64
#define SHA512_BLOCK_SIZE 128

typedef struct SHA1_CTX {
  uint64_t count;
  uint32_t state[5];
#if defined(HAVE_ENDIAN_H) && defined(HAVE_LITTLE_ENDIAN)
  union {
    uint8_t b[64];
    uint32_t w[16];
  } buf;
#else
  uint8_t buf[64];
#endif
} SHA1_CTX;

typedef struct {
  uint32_t h[8];
  uint32_t tot_len;
  uint32_t len;
  uint8_t block[2 * SHA256_BLOCK_SIZE];
  uint8_t buf[SHA256_DIGEST_SIZE];  /* Used for storing the final digest. */
} SHA256_CTX;

typedef struct {
  uint64_t h[8];
  uint32_t tot_len;
  uint32_t len;
  uint8_t block[2 * SHA512_BLOCK_SIZE];
  uint8_t buf[SHA512_DIGEST_SIZE];  /* Used for storing the final digest. */
} SHA512_CTX;


void SHA1_init(SHA1_CTX* ctx);
void SHA1_update(SHA1_CTX* ctx, const uint8_t* data, int len);
uint8_t* SHA1_final(SHA1_CTX* ctx);

void SHA256_init(SHA256_CTX* ctx);
void SHA256_update(SHA256_CTX* ctx, const uint8_t* data, int len);
uint8_t* SHA256_final(SHA256_CTX* ctx);

void SHA512_init(SHA512_CTX* ctx);
void SHA512_update(SHA512_CTX* ctx, const uint8_t* data, int len);
uint8_t* SHA512_final(SHA512_CTX* ctx);

/* Convenience function for SHA-1.  Computes hash on [data] of length [len].
 * and stores it into [digest]. [digest] should be pre-allocated to
 * SHA1_DIGEST_SIZE bytes.
 */
uint8_t* SHA1(const uint8_t* data, int len, uint8_t* digest);

/* Convenience function for SHA-256.  Computes hash on [data] of length [len].
 * and stores it into [digest]. [digest] should be pre-allocated to
 * SHA256_DIGEST_SIZE bytes.
 */
uint8_t* SHA256(const uint8_t* data, int len, uint8_t* digest);

/* Convenience function for SHA-512.  Computes hash on [data] of length [len].
 * and stores it into [digest]. [digest] should be pre-allocated to
 * SHA512_DIGEST_SIZE bytes.
 */
uint8_t* SHA512(const uint8_t* data, int len, uint8_t* digest);


#endif  /* VBOOT_REFERENCE_SHA_H_ */
