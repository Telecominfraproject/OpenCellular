/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keys.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <openssl/pem.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "host_key.h"
#include "host_misc.h"
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
  key = (VbPrivateKey*)malloc(sizeof(VbPrivateKey));
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
  free(key);
}


/* Write a private key to a file in .vbprivk format. */
int PrivateKeyWrite(const char* filename, const VbPrivateKey* key) {
  uint8_t *outbuf = 0;
  int buflen;
  FILE *f;

  buflen = i2d_RSAPrivateKey(key->rsa_private_key, &outbuf);
  if (buflen <= 0) {
    VbExError("Unable to write private key buffer\n");
    return 1;
  }

  f = fopen(filename, "wb");
  if (!f) {
    VbExError("Unable to open file %s\n", filename);
    free(outbuf);
    return 1;
  }

  if (1 != fwrite(&key->algorithm, sizeof(key->algorithm), 1, f)) {
    VbExError("Unable to write to file %s\n", filename);
    fclose(f);
    free(outbuf);
    unlink(filename);  /* Delete any partial file */
  }

  if (1 != fwrite(outbuf, buflen, 1, f)) {
    VbExError("Unable to write to file %s\n", filename);
    fclose(f);
    unlink(filename);  /* Delete any partial file */
    free(outbuf);
  }

  fclose(f);
  free(outbuf);
  return 0;
}

VbPrivateKey* PrivateKeyRead(const char* filename) {
  VbPrivateKey *key;
  uint64_t filelen = 0;
  uint8_t *buffer;
  const unsigned char *start;

  buffer = ReadFile(filename, &filelen);
  if (!buffer) {
    VbExError("unable to read from file %s\n", filename);
    return 0;
  }

  key = (VbPrivateKey*)malloc(sizeof(VbPrivateKey));
  if (!key) {
    VbExError("Unable to allocate VbPrivateKey\n");
    free(buffer);
    return 0;
  }

  key->algorithm = *(typeof(key->algorithm) *)buffer;
  start = buffer + sizeof(key->algorithm);

  key->rsa_private_key = d2i_RSAPrivateKey(0, &start,
                                           filelen - sizeof(key->algorithm));

  if (!key->rsa_private_key) {
    VbExError("Unable to parse RSA private key\n");
    free(buffer);
    free(key);
    return 0;
  }

  free(buffer);
  return key;
}


/* Allocate a new public key with space for a [key_size] byte key. */
VbPublicKey* PublicKeyAlloc(uint64_t key_size, uint64_t algorithm,
                            uint64_t version) {
  VbPublicKey* key = (VbPublicKey*)malloc(sizeof(VbPublicKey) + key_size);
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
    free(key_data);
    return NULL;
  }

  key = PublicKeyAlloc(key_size, algorithm, version);
  if (!key) {
    free(key_data);
    return NULL;
  }
  Memcpy(GetPublicKeyData(key), key_data, key_size);

  free(key_data);
  return key;
}


int PublicKeyLooksOkay(VbPublicKey *key, uint64_t file_size)
{
  uint64_t key_size;

  /* Sanity-check key data */
  if (0 != VerifyPublicKeyInside(key, file_size, key)) {
    VBDEBUG(("PublicKeyRead() not a VbPublicKey\n"));
    return 0;
  }
  if (key->algorithm >= kNumAlgorithms) {
    VBDEBUG(("PublicKeyRead() invalid algorithm\n"));
    return 0;
  }
  if (key->key_version > 0xFFFF) {
    VBDEBUG(("PublicKeyRead() invalid version\n"));
    return 0;  /* Currently, TPM only supports 16-bit version */
  }
  if (!RSAProcessedKeySize(key->algorithm, &key_size) ||
      key_size != key->key_size) {
    VBDEBUG(("PublicKeyRead() wrong key size for algorithm\n"));
    return 0;
  }

  /* Success */
  return 1;
}



VbPublicKey* PublicKeyRead(const char* filename) {
  VbPublicKey* key;
  uint64_t file_size;

  key = (VbPublicKey*)ReadFile(filename, &file_size);
  if (!key)
    return NULL;

  if (PublicKeyLooksOkay(key, file_size))
      return key;

  /* Error */
  free(key);
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
    free(kcopy);
    return 1;
  }

  /* Write the copy, then free it */
  rv = WriteFile(filename, kcopy, kcopy->key_offset + kcopy->key_size);
  free(kcopy);
  return rv;
}
