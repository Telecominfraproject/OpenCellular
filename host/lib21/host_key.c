/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keys.
 */

#include <stdio.h>

#include <openssl/pem.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_misc.h"

struct vb2_text_vs_enum vb2_text_vs_algorithm[] = {
	{"RSA1024 SHA1",   VB2_ALG_RSA1024_SHA1},
	{"RSA1024 SHA256", VB2_ALG_RSA1024_SHA256},
	{"RSA1024 SHA512", VB2_ALG_RSA1024_SHA512},
	{"RSA2048 SHA1",   VB2_ALG_RSA2048_SHA1},
	{"RSA2048 SHA256", VB2_ALG_RSA2048_SHA256},
	{"RSA2048 SHA512", VB2_ALG_RSA2048_SHA512},
	{"RSA4096 SHA1",   VB2_ALG_RSA4096_SHA1},
	{"RSA4096 SHA256", VB2_ALG_RSA4096_SHA256},
	{"RSA4096 SHA512", VB2_ALG_RSA4096_SHA512},
	{"RSA8192 SHA1",   VB2_ALG_RSA8192_SHA1},
	{"RSA8192 SHA256", VB2_ALG_RSA8192_SHA256},
	{"RSA8192 SHA512", VB2_ALG_RSA8192_SHA512},
	{0, 0}
};

struct vb2_text_vs_enum vb2_text_vs_sig[] = {
	{"RSA1024", VB2_SIG_RSA1024},
	{"RSA2048", VB2_SIG_RSA2048},
	{"RSA4096", VB2_SIG_RSA4096},
	{"RSA8192", VB2_SIG_RSA8192},
	{0, 0}
};

struct vb2_text_vs_enum vb2_text_vs_hash[] = {
	{"SHA1",   VB2_HASH_SHA1},
	{"SHA256", VB2_HASH_SHA256},
	{"SHA512", VB2_HASH_SHA512},
	{0, 0}
};

const struct vb2_text_vs_enum *vb2_lookup_by_num(
	const struct vb2_text_vs_enum *table,
	const unsigned int num)
{
	for (; table->name; table++)
		if (table->num == num)
			return table;
	return 0;
}

const struct vb2_text_vs_enum *vb2_lookup_by_name(
	const struct vb2_text_vs_enum *table,
	const char *name)
{
	for (; table->name; table++)
		if (!strcasecmp(table->name, name))
			return table;
	return 0;
}

void vb2_private_key_free(struct vb2_private_key *key)
{
	if (!key)
		return;

	if (key->rsa_private_key)
		RSA_free(key->rsa_private_key);

	if (key->desc)
		free(key->desc);

	free(key);
}

int vb2_private_key_unpack(struct vb2_private_key **key_ptr,
			   const uint8_t *buf,
			   uint32_t size)
{
	const struct vb2_packed_private_key *pkey =
		(const struct vb2_packed_private_key *)buf;
	struct vb2_private_key *key;
	const unsigned char *start;
	uint32_t min_offset = 0;

	*key_ptr = NULL;

	/*
	 * Check magic number.
	 *
	 * TODO: If it doesn't match, pass through to the old packed key format.
	 */
	if (pkey->c.magic != VB2_MAGIC_PACKED_PRIVATE_KEY)
		return VB2_ERROR_UNPACK_PRIVATE_KEY_MAGIC;

	if (vb2_verify_common_header(buf, size))
		return VB2_ERROR_UNPACK_PRIVATE_KEY_HEADER;

	/* Make sure key data is inside */
	if (vb2_verify_common_member(pkey, &min_offset,
				     pkey->key_offset, pkey->key_size))
		return VB2_ERROR_UNPACK_PRIVATE_KEY_DATA;

	/*
	 * Check for compatible version.  No need to check minor version, since
	 * that's compatible across readers matching the major version, and we
	 * haven't added any new fields.
	 */
	if (pkey->c.struct_version_major !=
	    VB2_PACKED_PRIVATE_KEY_VERSION_MAJOR)
		return VB2_ERROR_UNPACK_PRIVATE_KEY_STRUCT_VERSION;

	/* Allocate the new key */
	key = calloc(1, sizeof(*key));
	if (!key)
		return VB2_ERROR_UNPACK_PRIVATE_KEY_ALLOC;

	/* Copy key algorithms and ID */
	key->sig_alg = pkey->sig_alg;
	key->hash_alg = pkey->hash_alg;
	key->id = pkey->id;

	/* Unpack RSA key */
	if (pkey->sig_alg == VB2_SIG_NONE) {
		if (pkey->key_size != 0) {
			free(key);
			return VB2_ERROR_UNPACK_PRIVATE_KEY_HASH;
		}
	} else {
		start = (const unsigned char *)(buf + pkey->key_offset);
		key->rsa_private_key = d2i_RSAPrivateKey(0, &start,
							 pkey->key_size);
		if (!key->rsa_private_key) {
			free(key);
			return VB2_ERROR_UNPACK_PRIVATE_KEY_RSA;
		}
	}

	/* Key description */
	if (pkey->c.desc_size) {
		if (vb2_private_key_set_desc(
			     key, (const char *)(buf + pkey->c.fixed_size))) {
			vb2_private_key_free(key);
			return VB2_ERROR_UNPACK_PRIVATE_KEY_DESC;
		}
	}

	*key_ptr = key;
	return VB2_SUCCESS;
}

