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

/**
 * Convert a vb2 hash and crypto algorithm to a vb1 crypto algorithm.
 *
 * @param hash_alg	Hash algorithm
 * @param sig_alg	Signature algorithm
 *
 * @return The equivalent vb1 crypto algorithm or VB2_ALG_COUNT if error.
 */
enum vb2_crypto_algorithm vb2_get_crypto_algorithm(
		enum vb2_hash_algorithm hash_alg,
		enum vb2_signature_algorithm sig_alg);

/**
 * Read a private key from a .pem file.
 *
 * @param filename	Filename to read from
 * @param algorithm	Algorithm to associate with file
 * 			(enum vb2_crypto_algorithm)
 *
 * @return The private key or NULL if error.  Caller must free() it.
 */
struct vb2_private_key *vb2_read_private_key_pem(
		const char *filename,
		enum vb2_crypto_algorithm algorithm);

/**
 * Free a private key.
 *
 * @param key		Key to free; ok to pass NULL (ignored).
 */
void vb2_free_private_key(struct vb2_private_key *key);

/**
 * Write a private key to a file in .vbprivk format.
 *
 * @param filename	Filename to write to
 * @param key		Key to write
 *
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_write_private_key(const char *filename,
			  const struct vb2_private_key *key);


/**
 * Read a private key from a .vbprivk file.
 *
 * @param filename	Filename to read key from.
 *
 * @return The private key or NULL if error.  Caller must free() it.
 */
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
