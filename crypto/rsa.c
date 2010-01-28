/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Implementation of RSA signature verification which uses a pre-processed
 * key for computation. The code extends Android's RSA verification code to
 * support multiple RSA key lengths and hash digest algorithms.
 */

#include <stdio.h>

#include "padding.h"
#include "rsa.h"
#include "utility.h"

/* a[] -= mod */
static void subM(const RSAPublicKey *key, uint32_t *a) {
  int64_t A = 0;
  int i;
  for (i = 0; i < key->len; ++i) {
    A += (uint64_t)a[i] - key->n[i];
    a[i] = (uint32_t)A;
    A >>= 32;
  }
}

/* return a[] >= mod */
static int geM(const RSAPublicKey *key, uint32_t *a) {
  int i;
  for (i = key->len; i;) {
    --i;
    if (a[i] < key->n[i]) return 0;
    if (a[i] > key->n[i]) return 1;
  }
  return 1;  /* equal */
 }

/* montgomery c[] += a * b[] / R % mod */
static void montMulAdd(const RSAPublicKey *key,
                       uint32_t* c,
                       const uint32_t a,
                       const uint32_t* b) {
  uint64_t A = (uint64_t)a * b[0] + c[0];
  uint32_t d0 = (uint32_t)A * key->n0inv;
  uint64_t B = (uint64_t)d0 * key->n[0] + (uint32_t)A;
  int i;

  for (i = 1; i < key->len; ++i) {
    A = (A >> 32) + (uint64_t)a * b[i] + c[i];
    B = (B >> 32) + (uint64_t)d0 * key->n[i] + (uint32_t)A;
    c[i - 1] = (uint32_t)B;
  }

  A = (A >> 32) + (B >> 32);

  c[i - 1] = (uint32_t)A;

  if (A >> 32) {
    subM(key, c);
  }
}

/* montgomery c[] = a[] * b[] / R % mod */
static void montMul(const RSAPublicKey *key,
                    uint32_t* c,
                    uint32_t* a,
                    uint32_t* b) {
  int i;
  for (i = 0; i < key->len; ++i) {
    c[i] = 0;
  }
  for (i = 0; i < key->len; ++i) {
    montMulAdd(key, c, a[i], b);
  }
}

/* In-place public exponentiation. (65537}
 * Input and output big-endian byte array in inout.
 */
static void modpowF4(const RSAPublicKey *key,
                    uint8_t* inout) {
  uint32_t* a = (uint32_t*) Malloc(key->len * sizeof(uint32_t));
  uint32_t* aR = (uint32_t*) Malloc(key->len * sizeof(uint32_t));
  uint32_t* aaR = (uint32_t*) Malloc(key->len * sizeof(uint32_t));

  uint32_t* aaa = aaR;  /* Re-use location. */
  int i;

  /* Convert from big endian byte array to little endian word array. */
  for (i = 0; i < key->len; ++i) {
    uint32_t tmp =
        (inout[((key->len - 1 - i) * 4) + 0] << 24) |
        (inout[((key->len - 1 - i) * 4) + 1] << 16) |
        (inout[((key->len - 1 - i) * 4) + 2] << 8) |
        (inout[((key->len - 1 - i) * 4) + 3] << 0);
    a[i] = tmp;
  }

  montMul(key, aR, a, key->rr);  /* aR = a * RR / R mod M   */
  for (i = 0; i < 16; i+=2) {
    montMul(key, aaR, aR, aR);  /* aaR = aR * aR / R mod M */
    montMul(key, aR, aaR, aaR);  /* aR = aaR * aaR / R mod M */
  }
  montMul(key, aaa, aR, a);  /* aaa = aR * a / R mod M */


  /* Make sure aaa < mod; aaa is at most 1x mod too large. */
  if (geM(key, aaa)) {
    subM(key, aaa);
  }

  /* Convert to bigendian byte array */
  for (i = key->len - 1; i >= 0; --i) {
    uint32_t tmp = aaa[i];
    *inout++ = tmp >> 24;
    *inout++ = tmp >> 16;
    *inout++ = tmp >> 8;
    *inout++ = tmp >> 0;
  }

  Free(a);
  Free(aR);
  Free(aaR);
}

/* Verify a RSA PKCS1.5 signature against an expected hash.
 * Returns 0 on failure, 1 on success.
 */
int RSA_verify(const RSAPublicKey *key,
               const uint8_t *sig,
               const int sig_len,
               const uint8_t sig_type,
               const uint8_t *hash) {
  int i;
  uint8_t* buf;
  const uint8_t* padding;
  int success = 1;

  if (sig_len != (key->len * sizeof(uint32_t))) {
    fprintf(stderr, "Signature is of incorrect length!\n");
    return 0;
  }

  if (sig_type >= kNumAlgorithms) {
    fprintf(stderr, "Invalid signature type!\n");
    return 0;
  }

  if (key->len != siglen_map[sig_type]) {
    fprintf(stderr, "Wrong key passed in!\n");
    return 0;
  }

  buf = (uint8_t*) Malloc(sig_len);
  Memcpy(buf, sig, sig_len);

  modpowF4(key, buf);

  /* Determine padding to use depending on the signature type. */
  padding = padding_map[sig_type];

  /* Check pkcs1.5 padding bytes. */
  for (i = 0; i < padding_size_map[sig_type]; ++i) {
    if (buf[i] != padding[i]) {
#ifndef NDEBUG
/* TODO(gauravsh): Replace with a macro call for logging. */
      fprintf(stderr, "Padding: Expecting = %02x Got = %02x\n", padding[i],
              buf[i]);
#endif
      success = 0;
    }
  }

  /* Check if digest matches. */
  for (; i < sig_len; ++i) {
    if (buf[i] != *hash++) {
#ifndef NDEBUG
/* TODO(gauravsh): Replace with a macro call for logging. */
      fprintf(stderr, "Digest: Expecting = %02x Got = %02x\n", padding[i],
              buf[i]);
#endif
      success = 0;
    }
  }

  Free(buf);

  return success;
}