int vb2_private_key_read(struct vb2_private_key **key_ptr,
			 const char *filename)
{
	uint32_t size = 0;
	uint8_t *buf;
	int rv;

	*key_ptr = NULL;

	rv = vb2_read_file(filename, &buf, &size);
	if (rv)
		return rv;

	rv = vb2_private_key_unpack(key_ptr, buf, size);

	free(buf);

	return rv;
}

int vb2_private_key_read_pem(struct vb2_private_key **key_ptr,
			     const char *filename)
{
	struct vb2_private_key *key;
	FILE *f;

	*key_ptr = NULL;

	/* Allocate the new key */
	key = calloc(1, sizeof(*key));
	if (!key)
		return VB2_ERROR_READ_PEM_ALLOC;

	/* Read private key */
	f = fopen(filename, "rb");
	if (!f) {
		free(key);
		return VB2_ERROR_READ_PEM_FILE_OPEN;
	}

	key->rsa_private_key = PEM_read_RSAPrivateKey(f, NULL, NULL, NULL);
	fclose(f);

	if (!key->rsa_private_key) {
		free(key);
		return VB2_ERROR_READ_PEM_RSA;
	}

	*key_ptr = key;
	return VB2_SUCCESS;
}

int vb2_private_key_set_desc(struct vb2_private_key *key, const char *desc)
{
	if (key->desc)
		free(key->desc);

	if (desc) {
		key->desc = strdup(desc);
		if (!key->desc)
			return VB2_ERROR_PRIVATE_KEY_SET_DESC;
	} else {
		key->desc = NULL;
	}

	return VB2_SUCCESS;
}

int vb2_private_key_write(const struct vb2_private_key *key,
			  const char *filename)
{
	struct vb2_packed_private_key pkey = {
		.c.magic = VB2_MAGIC_PACKED_PRIVATE_KEY,
		.c.struct_version_major = VB2_PACKED_PRIVATE_KEY_VERSION_MAJOR,
		.c.struct_version_minor = VB2_PACKED_PRIVATE_KEY_VERSION_MINOR,
		.c.fixed_size = sizeof(pkey),
		.sig_alg = key->sig_alg,
		.hash_alg = key->hash_alg,
		.id = key->id,
	};
	uint8_t *buf;
	uint8_t *rsabuf = NULL;
	int rsalen = 0;
	int rv;

	memcpy(&pkey.id, &key->id, sizeof(pkey.id));

	pkey.c.desc_size = vb2_desc_size(key->desc);

	if (key->sig_alg != VB2_SIG_NONE) {
		/* Pack RSA key */
		rsalen = i2d_RSAPrivateKey(key->rsa_private_key, &rsabuf);
		if (rsalen <= 0 || !rsabuf)
			return VB2_ERROR_PRIVATE_KEY_WRITE_RSA;
	}

	pkey.key_offset = pkey.c.fixed_size + pkey.c.desc_size;
	pkey.key_size = roundup32(rsalen);
	pkey.c.total_size = pkey.key_offset + pkey.key_size;

	/* Pack private key */
	buf = calloc(1, pkey.c.total_size);
	if (!buf) {
		free(rsabuf);
		return VB2_ERROR_PRIVATE_KEY_WRITE_ALLOC;
	}

	memcpy(buf, &pkey, sizeof(pkey));

	/* strcpy() is ok here because we checked the length above */
	if (pkey.c.desc_size)
		strcpy((char *)buf + pkey.c.fixed_size, key->desc);

	if (rsabuf) {
		memcpy(buf + pkey.key_offset, rsabuf, rsalen);
		free(rsabuf);
	}

	rv = vb2_write_object(filename, buf);
	free(buf);

	return rv ? VB2_ERROR_PRIVATE_KEY_WRITE_FILE : VB2_SUCCESS;
}

