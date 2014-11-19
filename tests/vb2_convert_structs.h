/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef VBOOT_REFERENCE_VB2_CONVERT_STRUCTS_H_
#define VBOOT_REFERENCE_VB2_CONVERT_STRUCTS_H_

#include "2struct.h"

/**
 * Convert a packed key from vboot data format to vboot2 data format.
 *
 * Intended for use by unit tests.  Does NOT validate the original struct
 * contents, just copies them.
 *
 * @param key		Packed key in vboot1 format
 * @param desc		Description of packed key
 * @param out_size	Destination for size of the newly allocated buffer
 * @return a newly allocated buffer with the converted key.  Caller is
 * responsible for freeing this buffer.
 */
struct vb2_packed_key2 *vb2_convert_packed_key2(
				const struct vb2_packed_key *key,
				const char *desc, uint32_t *out_size);

/**
 * Convert a signature from vboot data format to vboot2 data format.
 *
 * Intended for use by unit tests.  Does NOT validate the original struct
 * contents, just copies them.
 *
 * @param sig		Signature in vboot1 format
 * @param desc		Description of signature
 * @param key		Key to take algorithms and GUID from.  If NULL, those
 *			fields are left uninitialized.
 * @param out_size	Destination for size of the newly allocated buffer
 * @return a newly allocated buffer with the converted signature.  Caller is
 * responsible for freeing this buffer.
 */
struct vb2_signature2 *vb2_convert_signature2(
			      const struct vb2_signature *sig,
			      const char *desc,
			      const struct vb2_packed_key2 *key,
			      uint32_t *out_size);

/**
 * Create an unsigned hash signature of the data.
 *
 * @param data		Data to sign
 * @param size		Size of data in bytes
 * @return a newly-allocated signature, which the caller must free, or NULL if
 *	   error.
 */
struct vb2_signature2 *vb2_create_hash_sig(const uint8_t *data,
					   uint32_t size,
					   enum vb2_hash_algorithm hash_alg);

#endif  /* VBOOT_REFERENCE_VB2_CONVERT_STRUCTS_H_ */
