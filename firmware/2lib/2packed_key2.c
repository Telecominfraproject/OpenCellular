/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Key unpacking functions
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"

const uint8_t *vb2_packed_key2_data(const struct vb2_packed_key2 *key)
{
	return (const uint8_t *)key + key->key_offset;
}

int vb2_verify_packed_key2_inside(const void *parent,
				  uint32_t parent_size,
				  const struct vb2_packed_key2 *key)
{
	int rv;

	rv = vb2_verify_member_inside(parent, parent_size,
				      key, sizeof(*key),
				      key->key_offset, key->key_size);
	if (rv)
		return rv;

	return vb2_verify_common_header(parent, parent_size, &key->c);
}

int vb2_unpack_key2(struct vb2_public_key *key,
		    const uint8_t *buf,
		    uint32_t size)
{
	const struct vb2_packed_key2 *pkey =
		(const struct vb2_packed_key2 *)buf;
	const uint32_t *buf32;
	uint32_t expected_key_size;
	uint32_t sig_size;
	int rv;

	/*
	 * Check magic number.
	 *
	 * If it doesn't match, pass through to the old packed key format.
	 *
	 * TODO: remove passthru when signing scripts have switched over to
	 * use the new format.
	 */
	if (pkey->c.magic != VB2_MAGIC_PACKED_KEY2)
		return vb2_unpack_key(key, buf, size);

	/* Make sure passed buffer is big enough for the packed key */
	rv = vb2_verify_packed_key2_inside(buf, size, pkey);
	if (rv)
		return rv;

	/*
	 * Check for compatible version.  No need to check minor version, since
	 * that's compatible across readers matching the major version, and we
	 * haven't added any new fields.
	 */
	if (pkey->c.struct_version_major != VB2_PACKED_KEY2_VERSION_MAJOR)
		return VB2_ERROR_UNPACK_KEY_STRUCT_VERSION;

	/* Copy key algorithms */
	key->sig_alg = pkey->sig_algorithm;
	sig_size = vb2_rsa_sig_size(key->sig_alg);
	if (!sig_size)
		return VB2_ERROR_UNPACK_KEY_SIG_ALGORITHM;

	key->hash_alg = pkey->hash_algorithm;
	if (!vb2_digest_size(key->hash_alg))
		return VB2_ERROR_UNPACK_KEY_HASH_ALGORITHM;

	expected_key_size = vb2_packed_key_size(key->sig_alg);
	if (!expected_key_size || expected_key_size != pkey->key_size) {
		VB2_DEBUG("Wrong key size for algorithm\n");
		return VB2_ERROR_UNPACK_KEY_SIZE;
	}

	/* Make sure source buffer is 32-bit aligned */
	buf32 = (const uint32_t *)vb2_packed_key2_data(pkey);
	if (!vb2_aligned(buf32, sizeof(uint32_t)))
		return VB2_ERROR_UNPACK_KEY_ALIGN;

	/* Sanity check key array size */
	key->arrsize = buf32[0];
	if (key->arrsize * sizeof(uint32_t) != sig_size)
		return VB2_ERROR_UNPACK_KEY_ARRAY_SIZE;

	key->n0inv = buf32[1];

	/* Arrays point inside the key data */
	key->n = buf32 + 2;
	key->rr = buf32 + 2 + key->arrsize;

	/* Key description */
	if (pkey->c.desc_size)
		key->desc = (const char *)&(pkey->c) + pkey->c.fixed_size;
	else
		key->desc = "";

	key->version = pkey->key_version;
	key->guid = &pkey->key_guid;

	return VB2_SUCCESS;
}
