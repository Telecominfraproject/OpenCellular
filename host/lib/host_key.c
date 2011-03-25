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
#include <openssl/x509.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "host_key.h"

#include "cryptolib.h"
#include "host_misc.h"
#include "utility.h"
#include "vboot_common.h"


VbPrivateKey* PrivateKeyReadPem(const char* filename, uint64_t algorithm) {

  VbPrivateKey* key;
  RSA* rsa_key;
  FILE* f;

  if (algorithm >= kNumAlgorithms) {
    VBDEBUG(("%s() called with invalid algorithm!\n", __FUNCTION__));
    return NULL;
  }

  /* Read private key */
  f = fopen(filename, "r");
  if (!f) {
    VBDEBUG(("%s(): Couldn't open key file: %s\n", __FUNCTION__, filename));
    return NULL;
  }
  rsa_key = PEM_read_RSAPrivateKey(f, NULL, NULL, NULL);
  fclose(f);
  if (!rsa_key) {
    VBDEBUG(("%s(): Couldn't read private key from file: %s\n", __FUNCTION__,
             filename));
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


/* Write a private key to a file in .vbprivk format. */
int PrivateKeyWrite(const char* filename, const VbPrivateKey* key) {
  uint8_t *outbuf = 0;
  int buflen;
  FILE *f;

  buflen = i2d_RSAPrivateKey(key->rsa_private_key, &outbuf);
  if (buflen <= 0) {
    error("Unable to write private key buffer\n");
    return 1;
  }

  f = fopen(filename, "wb");
  if (!f) {
    error("Unable to open file %s\n", filename);
    Free(outbuf);
    return 1;
  }

  if (1 != fwrite(&key->algorithm, sizeof(key->algorithm), 1, f)) {
    error("Unable to write to file %s\n", filename);
    fclose(f);
    Free(outbuf);
    unlink(filename);  /* Delete any partial file */
  }

  if (1 != fwrite(outbuf, buflen, 1, f)) {
    error("Unable to write to file %s\n", filename);
    fclose(f);
    unlink(filename);  /* Delete any partial file */
    Free(outbuf);
  }

  fclose(f);
  Free(outbuf);
  return 0;
}

VbPrivateKey* PrivateKeyRead(const char* filename) {
  VbPrivateKey *key;
  uint64_t filelen = 0;
  uint8_t *buffer;
  const unsigned char *start;

  buffer = ReadFile(filename, &filelen);
  if (!buffer) {
    error("unable to read from file %s\n", filename);
    return 0;
  }

  key = (VbPrivateKey*)Malloc(sizeof(VbPrivateKey));
  if (!key) {
    error("Unable to allocate VbPrivateKey\n");
    Free(buffer);
    return 0;
  }

  key->algorithm = *(typeof(key->algorithm) *)buffer;
  start = buffer + sizeof(key->algorithm);

  key->rsa_private_key = d2i_RSAPrivateKey(0, &start,
                                           filelen - sizeof(key->algorithm));

  if (!key->rsa_private_key) {
    error("Unable to parse RSA private key\n");
    Free(buffer);
    Free(key);
    return 0;
  }

  Free(buffer);
  return key;
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

VbPublicKey* PublicKeyReadKeyb(const char* filename, uint64_t algorithm,
                               uint64_t version) {
  VbPublicKey* key;
  uint8_t* key_data;
  uint64_t key_size;
  uint64_t expected_key_size;

  if (algorithm >= kNumAlgorithms) {
    VBDEBUG(("PublicKeyReadKeyb() called with invalid algorithm!\n"));
    return NULL;
  }
  if (version > 0xFFFF) {
    /* Currently, TPM only supports 16-bit version */
    VBDEBUG(("PublicKeyReadKeyb() called with invalid version!\n"));
    return NULL;
  }

  key_data = ReadFile(filename, &key_size);
  if (!key_data)
    return NULL;

  if (!RSAProcessedKeySize(algorithm, &expected_key_size) ||
      expected_key_size != key_size) {
    VBDEBUG(("PublicKeyReadKeyb() wrong key size for algorithm\n"));
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
  uint64_t key_size;

  key = (VbPublicKey*)ReadFile(filename, &file_size);
  if (!key)
    return NULL;

  do {
    /* Sanity-check key data */
    if (0 != VerifyPublicKeyInside(key, file_size, key)) {
      VBDEBUG(("PublicKeyRead() not a VbPublicKey\n"));
      break;
    }
    if (key->algorithm >= kNumAlgorithms) {
      VBDEBUG(("PublicKeyRead() invalid algorithm\n"));
      break;
    }
    if (key->key_version > 0xFFFF) {
      VBDEBUG(("PublicKeyRead() invalid version\n"));
      break;  /* Currently, TPM only supports 16-bit version */
    }
    if (!RSAProcessedKeySize(key->algorithm, &key_size) ||
        key_size != key->key_size) {
      VBDEBUG(("PublicKeyRead() wrong key size for algorithm\n"));
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
  VbPublicKey* kcopy;
  int rv;

  /* Copy the key, so its data is contiguous with the header */
  kcopy = PublicKeyAlloc(key->key_size, 0, 0);
  if (!kcopy)
    return 1;
  if (0 != PublicKeyCopy(kcopy, key)) {
    Free(kcopy);
    return 1;
  }

  /* Write the copy, then free it */
  rv = WriteFile(filename, kcopy, kcopy->key_offset + kcopy->key_size);
  Free(kcopy);
  return rv;
}
