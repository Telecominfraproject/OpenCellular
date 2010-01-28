/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_RSA_H_
#define VBOOT_REFERENCE_RSA_H_

#include <inttypes.h>

#define RSA1024NUMBYTES 128  /* 1024 bit key length */
#define RSA2048NUMBYTES 256  /* 2048 bit key length */
#define RSA4096NUMBYTES 512  /* 4096 bit key length */
#define RSA8192NUMBYTES 1024  /* 8192 bit key length */

#define RSA1024NUMWORDS (RSA1024NUMBYTES / sizeof(uint32_t))
#define RSA2048NUMWORDS (RSA2048NUMBYTES / sizeof(uint32_t))
#define RSA4096NUMWORDS (RSA4096NUMBYTES / sizeof(uint32_t))
#define RSA8192NUMWORDS (RSA8192NUMBYTES / sizeof(uint32_t))

typedef struct RSAPublicKey {
  int len;  /* Length of n[] in number of uint32_t */
  uint32_t n0inv;  /* -1 / n[0] mod 2^32 */
  uint32_t* n;  /* modulus as little endian array */
  uint32_t* rr; /* R^2 as little endian array */
} RSAPublicKey;

/* Verify a RSA PKCS1.5 signature [sig] of [sig_type] and length [sig_len]
 * against an expected [hash] using [key]. Returns 0 on failure, 1 on success.
 */
int RSA_verify(const RSAPublicKey *key,
               const uint8_t* sig,
               const int sig_len,
               const uint8_t sig_type,
               const uint8_t* hash);

#endif  /* VBOOT_REFERENCE_RSA_H_ */
