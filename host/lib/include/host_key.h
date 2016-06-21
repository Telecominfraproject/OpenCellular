/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_KEY_H_
#define VBOOT_REFERENCE_HOST_KEY_H_

#include "2crypto.h"
#include "cryptolib.h"
#include "vboot_struct.h"

struct vb2_packed_key;
struct vb2_private_key;

typedef struct rsa_st RSA;

/* Private key data */
typedef struct VbPrivateKey {
  RSA* rsa_private_key;  /* Private key data */
  uint64_t algorithm;    /* Algorithm to use when signing */
} VbPrivateKey;


/* Read a private key from a .pem file.  Caller owns the returned pointer,
 * and must free() it. */
VbPrivateKey* PrivateKeyReadPem(const char* filename, uint64_t algorithm);
struct vb2_private_key *vb2_read_private_key_pem(
		const char *filename,
		enum vb2_crypto_algorithm algorithm);

/* Free a private key. */
void PrivateKeyFree(VbPrivateKey* key);

/* Write a private key to a file in .vbprivk format. */
int PrivateKeyWrite(const char* filename, const VbPrivateKey* key);

/* Read a private key from a .vbprivk file.  Caller owns the returned
 * pointer, and must free() it.
 *
 * Returns NULL if error. */
VbPrivateKey* PrivateKeyRead(const char* filename);
struct vb2_private_key *vb2_read_private_key(const char *filename);

/* Allocate a new public key with space for a [key_size] byte key. */
VbPublicKey* PublicKeyAlloc(uint64_t key_size, uint64_t algorithm,
                            uint64_t version);

/**
 * Initialize a packed key structure.
 *
 * @param key		Structure to initialize
 * @param key_data	Pointer to key data (following the structure)
 * @param key_size	Size of key
 */
void vb2_init_packed_key(struct vb2_packed_key *key, uint8_t *key_data,
			 uint32_t key_size);

/**
 * Copy a packed key.
 *
 * @param dest		Destination packed key
 * @param src		Source packed key
 *
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_copy_packed_key(struct vb2_packed_key *dest,
			const struct vb2_packed_key *src);

/* Read a public key from a .vbpubk file.  Caller owns the returned
 * pointer, and must free it with Free().
 *
 * Returns NULL if error. */
VbPublicKey* PublicKeyRead(const char* filename);
struct vb2_packed_key *vb2_read_packed_key(const char *filename);

/* Return true if the packed (public) key struct appears correct. */
int packed_key_looks_ok(const struct vb2_packed_key *key, uint32_t size);

/* Read a public key from a .keyb file.  Caller owns the returned
 * pointer, and must free it with Free().
 *
 * Returns NULL if error. */
VbPublicKey* PublicKeyReadKeyb(const char* filename, uint64_t algorithm,
                               uint64_t version);


/* Write a public key to a file in .vbpubk format. */
int PublicKeyWrite(const char* filename, const VbPublicKey* key);


#endif  /* VBOOT_REFERENCE_HOST_KEY_H_ */
