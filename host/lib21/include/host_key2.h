/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot key structures
 */

#ifndef VBOOT_REFERENCE_HOST_KEY2_H_
#define VBOOT_REFERENCE_HOST_KEY2_H_

#include "2struct.h"

struct vb2_public_key;

/* Private key data, in-memory format for use in signing calls. */
struct vb2_private_key {
	struct rsa_st *rsa_private_key;		/* Private key data */
	enum vb2_hash_algorithm hash_alg;	/* Hash algorithm */
	enum vb2_signature_algorithm sig_alg;	/* Signature algorithm */
	char *desc;				/* Description */
	struct vb2_guid guid;			/* Key GUID */
};

/* Convert between enums and human-readable form. Terminated with {0, 0}. */
struct vb2_text_vs_enum {
	const char *name;
	unsigned int num;
};

/**
 * @param table         Table to search
 * @param num           Enum value to search for
 * @return pointer to table entry or NULL if no match
 */
const struct vb2_text_vs_enum *vb2_lookup_by_num(
	const struct vb2_text_vs_enum *table,
	const unsigned int num);

/**
 * @param table         Table to search
 * @param name          String value to search for
 * @return pointer to table entry or NULL if no match
 */
const struct vb2_text_vs_enum *vb2_lookup_by_name(
	const struct vb2_text_vs_enum *table,
	const char *name);

extern struct vb2_text_vs_enum vb2_text_vs_algorithm[];
extern struct vb2_text_vs_enum vb2_text_vs_sig[];
extern struct vb2_text_vs_enum vb2_text_vs_hash[];

/**
 * Free a private key.
 *
 * @param key		Key containing internal data to free.
 */
void vb2_private_key_free(struct vb2_private_key *key);

/**
 * Unpack a private key from vb2_packed_private_key format.
 *
 * @param key_ptr	Destination for newly allocated key; this must be
 *			freed with vb2_private_key_free().
 * @param buf		Source buffer containing packed key
 * @param size		Size of buffer in bytes
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_private_key_unpack(struct vb2_private_key **key_ptr,
			   const uint8_t *buf,
			   uint32_t size);

/**
 * Read a private key from vb2_packed_private_key format.
 *
 * @param key_ptr	Destination for newly allocated key; this must be
 *			freed with vb2_private_key_free().
 * @param filename	File to read key data from.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_private_key_read(struct vb2_private_key **key_ptr,
			 const char *filename);

/**
 * Read a private key from a .pem file.
 *
 * This only reads the internal data for the key.  It does not set any of the
 * other fields in *key_ptr, since those are not contained in the .pem file.
 *
 * @param key_ptr	Destination for newly allocated key; this must be
 *			freed with vb2_private_key_free().
 * @param filename	File to read key data from.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_private_key_read_pem(struct vb2_private_key **key_ptr,
			     const char *filename);

/**
 * Set the description of a private key.
 *
 * @param key		Key to set description for
 * @param desc		Description string, or NULL if no description.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_private_key_set_desc(struct vb2_private_key *key, const char *desc);

/**
 * Write a private key to vb2_packed_private_key format.
 *
 * @param key		Key to write
 * @param filename	File to write key data to.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_private_key_write(const struct vb2_private_key *key,
			  const char *filename);

/**
 * Get a private key for an unsigned hash
 *
 * @param key_ptr	Destination for pointer to key.  The key is statically
 *			allocated and must not be freed.
 * @param hash_alg	Hash algorithm to use
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_private_key_hash(const struct vb2_private_key **key_ptr,
			 enum vb2_hash_algorithm hash_alg);

/**
 * Allocate a public key buffer of sufficient size for the signature algorithm.
 *
 * This only initializes the sig_alg field and the guid field to an empty
 * guid.  It does not set any of the other fields in *key_ptr.
 *
 * @param key_ptr	Destination for newly allocated key; this must be
 *			freed with vb2_public_key_free().
 * @param sig_alg	Signature algorithm for key.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_public_key_alloc(struct vb2_public_key **key_ptr,
			 enum vb2_signature_algorithm sig_alg);

/**
 * Return the packed data for a key allocated with vb2_public_key_alloc().
 *
 * The packed data is in the same buffer, following the key struct and GUID.
 */
uint8_t *vb2_public_key_packed_data(struct vb2_public_key *key);

/**
 * Free a public key allocated by one of the functions below.
 *
 * Note that this should ONLY be called for public keys allocated via one
 * of those functions; public keys created or filled in other ways (such as
 * vb2_unpack_key()) do not allocate memory for sub-fields in the same way.
 *
 * @param key		Key to free
 */
void vb2_public_key_free(struct vb2_public_key *key);

/**
 * Read a public key from a .keyb file.
 *
 * Guesses the signature algorithm based on the size of the .keyb file.  Does
 * not set the hash_alg, guid, or desc fields, since those are not contained in
 * the .keyb file.
 *
 * @param key_ptr	Destination for newly allocated key; this must be
 *			freed with vb2_public_key_free().
 * @param filename	File to read key from.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */

int vb2_public_key_read_keyb(struct vb2_public_key **key_ptr,
			     const char *filename);

/**
 * Set the description of a public key.
 *
 * @param key		Key to set description for
 * @param desc		Description string, or NULL if no description.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_public_key_set_desc(struct vb2_public_key *key, const char *desc);

/**
 * Read a public key in vb2_packed_key format.
 *
 * @param key_ptr	On success, points to the newly allocated key buffer.
 *			Caller is responsible for calling free() on this.
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_packed_key_read(struct vb2_packed_key **key_ptr,
			 const char *filename);

/**
 * Pack a public key into vb2_packed_key format.
 *
 * @param pubk		Public key to pack
 * @param key_ptr	On success, points to a newly allocated packed key
 *			buffer.  Caller is responsible for calling free() on
 *			this.
 * @return VB2_SUCCESS, or non-zero if error.
 */
int vb2_public_key_pack(struct vb2_packed_key **key_ptr,
			const struct vb2_public_key *pubk);

/**
 * Get a public key for an unsigned hash.
 *
 * @param key		Destination for key data.
 * @param hash_alg	Hash algorithm to use
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_public_key_hash(struct vb2_public_key *key,
			enum vb2_hash_algorithm hash_alg);


/**
 * Return the signature algorithm implied by the bit length of an RSA key
 *
 * @param rsa		RSA key
 * @return vb2 signature algorithm
 */
enum vb2_signature_algorithm vb2_rsa_sig_alg(struct rsa_st *rsa);

/**
 * Write a public key to the vb2_packed_key format.
 *
 * @param key		Key to write
 * @param filename	File to write key data to.
 * @return VB2_SUCCESS, or non-zero error code if error.
 */
int vb2_public_key_write(const struct vb2_public_key *key,
			 const char *filename);

#endif  /* VBOOT_REFERENCE_HOST_KEY2_H_ */
