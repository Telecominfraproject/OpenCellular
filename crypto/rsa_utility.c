/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility functions for message digest functions.
 */

#include "padding.h"
#include "rsa_utility.h"
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

RSAPublicKey* RSAPublicKeyFromBuf(uint8_t* buf, int len) {
  RSAPublicKey* key = (RSAPublicKey*) Malloc(sizeof(RSAPublicKey));
  MemcpyState st;
  int key_len;

  st.remaining_buf = buf;
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
