/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_KEY_H_
#define VBOOT_REFERENCE_HOST_KEY_H_

#include "cryptolib.h"
#include "vboot_struct.h"


typedef struct rsa_st RSA;

/* Private key data */
typedef struct VbPrivateKey {
  RSA* rsa_private_key;  /* Private key data */
  uint64_t algorithm;    /* Algorithm to use when signing */
} VbPrivateKey;


/* Read a private key from a .pem file.  Caller owns the returned pointer,
 * and must free it with PrivateKeyFree(). */
VbPrivateKey* PrivateKeyReadPem(const char* filename, uint64_t algorithm);


/* Free a private key. */
void PrivateKeyFree(VbPrivateKey* key);

/* Write a private key to a file in .vbprivk format. */
int PrivateKeyWrite(const char* filename, const VbPrivateKey* key);

/* Read a privake key from a .vbprivk file.  Caller owns the returned
 * pointer, and must free it with PrivateKeyFree().
 *
 * Returns NULL if error. */
VbPrivateKey* PrivateKeyRead(const char* filename);



/* Allocate a new public key with space for a [key_size] byte key. */
VbPublicKey* PublicKeyAlloc(uint64_t key_size, uint64_t algorithm,
                            uint64_t version);


/* Read a public key from a .vbpubk file.  Caller owns the returned
 * pointer, and must free it with Free().
 *
 * Returns NULL if error. */
VbPublicKey* PublicKeyRead(const char* filename);

/* Return true if the public key struct appears correct. */
int PublicKeyLooksOkay(VbPublicKey *key, uint64_t file_size);

/* Read a public key from a .keyb file.  Caller owns the returned
 * pointer, and must free it with Free().
 *
 * Returns NULL if error. */
VbPublicKey* PublicKeyReadKeyb(const char* filename, uint64_t algorithm,
                               uint64_t version);


/* Write a public key to a file in .vbpubk format. */
int PublicKeyWrite(const char* filename, const VbPublicKey* key);


#endif  /* VBOOT_REFERENCE_HOST_KEY_H_ */
