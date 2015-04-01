/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* SHA-1, 256 and 512 functions. */

#ifndef VBOOT_REFERENCE_SHA_H_
#define VBOOT_REFERENCE_SHA_H_

#ifndef VBOOT_REFERENCE_CRYPTOLIB_H_
#error "Do not include this file directly. Use cryptolib.h instead."
#endif

#include "sysincludes.h"

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
} VB_SHA256_CTX;

typedef struct {
  uint64_t h[8];
  uint32_t tot_len;
  uint32_t len;
  uint8_t block[2 * SHA512_BLOCK_SIZE];
  uint8_t buf[SHA512_DIGEST_SIZE];  /* Used for storing the final digest. */
} VB_SHA512_CTX;


void SHA1_init(SHA1_CTX* ctx);
void SHA1_update(SHA1_CTX* ctx, const uint8_t* data, uint64_t len);
uint8_t* SHA1_final(SHA1_CTX* ctx);

void SHA256_init(VB_SHA256_CTX* ctx);
void SHA256_update(VB_SHA256_CTX* ctx, const uint8_t* data, uint32_t len);
uint8_t* SHA256_final(VB_SHA256_CTX* ctx);

void SHA512_init(VB_SHA512_CTX* ctx);
void SHA512_update(VB_SHA512_CTX* ctx, const uint8_t* data, uint32_t len);
uint8_t* SHA512_final(VB_SHA512_CTX* ctx);

/* Convenience function for SHA-1.  Computes hash on [data] of length [len].
 * and stores it into [digest]. [digest] should be pre-allocated to
 * SHA1_DIGEST_SIZE bytes.
 */
uint8_t* internal_SHA1(const uint8_t* data, uint64_t len, uint8_t* digest);

/* Convenience function for SHA-256.  Computes hash on [data] of length [len].
 * and stores it into [digest]. [digest] should be pre-allocated to
 * SHA256_DIGEST_SIZE bytes.
 */
uint8_t* internal_SHA256(const uint8_t* data, uint64_t len, uint8_t* digest);

/* Convenience function for SHA-512.  Computes hash on [data] of length [len].
 * and stores it into [digest]. [digest] should be pre-allocated to
 * SHA512_DIGEST_SIZE bytes.
 */
uint8_t* internal_SHA512(const uint8_t* data, uint64_t len, uint8_t* digest);


/*---- Utility functions/wrappers for message digests. */

#define SHA1_DIGEST_ALGORITHM 0
#define SHA256_DIGEST_ALGORITHM 1
#define SHA512_DIGEST_ALGORITHM 2

/* A generic digest context structure which can be used to represent
 * the SHA*_CTX for multiple digest algorithms.
 */
typedef struct DigestContext {
  SHA1_CTX* sha1_ctx;
  VB_SHA256_CTX* sha256_ctx;
  VB_SHA512_CTX* sha512_ctx;
  int algorithm;  /* Hashing algorithm to use. */
} DigestContext;

/* Wrappers for message digest algorithms. These are useful when the hashing
 * operation is being done in parallel with something else. DigestContext tracks
 * and stores the state of any digest algorithm (one at any given time).
 */

/* Initialize a digest context for use with signature algorithm [algorithm]. */
void DigestInit(DigestContext* ctx, int sig_algorithm);
void DigestUpdate(DigestContext* ctx, const uint8_t* data, uint32_t len);

/* Caller owns the returned digest and must free it. */
uint8_t* DigestFinal(DigestContext* ctx);

/* Returns the appropriate digest for the data in [input_file]
 * based on the signature [algorithm].
 * Caller owns the returned digest and must free it.
 */
uint8_t* DigestFile(char* input_file, int sig_algorithm);

/* Returns the appropriate digest of [buf] of length
 * [len] based on the signature [algorithm].
 * Caller owns the returned digest and must free it.
 */
uint8_t* DigestBuf(const uint8_t* buf, uint64_t len, int sig_algorithm);


#endif  /* VBOOT_REFERENCE_SHA_H_ */
