/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_KEY_H_
#define VBOOT_REFERENCE_HOST_KEY_H_

#include "2crypto.h"

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

/**
 * Allocate a new public key.
 * @param key_size	Size of key data the key can hold
 * @param algorithm	Algorithm to store in key header
 * @param version	Version to store in key header
 *
 * @return The public key or NULL if error.  Caller must free() it.
 */
struct vb2_packed_key *vb2_alloc_packed_key(uint32_t key_size,
					    uint32_t algorithm,
					    uint32_t version);

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

/**
 * Read a packed key from a .vbpubk file.
 *
 * @param filename	Name of file to read
 * @param algorithm	Crypto algorithm to associate with key
 * @param version	Version to store in key
 *
 * @return The packed key, or NULL if error.  Caller must free() it.
 */
struct vb2_packed_key *vb2_read_packed_key(const char *filename);

/**
 * Sanity-check a packed key structure.
 *
 * @param key	     	Key to check
 * @param size		Size of key buffer in bytes
 *
 * @return True if the key struct appears valid.
 */
int packed_key_looks_ok(const struct vb2_packed_key *key, uint32_t size);

/**
 * Read a packed key from a .keyb file.
 *
 * @param filename	Name of file to read
 * @param algorithm	Crypto algorithm to associate with key
 * @param version	Version to store in key
 *
 * @return The packed key, or NULL if error.  Caller must free() it.
 */
struct vb2_packed_key *vb2_read_packed_keyb(const char *filename,
					    uint32_t algorithm,
					    uint32_t version);

/**
 * Write a packed key in .vbpubk format.
 *
 * @param filename	Name of file to write
 * @param key		Key to write
 *
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_write_packed_key(const char *filename,
			 const struct vb2_packed_key *key);

#endif  /* VBOOT_REFERENCE_HOST_KEY_H_ */
