/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef VBOOT_REFERENCE_RSA_H_
#define VBOOT_REFERENCE_RSA_H_

#ifndef VBOOT_REFERENCE_CRYPTOLIB_H_
#error "Do not include this file directly. Use cryptolib.h instead."
#endif

#include "sysincludes.h"

#define RSA1024NUMBYTES 128  /* 1024 bit key length */
#define RSA2048NUMBYTES 256  /* 2048 bit key length */
#define RSA4096NUMBYTES 512  /* 4096 bit key length */
#define RSA8192NUMBYTES 1024  /* 8192 bit key length */

#define RSA1024NUMWORDS (RSA1024NUMBYTES / sizeof(uint32_t))
#define RSA2048NUMWORDS (RSA2048NUMBYTES / sizeof(uint32_t))
#define RSA4096NUMWORDS (RSA4096NUMBYTES / sizeof(uint32_t))
#define RSA8192NUMWORDS (RSA8192NUMBYTES / sizeof(uint32_t))

typedef struct RSAPublicKey {
  uint32_t len;  /* Length of n[] in number of uint32_t */
  uint32_t n0inv;  /* -1 / n[0] mod 2^32 */
  uint32_t* n;  /* modulus as little endian array */
  uint32_t* rr; /* R^2 as little endian array */
  unsigned int algorithm; /* Algorithm to use when verifying with the key */
} RSAPublicKey;

/* Verify a RSA PKCS1.5 signature [sig] of [sig_type] and length [sig_len]
 * against an expected [hash] using [key]. Returns 0 on failure, 1 on success.
 */
int RSAVerify(const RSAPublicKey *key,
              const uint8_t* sig,
              const uint32_t sig_len,
              const uint8_t sig_type,
              const uint8_t* hash);

/* Perform RSA signature verification on [buf] of length [len] against expected
 * signature [sig] using signature algorithm [algorithm]. The public key used
 * for verification can either be in the form of a pre-process key blob
 * [key_blob] or RSAPublicKey structure [key]. One of [key_blob] or [key] must
 * be non-NULL, and the other NULL or the function will fail.
 *
 * Returns 1 on verification success, 0 on verification failure or invalid
 * arguments.
 *
 * Note: This function is for use in the firmware and assumes all pointers point
 * to areas in the memory of the right size.
 *
 */
int RSAVerifyBinary_f(const uint8_t* key_blob,
                      const RSAPublicKey* key,
                      const uint8_t* buf,
                      uint64_t len,
                      const uint8_t* sig,
                      unsigned int algorithm);

/* Version of RSAVerifyBinary_f() where instead of the raw binary blob
 * of data, its digest is passed as the argument. */
int RSAVerifyBinaryWithDigest_f(const uint8_t* key_blob,
                                const RSAPublicKey* key,
                                const uint8_t* digest,
                                const uint8_t* sig,
                                unsigned int algorithm);


/* ----Some additional utility functions for RSA.---- */

/* Returns the size of a pre-processed RSA public key in
 * [out_size] with the algorithm [algorithm].
 *
 * Returns 1 on success, 0 on failure.
 */
uint64_t RSAProcessedKeySize(uint64_t algorithm, uint64_t* out_size);

/* Allocate a new RSAPublicKey structure and initialize its pointer fields to
 * NULL */
RSAPublicKey* RSAPublicKeyNew(void);

/* Deep free the contents of [key]. */
void RSAPublicKeyFree(RSAPublicKey* key);

/* Create a RSAPublic key structure from binary blob [buf] of length
 * [len].
 *
 * Caller owns the returned key and must free it.
 */
RSAPublicKey* RSAPublicKeyFromBuf(const uint8_t* buf, uint64_t len);


#endif  /* VBOOT_REFERENCE_RSA_H_ */
