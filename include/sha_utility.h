/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for message digests.
*/

#ifndef VBOOT_REFERENCE_SHA_UTILITY_H_
#define VBOOT_REFERENCE_SHA_UTILITY_H_

#include <inttypes.h>

#include "sha.h"

#define SHA1_DIGEST_ALGORITHM 0
#define SHA256_DIGEST_ALGORITHM 1
#define SHA512_DIGEST_ALGORITHM 2

/* A generic digest context structure which can be used to represent
 * the SHA*_CTX for multiple digest algorithms.
 */
typedef struct DigestContext {
  SHA1_CTX* sha1_ctx;
  SHA256_CTX* sha256_ctx;
  SHA512_CTX* sha512_ctx;
  int algorithm;  /* Hashing algorithm to use. */
} DigestContext;

/* Wrappers for message digest algorithms. These are useful when the hashing
 * operation is being done in parallel with something else. DigestContext tracks
 * and stores the state of any digest algorithm (one at any given time).
 */

/* Initialize a digest context for use with signature algorithm [algorithm]. */
void DigestInit(DigestContext* ctx, int sig_algorithm);
void DigestUpdate(DigestContext* ctx, const uint8_t* data, int len);

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
uint8_t* DigestBuf(uint8_t* buf, int len, int sig_algorithm);

#endif  /* VBOOT_REFERENCE_SHA_UTILITY_H_ */