int vb2_private_key_hash(const struct vb2_private_key **key_ptr,
			 enum vb2_hash_algorithm hash_alg)
{
	*key_ptr = NULL;

	switch (hash_alg) {
#if VB2_SUPPORT_SHA1
	case VB2_HASH_SHA1:
		{
			static const struct vb2_private_key key = {
				.hash_alg = VB2_HASH_SHA1,
				.sig_alg = VB2_SIG_NONE,
				.desc = "Unsigned SHA1",
				.id = VB2_ID_NONE_SHA1,
			};
			*key_ptr = &key;
			return VB2_SUCCESS;
		}
#endif
#if VB2_SUPPORT_SHA256
	case VB2_HASH_SHA256:
		{
			static const struct vb2_private_key key = {
				.hash_alg = VB2_HASH_SHA256,
				.sig_alg = VB2_SIG_NONE,
				.desc = "Unsigned SHA-256",
				.id = VB2_ID_NONE_SHA256,
			};
			*key_ptr = &key;
			return VB2_SUCCESS;
		}
#endif
#if VB2_SUPPORT_SHA512
	case VB2_HASH_SHA512:
		{
			static const struct vb2_private_key key = {
				.hash_alg = VB2_HASH_SHA512,
				.sig_alg = VB2_SIG_NONE,
				.desc = "Unsigned SHA-512",
				.id = VB2_ID_NONE_SHA512,
			};
			*key_ptr = &key;
			return VB2_SUCCESS;
		}
#endif
	default:
		return VB2_ERROR_PRIVATE_KEY_HASH;
	}
}

int vb2_public_key_alloc(struct vb2_public_key **key_ptr,
			 enum vb2_signature_algorithm sig_alg)
{
	struct vb2_public_key *key;
	uint32_t key_data_size = vb2_packed_key_size(sig_alg);

	/* The buffer contains the key, its ID, and its packed data */
	uint32_t buf_size = sizeof(*key) + sizeof(struct vb2_id) +
		key_data_size;

	if (!key_data_size)
		return VB2_ERROR_PUBLIC_KEY_ALLOC_SIZE;

	key = calloc(1, buf_size);
	if (!key)
		return VB2_ERROR_PUBLIC_KEY_ALLOC;

	key->id = (struct vb2_id *)(key + 1);
	key->sig_alg = sig_alg;

	*key_ptr = key;

	return VB2_SUCCESS;
}

void vb2_public_key_free(struct vb2_public_key *key)
{
	if (!key)
		return;

	if (key->desc)
		free((void *)key->desc);

	free(key);
}

uint8_t *vb2_public_key_packed_data(struct vb2_public_key *key)
{
	return (uint8_t *)(key->id + 1);
}

int vb2_public_key_read_keyb(struct vb2_public_key **key_ptr,
			     const char *filename)
{
	struct vb2_public_key *key = NULL;
	uint8_t *key_data, *key_buf;
	uint32_t key_size;
	enum vb2_signature_algorithm sig_alg;

	*key_ptr = NULL;

	if (vb2_read_file(filename, &key_data, &key_size))
		return VB2_ERROR_READ_KEYB_DATA;

	/* Guess the signature algorithm from the key size */
	for (sig_alg = VB2_SIG_RSA1024; sig_alg <= VB2_SIG_RSA8192; sig_alg++) {
		if (key_size == vb2_packed_key_size(sig_alg))
			break;
	}
	if (sig_alg > VB2_SIG_RSA8192) {
		free(key_data);
		return VB2_ERROR_READ_KEYB_SIZE;
	}

	if (vb2_public_key_alloc(&key, sig_alg)) {
		free(key_data);
		return VB2_ERROR_READ_KEYB_ALLOC;
	}

	/* Copy data from the file buffer to the public key buffer */
	key_buf = vb2_public_key_packed_data(key);
	memcpy(key_buf, key_data, key_size);
	free(key_data);

	if (vb2_unpack_key_data(key, key_buf, key_size)) {
		vb2_public_key_free(key);
		return VB2_ERROR_READ_KEYB_UNPACK;
	}

	*key_ptr = key;

	return VB2_SUCCESS;
}

int vb2_public_key_set_desc(struct vb2_public_key *key, const char *desc)
{
	if (key->desc)
		free((void *)key->desc);

	if (desc) {
		key->desc = strdup(desc);
		if (!key->desc)
			return VB2_ERROR_PUBLIC_KEY_SET_DESC;
	} else {
		key->desc = NULL;
	}

	return VB2_SUCCESS;
}

