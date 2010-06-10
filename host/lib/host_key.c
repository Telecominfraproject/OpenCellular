/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keys.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#define OPENSSL_NO_SHA
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "host_key.h"

#include "cryptolib.h"
#include "file_keys.h"
#include "utility.h"
#include "vboot_common.h"


VbPrivateKey* PrivateKeyRead(const char* filename, uint64_t algorithm) {

  VbPrivateKey* key;
  RSA* rsa_key;
  FILE* f;

  if (algorithm >= kNumAlgorithms) {
    debug("PrivateKeyRead() called with invalid algorithm!\n");
    return NULL;
  }

  /* Read private key */
  f = fopen(filename, "r");
  if (!f) {
    debug("PrivateKeyRead(): Couldn't open key file: %s\n", filename);
    return NULL;
  }
  rsa_key = PEM_read_RSAPrivateKey(f, NULL, NULL, NULL);
  fclose(f);
  if (!rsa_key) {
    debug("PrivateKeyRead(): Couldn't read private key from file: %s\n",
          filename);
    return NULL;
  }

  /* Store key and algorithm in our struct */
  key = (VbPrivateKey*)Malloc(sizeof(VbPrivateKey));
  if (!key) {
    RSA_free(rsa_key);
    return NULL;
  }
  key->rsa_private_key = rsa_key;
  key->algorithm = algorithm;

  /* Return the key */
  return key;
}


void PrivateKeyFree(VbPrivateKey* key) {
  if (!key)
    return;
  if (key->rsa_private_key)
    RSA_free(key->rsa_private_key);
  Free(key);
}


void PublicKeyInit(VbPublicKey* key, uint8_t* key_data, uint64_t key_size) {
  key->key_offset = OffsetOf(key, key_data);
  key->key_size = key_size;
  key->algorithm = kNumAlgorithms; /* Key not present yet */
  key->key_version = 0;
}


VbPublicKey* PublicKeyAlloc(uint64_t key_size, uint64_t algorithm,
                            uint64_t version) {
  VbPublicKey* key = (VbPublicKey*)Malloc(sizeof(VbPublicKey) + key_size);
  if (!key)
    return NULL;

  key->algorithm = algorithm;
  key->key_version = version;
  key->key_size = key_size;
  key->key_offset = sizeof(VbPublicKey);
  return key;
}


int PublicKeyCopy(VbPublicKey* dest, const VbPublicKey* src) {
  if (dest->key_size < src->key_size)
    return 1;

  dest->key_size = src->key_size;
  dest->algorithm = src->algorithm;
  dest->key_version = src->key_version;
  Memcpy(GetPublicKeyData(dest), GetPublicKeyDataC(src), src->key_size);
  return 0;
}


VbPublicKey* PublicKeyRead(const char* filename, uint64_t algorithm,
                           uint64_t version) {

  VbPublicKey* key;
  uint8_t* key_data;
  uint64_t key_size;

  if (algorithm >= kNumAlgorithms) {
    debug("PublicKeyRead() called with invalid algorithm!\n");
    return NULL;
  }
  if (version > 0xFFFF) {
    /* Currently, TPM only supports 16-bit version */
    debug("PublicKeyRead() called with invalid version!\n");
    return NULL;
  }

  key_data = BufferFromFile(filename, &key_size);
  if (!key_data)
    return NULL;

  /* TODO: sanity-check key length based on algorithm */

  key = PublicKeyAlloc(key_size, algorithm, version);
  if (!key) {
    Free(key_data);
    return NULL;
  }
  Memcpy(GetPublicKeyData(key), key_data, key_size);

  Free(key_data);
  return key;
}
