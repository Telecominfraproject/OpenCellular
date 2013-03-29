/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Implementation of RSA utility functions.
 */

#include "sysincludes.h"

#include "cryptolib.h"
#include "stateful_util.h"
#include "utility.h"
#include "vboot_api.h"

uint64_t RSAProcessedKeySize(uint64_t algorithm, uint64_t* out_size) {
  int key_len; /* Key length in bytes.  (int type matches siglen_map) */
  if (algorithm < kNumAlgorithms) {
    key_len = siglen_map[algorithm];
    /* Total size needed by a RSAPublicKey buffer is =
     *  2 * key_len bytes for the  n and rr arrays
     *  + sizeof len + sizeof n0inv.
     */
    *out_size = (2 * key_len + sizeof(uint32_t) + sizeof(uint32_t));
    return 1;
  }
  return 0;
}

RSAPublicKey* RSAPublicKeyNew(void) {
  RSAPublicKey* key = (RSAPublicKey*) VbExMalloc(sizeof(RSAPublicKey));
  key->n = NULL;
  key->rr = NULL;
  key->len = 0;
  key->algorithm = kNumAlgorithms;
  return key;
}

void RSAPublicKeyFree(RSAPublicKey* key) {
  if (key) {
    if (key->n)
      VbExFree(key->n);
    if (key->rr)
      VbExFree(key->rr);
    VbExFree(key);
  }
}

RSAPublicKey* RSAPublicKeyFromBuf(const uint8_t* buf, uint64_t len) {
  RSAPublicKey* key = RSAPublicKeyNew();
  MemcpyState st;
  uint64_t key_len;

  StatefulInit(&st, (void*)buf, len);

  StatefulMemcpy(&st, &key->len, sizeof(key->len));
  /* key length in bytes (avoiding possible 32-bit rollover) */
  key_len = key->len;
  key_len *= sizeof(uint32_t);

  /* Sanity Check the key length. */
  if (RSA1024NUMBYTES != key_len &&
      RSA2048NUMBYTES != key_len &&
      RSA4096NUMBYTES != key_len &&
      RSA8192NUMBYTES != key_len) {
    RSAPublicKeyFree(key);
    return NULL;
  }

  key->n = (uint32_t*) VbExMalloc(key_len);
  key->rr = (uint32_t*) VbExMalloc(key_len);

  StatefulMemcpy(&st, &key->n0inv, sizeof(key->n0inv));
  StatefulMemcpy(&st, key->n, key_len);
  StatefulMemcpy(&st, key->rr, key_len);
  if (st.overrun || st.remaining_len != 0) {  /* Underrun or overrun. */
    RSAPublicKeyFree(key);
    return NULL;
  }

  return key;
}

int RSAVerifyBinary_f(const uint8_t* key_blob,
                      const RSAPublicKey* key,
                      const uint8_t* buf,
                      uint64_t len,
                      const uint8_t* sig,
                      unsigned int algorithm) {
  RSAPublicKey* verification_key = NULL;
  uint8_t* digest = NULL;
  uint64_t key_size;
  int sig_size;
  int success;

  if (algorithm >= (unsigned int)kNumAlgorithms)
    return 0;  /* Invalid algorithm. */
  if (!RSAProcessedKeySize(algorithm, &key_size))
    return 0;
  sig_size = siglen_map[algorithm];

  if (key_blob && !key)
    verification_key = RSAPublicKeyFromBuf(key_blob, key_size);
  else if (!key_blob && key)
    verification_key = (RSAPublicKey*) key;  /* Supress const warning. */
  else
    return 0; /* Both can't be NULL or non-NULL. */

  /* Ensure we have a valid key. */
  if (!verification_key)
    return 0;

  digest = DigestBuf(buf, len, algorithm);
  success = RSAVerify(verification_key, sig, (uint32_t)sig_size,
                      (uint8_t)algorithm, digest);

  VbExFree(digest);
  if (!key)
    RSAPublicKeyFree(verification_key);  /* Only free if we allocated it. */
  return success;
}

/* Version of RSAVerifyBinary_f() where instead of the raw binary blob
 * of data, its digest is passed as the argument. */
int RSAVerifyBinaryWithDigest_f(const uint8_t* key_blob,
                                const RSAPublicKey* key,
                                const uint8_t* digest,
                                const uint8_t* sig,
                                unsigned int algorithm) {
  RSAPublicKey* verification_key = NULL;
  uint64_t key_size;
  int sig_size;
  int success;

  if (algorithm >= (unsigned int)kNumAlgorithms)
    return 0;  /* Invalid algorithm. */
  if (!RSAProcessedKeySize(algorithm, &key_size))
    return 0;
  sig_size = siglen_map[algorithm];

  if (key_blob && !key)
    verification_key = RSAPublicKeyFromBuf(key_blob, key_size);
  else if (!key_blob && key)
    verification_key = (RSAPublicKey*) key;  /* Supress const warning. */
  else
    return 0; /* Both can't be NULL or non-NULL. */

  /* Ensure we have a valid key. */
  if (!verification_key)
    return 0;

  success = RSAVerify(verification_key, sig, (uint32_t)sig_size,
                      (uint8_t)algorithm, digest);

  if (!key)
    RSAPublicKeyFree(verification_key);  /* Only free if we allocated it. */
  return success;
}
