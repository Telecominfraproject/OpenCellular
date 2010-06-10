/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_KEY_H_
#define VBOOT_REFERENCE_HOST_KEY_H_

#include <stdint.h>

#include "cryptolib.h"
#include "utility.h"
#include "vboot_struct.h"


typedef struct rsa_st RSA;

/* Private key data */
typedef struct VbPrivateKey {
  RSA* rsa_private_key;  /* Private key data */
  uint64_t algorithm;    /* Algorithm to use when signing */
} VbPrivateKey;


/* Read a private key from a file.  Caller owns the returned pointer,
 * and must free it with PrivateKeyFree(). */
VbPrivateKey* PrivateKeyRead(const char* filename, uint64_t algorithm);


/* Free a private key. */
void PrivateKeyFree(VbPrivateKey* key);


/* Initialize a public key to refer to [key_data]. */
void PublicKeyInit(VbPublicKey* key, uint8_t* key_data, uint64_t key_size);


/* Allocate a new public key with space for a [key_size] byte key. */
VbPublicKey* PublicKeyAlloc(uint64_t key_size, uint64_t algorithm,
                            uint64_t version);


/* Copy a public key from [src] to [dest].
 *
 * Returns 0 if success, non-zero if error. */
int PublicKeyCopy(VbPublicKey* dest, const VbPublicKey* src);


/* Read a public key from a file.  Caller owns the returned pointer,
 * and must free it with Free().
 *
 * Returns NULL if error. */
/* TODO: should really store public keys in files as VbPublicKey */
VbPublicKey* PublicKeyRead(const char* filename, uint64_t algorithm,
                           uint64_t version);

#endif  /* VBOOT_REFERENCE_HOST_KEY_H_ */
