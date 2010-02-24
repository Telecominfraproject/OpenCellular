/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for message digest functions.
 */

#include "padding.h"
#include "rsa_utility.h"
#include "sha_utility.h"
#include "utility.h"

int RSAProcessedKeySize(int algorithm) {
  int key_len = siglen_map[algorithm] * sizeof(uint32_t);  /* Key length in
                                                            * bytes. */
  /* Total size needed by a RSAPublicKey structure is =
   *  2 * key_len bytes for the  n and rr arrays
   *  + sizeof len + sizeof n0inv.
   */
  return (2 * key_len + sizeof(int) + sizeof(uint32_t));
}

void RSAPublicKeyFree(RSAPublicKey* key) {
  if (key) {
    Free(key->n);
    Free(key->rr);
    Free(key);
  }
}

RSAPublicKey* RSAPublicKeyFromBuf(const uint8_t* buf, int len) {
  RSAPublicKey* key = (RSAPublicKey*) Malloc(sizeof(RSAPublicKey));
  MemcpyState st;
  int key_len;

  st.remaining_buf = (uint8_t*) buf;
  st.remaining_len = len;

  StatefulMemcpy(&st, &key->len, sizeof(key->len));
  key_len = key->len * sizeof(uint32_t);  /* key length in bytes. */
  key->n = (uint32_t*) Malloc(key_len);
  key->rr = (uint32_t*) Malloc(key_len);

  StatefulMemcpy(&st, &key->n0inv, sizeof(key->n0inv));
  StatefulMemcpy(&st, key->n, key_len);
  StatefulMemcpy(&st, key->rr, key_len);
  if (st.remaining_len != 0) {  /* Underrun or overrun. */
    Free(key->n);
    Free(key->rr);
    Free(key);
    return NULL;
  }

  return key;
}

int RSAVerifyBinary_f(const uint8_t* key_blob,
                      const RSAPublicKey* key,
                      const uint8_t* buf,
                      int len,
                      const uint8_t* sig,
                      int algorithm) {
  RSAPublicKey* verification_key = NULL;
  uint8_t* digest = NULL;
  int key_size;
  int sig_size;
  int success;

  if (algorithm >= kNumAlgorithms)
    return 0;  /* Invalid algorithm. */
  key_size = RSAProcessedKeySize(algorithm);
  sig_size = siglen_map[algorithm] * sizeof(uint32_t);

  if (key_blob && !key)
    verification_key = RSAPublicKeyFromBuf(key_blob, key_size);
  else if (!key_blob && key)
    verification_key = (RSAPublicKey*) key;  /* Supress const warning. */
  else
    return 0; /* Both can't be NULL or non-NULL. */

  digest = DigestBuf(buf, len, algorithm);
  success = RSA_verify(verification_key, sig, sig_size, algorithm, digest);

  Free(digest);
  if (!key)
    RSAPublicKeyFree(verification_key);  /* Only free if we allocated it. */
  return success;
}
