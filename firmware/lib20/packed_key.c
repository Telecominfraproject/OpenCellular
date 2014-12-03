/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Key unpacking functions
 */

#include "2sysincludes.h"
#include "2rsa.h"
#include "vb2_common.h"

const uint8_t *vb2_packed_key_data(const struct vb2_packed_key *key)
{
	return (const uint8_t *)key + key->key_offset;
}

int vb2_verify_packed_key_inside(const void *parent,
				 uint32_t parent_size,
				 const struct vb2_packed_key *key)
{
	return vb2_verify_member_inside(parent, parent_size,
					key, sizeof(*key),
					key->key_offset, key->key_size);
}

int vb2_unpack_key(struct vb2_public_key *key,
		   const uint8_t *buf,
		   uint32_t size)
{
	const struct vb2_packed_key *packed_key =
		(const struct vb2_packed_key *)buf;
	const uint32_t *buf32;
	uint32_t expected_key_size;
	int rv;

	/* Make sure passed buffer is big enough for the packed key */
	rv = vb2_verify_packed_key_inside(buf, size, packed_key);
	if (rv)
		return rv;

	/* Unpack key algorithm */
	key->sig_alg = vb2_crypto_to_signature(packed_key->algorithm);
	if (key->sig_alg == VB2_SIG_INVALID) {
		VB2_DEBUG("Unsupported signature algorithm.\n");
		return VB2_ERROR_UNPACK_KEY_SIG_ALGORITHM;
	}

	key->hash_alg = vb2_crypto_to_hash(packed_key->algorithm);
	if (key->hash_alg == VB2_HASH_INVALID) {
		VB2_DEBUG("Unsupported hash algorithm.\n");
		return VB2_ERROR_UNPACK_KEY_HASH_ALGORITHM;
	}

	expected_key_size = vb2_packed_key_size(key->sig_alg);
	if (!expected_key_size || expected_key_size != packed_key->key_size) {
		VB2_DEBUG("Wrong key size for algorithm\n");
		return VB2_ERROR_UNPACK_KEY_SIZE;
	}

	/* Make sure source buffer is 32-bit aligned */
	buf32 = (const uint32_t *)vb2_packed_key_data(packed_key);
	if (!vb2_aligned(buf32, sizeof(uint32_t)))
		return VB2_ERROR_UNPACK_KEY_ALIGN;

	/* Sanity check key array size */
	key->arrsize = buf32[0];
	if (key->arrsize * sizeof(uint32_t) != vb2_rsa_sig_size(key->sig_alg))
		return VB2_ERROR_UNPACK_KEY_ARRAY_SIZE;

	key->n0inv = buf32[1];

	/* Arrays point inside the key data */
	key->n = buf32 + 2;
	key->rr = buf32 + 2 + key->arrsize;

	return VB2_SUCCESS;
}