int vb2_packed_key_read(struct vb2_packed_key **key_ptr,
			const char *filename)
{
	struct vb2_public_key key;
	uint8_t *buf;
	uint32_t size;

	*key_ptr = NULL;

	if (vb2_read_file(filename, &buf, &size))
		return VB2_ERROR_READ_PACKED_KEY_DATA;

	/* Sanity check: make sure key unpacks properly */
	if (vb2_unpack_key(&key, buf, size))
		return VB2_ERROR_READ_PACKED_KEY;

	*key_ptr = (struct vb2_packed_key *)buf;

	return VB2_SUCCESS;
}

int vb2_public_key_pack(struct vb2_packed_key **key_ptr,
			const struct vb2_public_key *pubk)
{
	struct vb2_packed_key key = {
		.c.magic = VB2_MAGIC_PACKED_KEY,
		.c.struct_version_major = VB2_PACKED_KEY_VERSION_MAJOR,
		.c.struct_version_minor = VB2_PACKED_KEY_VERSION_MINOR,
	};
	uint8_t *buf;
	uint32_t *buf32;

	*key_ptr = NULL;

	/* Calculate sizes and offsets */
	key.c.fixed_size = sizeof(key);
	key.c.desc_size = vb2_desc_size(pubk->desc);
	key.key_offset = key.c.fixed_size + key.c.desc_size;

	if (pubk->sig_alg != VB2_SIG_NONE) {
		key.key_size = vb2_packed_key_size(pubk->sig_alg);
		if (!key.key_size)
			return VB2_ERROR_PUBLIC_KEY_PACK_SIZE;
	}

	key.c.total_size = key.key_offset + key.key_size;

	/* Copy/initialize fields */
	key.key_version = pubk->version;
	key.sig_alg = pubk->sig_alg;
	key.hash_alg = pubk->hash_alg;
	key.id = *pubk->id;

	/* Allocate the new buffer */
	buf = calloc(1, key.c.total_size);

	/* Copy data into the buffer */
	memcpy(buf, &key, sizeof(key));

	/* strcpy() is safe because we allocated above based on strlen() */
	if (pubk->desc && *pubk->desc) {
		strcpy((char *)(buf + key.c.fixed_size), pubk->desc);
		buf[key.c.fixed_size + key.c.desc_size - 1] = 0;
	}

	if (pubk->sig_alg != VB2_SIG_NONE) {
		/* Re-pack the key arrays */
		buf32 = (uint32_t *)(buf + key.key_offset);
		buf32[0] = pubk->arrsize;
		buf32[1] = pubk->n0inv;
		memcpy(buf32 + 2, pubk->n, pubk->arrsize * sizeof(uint32_t));
		memcpy(buf32 + 2 + pubk->arrsize, pubk->rr,
		       pubk->arrsize * sizeof(uint32_t));
	}

	*key_ptr = (struct vb2_packed_key *)buf;

	return VB2_SUCCESS;
}

int vb2_public_key_hash(struct vb2_public_key *key,
			enum vb2_hash_algorithm hash_alg)
{
	switch (hash_alg) {
#if VB2_SUPPORT_SHA1
	case VB2_HASH_SHA1:
		key->desc = "Unsigned SHA1";
		break;
#endif
#if VB2_SUPPORT_SHA256
	case VB2_HASH_SHA256:
		key->desc = "Unsigned SHA-256";
		break;
#endif
#if VB2_SUPPORT_SHA512
	case VB2_HASH_SHA512:
		key->desc = "Unsigned SHA-512";
		break;
#endif
	default:
		return VB2_ERROR_PUBLIC_KEY_HASH;
	}

	key->sig_alg = VB2_SIG_NONE;
	key->hash_alg = hash_alg;
	key->id = vb2_hash_id(hash_alg);
	return VB2_SUCCESS;
}

enum vb2_signature_algorithm vb2_rsa_sig_alg(struct rsa_st *rsa)
{
	int bits = BN_num_bits(rsa->n);

	switch (bits) {
	case 1024:
		return VB2_SIG_RSA1024;
	case 2048:
		return VB2_SIG_RSA2048;
	case 4096:
		return VB2_SIG_RSA4096;
	case 8192:
		return VB2_SIG_RSA8192;
	}

	/* no clue */
	return VB2_SIG_INVALID;
}

int vb2_public_key_write(const struct vb2_public_key *key,
			 const char *filename)
{
	struct vb2_packed_key *pkey;
	int ret;

	ret = vb2_public_key_pack(&pkey, key);
	if (ret)
		return ret;

	ret = vb2_write_object(filename, pkey);

	free(pkey);
	return ret;
}
