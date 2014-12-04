/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Key unpacking functions
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "vb2_common.h"

int vb2_unpack_key_data(struct vb2_public_key *key,
			 const uint8_t *key_data,
			 uint32_t key_size)
{
	const uint32_t *buf32 = (const uint32_t *)key_data;
	uint32_t expected_key_size = vb2_packed_key_size(key->sig_alg);

	/* Make sure buffer is the correct length */
	if (!expected_key_size || expected_key_size != key_size) {
		VB2_DEBUG("Wrong key size for algorithm\n");
		return VB2_ERROR_UNPACK_KEY_SIZE;
	}

	/* Check for alignment */
	if (!vb2_aligned(buf32, sizeof(uint32_t)))
		return VB2_ERROR_UNPACK_KEY_ALIGN;

	key->arrsize = buf32[0];

	/* Sanity check key array size */
	if (key->arrsize * sizeof(uint32_t) != vb2_rsa_sig_size(key->sig_alg))
		return VB2_ERROR_UNPACK_KEY_ARRAY_SIZE;

	key->n0inv = buf32[1];

	/* Arrays point inside the key data */
	key->n = buf32 + 2;
	key->rr = buf32 + 2 + key->arrsize;

	return VB2_SUCCESS;
}

int vb2_unpack_key(struct vb2_public_key *key,
		    const uint8_t *buf,
		    uint32_t size)
{
	const struct vb2_packed_key *pkey =
		(const struct vb2_packed_key *)buf;
	uint32_t sig_size;
	uint32_t min_offset = 0;
	int rv;

	/* Check magic number */
	if (pkey->c.magic != VB2_MAGIC_PACKED_KEY)
		return VB2_ERROR_UNPACK_KEY_MAGIC;

	rv = vb2_verify_common_header(buf, size);
	if (rv)
		return rv;

	/* Make sure key data is inside */
	rv = vb2_verify_common_member(pkey, &min_offset,
				      pkey->key_offset, pkey->key_size);
	if (rv)
		return rv;

	/*
	 * Check for compatible version.  No need to check minor version, since
	 * that's compatible across readers matching the major version, and we
	 * haven't added any new fields.
	 */
	if (pkey->c.struct_version_major != VB2_PACKED_KEY_VERSION_MAJOR)
		return VB2_ERROR_UNPACK_KEY_STRUCT_VERSION;

	/* Copy key algorithms */
	key->hash_alg = pkey->hash_alg;
	if (!vb2_digest_size(key->hash_alg))
		return VB2_ERROR_UNPACK_KEY_HASH_ALGORITHM;

	key->sig_alg = pkey->sig_alg;
	if (key->sig_alg != VB2_SIG_NONE) {
		sig_size = vb2_rsa_sig_size(key->sig_alg);
		if (!sig_size)
			return VB2_ERROR_UNPACK_KEY_SIG_ALGORITHM;
		rv = vb2_unpack_key_data(
				key,
				(const uint8_t *)pkey + pkey->key_offset,
				pkey->key_size);
		if (rv)
			return rv;
	}

	/* Key description */
	key->desc = vb2_common_desc(pkey);
	key->version = pkey->key_version;
	key->guid = &pkey->guid;

	return VB2_SUCCESS;
}
