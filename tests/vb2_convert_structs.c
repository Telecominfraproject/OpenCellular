/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Convert structs from vboot1 data format to new vboot2 structs
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "vb2_convert_structs.h"
#include "vboot_struct.h"  /* For old struct sizes */

#include "test_common.h"

struct vb2_packed_key2 *vb2_convert_packed_key2(
				const struct vb2_packed_key *key,
				const char *desc, uint32_t *out_size)
{
	struct vb2_packed_key2 k2 = {
		.c.magic = VB2_MAGIC_PACKED_KEY2,
		.c.struct_version_major = VB2_PACKED_KEY2_VERSION_MAJOR,
		.c.struct_version_minor = VB2_PACKED_KEY2_VERSION_MINOR,
	};
	uint8_t *kbuf;

	/* Calculate sizes and offsets */
	k2.c.fixed_size = sizeof(k2);
	k2.c.desc_size = roundup32(strlen(desc) + 1);
	k2.key_offset = k2.c.fixed_size + k2.c.desc_size;
	k2.key_size = key->key_size;
	k2.c.total_size = k2.key_offset + k2.key_size;

	/* Copy/initialize fields */
	k2.key_version = key->key_version;
	k2.sig_alg = vb2_crypto_to_signature(key->algorithm);
	k2.hash_alg = vb2_crypto_to_hash(key->algorithm);
	/* TODO: fill in a non-zero GUID */

	/* Allocate the new buffer */
	*out_size = k2.c.total_size;
	kbuf = malloc(*out_size);
	memset(kbuf, 0, *out_size);

	/* Copy data into the buffer */
	memcpy(kbuf, &k2, sizeof(k2));

	/* strcpy() is safe because we allocated above based on strlen() */
	strcpy((char *)(kbuf + k2.c.fixed_size), desc);
	kbuf[k2.c.fixed_size + k2.c.desc_size - 1] = 0;

	memcpy(kbuf + k2.key_offset,
	       (const uint8_t *)key + key->key_offset,
	       key->key_size);

	/* Return the newly allocated buffer */
	return (struct vb2_packed_key2 *)kbuf;
}

struct vb2_signature2 *vb2_convert_signature2(
			      const struct vb2_signature *sig,
			      const char *desc,
			      const struct vb2_packed_key2 *key,
			      uint32_t *out_size)
{
	struct vb2_signature2 s2 = {
		.c.magic = VB2_MAGIC_SIGNATURE2,
		.c.struct_version_major = VB2_SIGNATURE2_VERSION_MAJOR,
		.c.struct_version_minor = VB2_SIGNATURE2_VERSION_MINOR,
	};
	uint8_t *buf;

	/* Calculate description size */
	s2.c.fixed_size = sizeof(s2);
	s2.c.desc_size = roundup32(strlen(desc) + 1);

	/* Copy/initialize fields */
	s2.sig_offset = s2.c.fixed_size + s2.c.desc_size;
	s2.sig_size = sig->sig_size;
	s2.c.total_size = s2.sig_offset + s2.sig_size;
	s2.data_size = sig->data_size;

	/* Copy fields from key if present */
	if (key) {
		s2.sig_alg = key->sig_alg;
		s2.hash_alg = key->hash_alg;
		memcpy(&s2.guid, &key->guid, GUID_SIZE);
	} else {
		s2.sig_alg = VB2_SIG_INVALID;
		s2.hash_alg = VB2_HASH_INVALID;
		memset(&s2.guid, 0, GUID_SIZE);
	}

	/* Allocate the new buffer */
	*out_size = s2.sig_offset + s2.sig_size;
	buf = malloc(*out_size);
	memset(buf, 0, *out_size);

	/* Copy data into the buffer */
	memcpy(buf, &s2, sizeof(s2));

	/* strcpy() is safe because we allocated above based on strlen() */
	strcpy((char *)(buf + s2.c.fixed_size), desc);
	buf[s2.c.fixed_size + s2.c.desc_size - 1] = 0;

	memcpy(buf + s2.sig_offset,
	       (const uint8_t *)sig + sig->sig_offset,
	       sig->sig_size);

	/* Return the newly allocated buffer */
	return (struct vb2_signature2 *)buf;
}

struct vb2_signature2 *vb2_create_hash_sig(const uint8_t *data,
					   uint32_t size,
					   enum vb2_hash_algorithm hash_alg)
{
	const char desc[12] = "hash sig";
	struct vb2_signature2 s = {
		.c.magic = VB2_MAGIC_SIGNATURE2,
		.c.struct_version_major = VB2_SIGNATURE2_VERSION_MAJOR,
		.c.struct_version_minor = VB2_SIGNATURE2_VERSION_MINOR,
		.c.fixed_size = sizeof(s),
		.c.desc_size = sizeof(desc),
		.sig_alg = VB2_SIG_NONE,
		.hash_alg = hash_alg,
		.sig_size = vb2_sig_size(VB2_SIG_NONE, hash_alg),
		.data_size = size,
	};
	const struct vb2_guid *hash_guid = vb2_hash_guid(hash_alg);
	struct vb2_digest_context dc;
	uint8_t *buf;

	/* Make sure hash algorithm was supported */
	if (!hash_guid || !s.sig_size)
		return NULL;

	memcpy(&s.guid, hash_guid, sizeof(s.guid));
	s.sig_offset = s.c.fixed_size + s.c.desc_size;
	s.c.total_size = s.sig_offset + s.sig_size;

	buf = malloc(s.c.total_size);
	memset(buf, 0, s.c.total_size);
	memcpy(buf, &s, sizeof(s));
	memcpy(buf + s.c.fixed_size, desc, sizeof(desc));

	if (vb2_digest_init(&dc, hash_alg) ||
	    vb2_digest_extend(&dc, data, size) ||
	    vb2_digest_finalize(&dc, buf + s.sig_offset, s.sig_size)) {
		free(buf);
		return NULL;
	}

	return (struct vb2_signature2 *)buf;
}
