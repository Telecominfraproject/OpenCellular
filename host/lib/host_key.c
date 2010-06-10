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
#include "host_misc.h"
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


/* Allocate a new public key with space for a [key_size] byte key. */
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


/* Copy a public key from [src] to [dest].
 *
 * Returns zero if success, non-zero if error. */
int PublicKeyCopy(VbPublicKey* dest, const VbPublicKey* src) {
  if (dest->key_size < src->key_size)
    return 1;

  dest->key_size = src->key_size;
  dest->algorithm = src->algorithm;
  dest->key_version = src->key_version;
  Memcpy(GetPublicKeyData(dest), GetPublicKeyDataC(src), src->key_size);
  return 0;
}


VbPublicKey* PublicKeyReadKeyb(const char* filename, uint64_t algorithm,
                               uint64_t version) {
  VbPublicKey* key;
  uint8_t* key_data;
  uint64_t key_size;

  if (algorithm >= kNumAlgorithms) {
    debug("PublicKeyReadKeyb() called with invalid algorithm!\n");
    return NULL;
  }
  if (version > 0xFFFF) {
    /* Currently, TPM only supports 16-bit version */
    debug("PublicKeyReadKeyb() called with invalid version!\n");
    return NULL;
  }

  key_data = ReadFile(filename, &key_size);
  if (!key_data)
    return NULL;

  if (RSAProcessedKeySize(algorithm) != key_size) {
    debug("PublicKeyReadKeyb() wrong key size for algorithm\n");
    Free(key_data);
    return NULL;
  }

  key = PublicKeyAlloc(key_size, algorithm, version);
  if (!key) {
    Free(key_data);
    return NULL;
  }
  Memcpy(GetPublicKeyData(key), key_data, key_size);

  Free(key_data);
  return key;
}


VbPublicKey* PublicKeyRead(const char* filename) {
  VbPublicKey* key;
  uint64_t file_size;

  key = (VbPublicKey*)ReadFile(filename, &file_size);
  if (!key)
    return NULL;

  do {
    /* Sanity-check key data */
    if (0 != VerifyPublicKeyInside(key, file_size, key)) {
      debug("PublicKeyRead() not a VbPublicKey\n");
      break;
    }
    if (key->algorithm >= kNumAlgorithms) {
      debug("PublicKeyRead() invalid algorithm\n");
      break;
    }
    if (key->key_version > 0xFFFF) {
      debug("PublicKeyRead() invalid version\n");
      break;  /* Currently, TPM only supports 16-bit version */
    }
    if (RSAProcessedKeySize(key->algorithm) != key->key_size) {
      debug("PublicKeyRead() wrong key size for algorithm\n");
      break;
    }

    /* Success */
    return key;

  } while(0);

  /* Error */
  Free(key);
  return NULL;
}


int PublicKeyWrite(const char* filename, const VbPublicKey* key) {
  VbPublicKey* kcopy = NULL;
  FILE* f = NULL;
  int rv = 1;

  do {
    f = fopen(filename, "wb");
    if (!f) {
      debug("PublicKeyWrite() unable to open file %s\n", filename);
      break;
    }

    /* Copy the key, so its data is contiguous with the header */
    kcopy = PublicKeyAlloc(key->key_size, 0, 0);
    if (!kcopy || 0 != PublicKeyCopy(kcopy, key))
      break;

    if (1 != fwrite(kcopy, kcopy->key_offset + kcopy->key_size, 1, f))
      break;

    /* Success */
    rv = 0;

  } while(0);

  if (kcopy)
    Free(kcopy);
  if (f)
    fclose(f);

  if (0 != rv)
    unlink(filename);  /* Delete any partial file */

  return rv;
}
