/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_SIGNATURE_H_
#define VBOOT_REFERENCE_HOST_SIGNATURE_H_

#include "cryptolib.h"
#include "host_key.h"
#include "utility.h"
#include "vboot_struct.h"

struct vb2_private_key;
struct vb2_signature;

/**
 * Initialize a signature struct.
 *
 * @param sig		Structure to initialize
 * @param sig_data	Pointer to signature data buffer (after sig)
 * @param sig_size	Size of signature data buffer in bytes
 * @param data_size	Amount of data signed in bytes
 */
void vb2_init_signature(struct vb2_signature *sig, uint8_t *sig_data,
			uint32_t sig_size, uint32_t data_size);


/**
 * Allocate a new signature.
 *
 * @param sig_size	Size of signature in bytes
 * @param data_size	Amount of data signed in bytes
 *
 * @return The signature or NULL if error.  Caller must free() it.
 */
struct vb2_signature *vb2_alloc_signature(uint32_t sig_size,
					  uint32_t data_size);

/**
 * Copy a signature.
 *
 * @param dest		Destination signature
 * @param src		Source signature
 *
 * @return VB2_SUCCESS, or non-zero if error. */
int vb2_copy_signature(struct vb2_signature *dest,
		       const struct vb2_signature *src);

/**
 * Calculate a SHA-512 digest-only signature.
 *
 * @param data		Pointer to data to hash
 * @param size		Length of data in bytes
 *
 * @return The signature, or NULL if error.  Caller must free() it.
 */
struct vb2_signature *vb2_sha512_signature(const uint8_t *data, uint32_t size);

/**
 * Calculate a signature for the data using the specified key.
 *
 * @param data		Pointer to data to sign
 * @param size		Length of data in bytes
 * @param key		Private key to use to sign data
 *
 * @return The signature, or NULL if error.  Caller must free() it.
 */
struct vb2_signature *vb2_calculate_signature(
		const uint8_t *data, uint32_t size,
		const struct vb2_private_key *key);

/**
 * Calculate a signature for the data using an external signer.
 *
 * @param data			Pointer to data to sign
 * @param size			Length of data in bytes
 * @param key_file		Name of file containing private key
 * @param key_algorithm		Key algorithm
 * @param external_signer	Path to external signer program
 *
 * @return The signature, or NULL if error.  Caller must free() it.
 */
struct vb2_signature *vb2_external_signature(const uint8_t *data,
					     uint32_t size,
					     const char *key_file,
					     uint32_t key_algorithm,
					     const char *external_signer);

#endif  /* VBOOT_REFERENCE_HOST_SIGNATURE_H_ */
