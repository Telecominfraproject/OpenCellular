/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Boot descriptor block firmware functions
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2sha.h"
#include "bdb.h"

/*****************************************************************************/

/**
 * Check if string contains a null terminator.
 *
 * Bytes after the null terminator do not need to be null.
 *
 * @param s		String to check
 * @param size		Size of string buffer in characters
 * @return 1 if string has a null terminator, 0 if not
 */
int string_has_null(const char *s, size_t size)
{
	for (; size; size--) {
		if (*s++ == 0)
			return 1;
	}
	return 0;
}

int bdb_check_header(const struct bdb_header *p, size_t size)
{
	if (size < sizeof(*p) || size < p->struct_size)
		return BDB_ERROR_BUF_SIZE;

	if (p->struct_magic != BDB_HEADER_MAGIC)
		return BDB_ERROR_STRUCT_MAGIC;

	if (p->struct_major_version != BDB_HEADER_VERSION_MAJOR)
		return BDB_ERROR_STRUCT_VERSION;

	/* Note that minor version doesn't matter yet */

	if (p->struct_size < sizeof(*p))
		return BDB_ERROR_STRUCT_SIZE;

	if (p->oem_area_0_size & 3)
		return BDB_ERROR_OEM_AREA_SIZE;  /* Not 32-bit aligned */

	/*
	 * Make sure the BDB is at least big enough for us.  At this point, all
	 * the caller may have loaded is this header We'll check if there's
	 * space for everything else after we load it.
	 */
	if (p->bdb_size < sizeof(*p))
		return BDB_ERROR_BDB_SIZE;

	/*
	 * The rest of the fields don't matter yet; we'll check them when we
	 * check the BDB itself.
	 */
	return BDB_SUCCESS;
}

int bdb_check_key(const struct bdb_key *p, size_t size)
{
	size_t expect_key_size = 0;

	if (size < sizeof(*p) || size < p->struct_size)
		return BDB_ERROR_BUF_SIZE;

	if (p->struct_magic != BDB_KEY_MAGIC)
		return BDB_ERROR_STRUCT_MAGIC;

	if (p->struct_major_version != BDB_KEY_VERSION_MAJOR)
		return BDB_ERROR_STRUCT_VERSION;

	/* Note that minor version doesn't matter yet */

	if (!string_has_null(p->description, sizeof(p->description)))
		return BDB_ERROR_DESCRIPTION;

	/* We currently only support SHA-256 */
	if (p->hash_alg != BDB_HASH_ALG_SHA256)
		return BDB_ERROR_HASH_ALG;

	/* Make sure signature algorithm and size are correct */
	switch (p->sig_alg) {
	case BDB_SIG_ALG_RSA4096:
		expect_key_size = BDB_RSA4096_KEY_DATA_SIZE;
		break;
	case BDB_SIG_ALG_ECSDSA521:
		expect_key_size = BDB_ECDSA521_KEY_DATA_SIZE;
		break;
	case BDB_SIG_ALG_RSA3072B:
		expect_key_size = BDB_RSA3072B_KEY_DATA_SIZE;
		break;
	default:
		return BDB_ERROR_SIG_ALG;
	}

	if (p->struct_size < sizeof(*p) + expect_key_size)
		return BDB_ERROR_STRUCT_SIZE;

	return BDB_SUCCESS;
}

int bdb_check_sig(const struct bdb_sig *p, size_t size)
{
	size_t expect_sig_size = 0;

	if (size < sizeof(*p) || size < p->struct_size)
		return BDB_ERROR_BUF_SIZE;

	if (p->struct_magic != BDB_SIG_MAGIC)
		return BDB_ERROR_STRUCT_MAGIC;

	if (p->struct_major_version != BDB_SIG_VERSION_MAJOR)
		return BDB_ERROR_STRUCT_VERSION;

	/* Note that minor version doesn't matter yet */

	if (!string_has_null(p->description, sizeof(p->description)))
		return BDB_ERROR_DESCRIPTION;

	/* We currently only support SHA-256 */
	if (p->hash_alg != BDB_HASH_ALG_SHA256)
		return BDB_ERROR_HASH_ALG;

	/* Make sure signature algorithm and size are correct */
	switch (p->sig_alg) {
	case BDB_SIG_ALG_RSA4096:
		expect_sig_size = BDB_RSA4096_SIG_SIZE;
		break;
	case BDB_SIG_ALG_ECSDSA521:
		expect_sig_size = BDB_ECDSA521_SIG_SIZE;
		break;
	case BDB_SIG_ALG_RSA3072B:
		expect_sig_size = BDB_RSA3072B_SIG_SIZE;
		break;
	default:
		return BDB_ERROR_SIG_ALG;
	}

	if (p->struct_size < sizeof(*p) + expect_sig_size)
		return BDB_ERROR_STRUCT_SIZE;

	return BDB_SUCCESS;
}

int bdb_check_data(const struct bdb_data *p, size_t size)
{
	size_t need_size;

	if (size < sizeof(*p) || size < p->signed_size)
		return BDB_ERROR_BUF_SIZE;

	if (p->struct_magic != BDB_DATA_MAGIC)
		return BDB_ERROR_STRUCT_MAGIC;

	if (p->struct_major_version != BDB_DATA_VERSION_MAJOR)
		return BDB_ERROR_STRUCT_VERSION;

	/* Note that minor version doesn't matter yet */

	if (!string_has_null(p->description, sizeof(p->description)))
		return BDB_ERROR_DESCRIPTION;

	if (p->struct_size < sizeof(*p))
		return BDB_ERROR_STRUCT_SIZE;

	if (p->hash_entry_size < sizeof(struct bdb_hash))
		return BDB_ERROR_HASH_ENTRY_SIZE;

	/* Calculate expected size */
	need_size = p->struct_size + p->num_hashes * p->hash_entry_size;

	/* Make sure OEM area size doesn't cause wraparound */
	if (need_size + p->oem_area_1_size < need_size)
		return BDB_ERROR_OEM_AREA_SIZE;
	if (p->oem_area_1_size & 3)
		return BDB_ERROR_OEM_AREA_SIZE;  /* Not 32-bit aligned */
	need_size += p->oem_area_1_size;

	if (p->signed_size < need_size)
		return BDB_ERROR_SIGNED_SIZE;

	return BDB_SUCCESS;
}

/*****************************************************************************/

const struct bdb_header *bdb_get_header(const void *buf)
{
	return buf;
}

uint32_t bdb_size_of(const void *buf)
{
	return bdb_get_header(buf)->bdb_size;
}

const struct bdb_key *bdb_get_bdbkey(const void *buf)
{
	const struct bdb_header *h = bdb_get_header(buf);
	const uint8_t *b8 = buf;

	/* BDB key follows header */
	return (const struct bdb_key *)(b8 + h->struct_size);
}

const void *bdb_get_oem_area_0(const void *buf)
{
	const struct bdb_key *k = bdb_get_bdbkey(buf);
	const uint8_t *b8 = (const uint8_t *)k;

	/* OEM area 0 follows BDB key */
	return b8 + k->struct_size;
}

const struct bdb_key *bdb_get_datakey(const void *buf)
{
	const struct bdb_header *h = bdb_get_header(buf);
	const uint8_t *b8 = bdb_get_oem_area_0(buf);

	/* datakey follows OEM area 0 */
	return (const struct bdb_key *)(b8 + h->oem_area_0_size);
}

ptrdiff_t bdb_offset_of_datakey(const void *buf)
{
	return vb2_offset_of(buf, bdb_get_datakey(buf));
}

const struct bdb_sig *bdb_get_header_sig(const void *buf)
{
	const struct bdb_header *h = bdb_get_header(buf);
	const uint8_t *b8 = bdb_get_oem_area_0(buf);

	/* Header signature starts after signed data */
	return (const struct bdb_sig *)(b8 + h->signed_size);
}

ptrdiff_t bdb_offset_of_header_sig(const void *buf)
{
	return vb2_offset_of(buf, bdb_get_header_sig(buf));
}

const struct bdb_data *bdb_get_data(const void *buf)
{
	const struct bdb_sig *s = bdb_get_header_sig(buf);
	const uint8_t *b8 = (const uint8_t *)s;

	/* Data follows header signature */
	return (const struct bdb_data *)(b8 + s->struct_size);
}

ptrdiff_t bdb_offset_of_data(const void *buf)
{
	return vb2_offset_of(buf, bdb_get_data(buf));
}

const void *bdb_get_oem_area_1(const void *buf)
{
	const struct bdb_data *p = bdb_get_data(buf);
	const uint8_t *b8 = (const uint8_t *)p;

	/* OEM area 1 follows BDB data */
	return b8 + p->struct_size;
}

const struct bdb_hash *bdb_get_hash(const void *buf, enum bdb_data_type type)
{
	const struct bdb_data *data = bdb_get_data(buf);
	const uint8_t *b8 = bdb_get_oem_area_1(buf);
	int i;

	/* Hashes follow OEM area 0 */
	b8 += data->oem_area_1_size;

	/* Search for a matching hash */
	for (i = 0; i < data->num_hashes; i++, b8 += data->hash_entry_size) {
		const struct bdb_hash *h = (const struct bdb_hash *)b8;

		if (h->type == type)
			return h;
	}

	return NULL;
}

const struct bdb_sig *bdb_get_data_sig(const void *buf)
{
	const struct bdb_data *data = bdb_get_data(buf);
	const uint8_t *b8 = (const uint8_t *)data;

	/* Data signature starts after signed data */
	return (const struct bdb_sig *)(b8 + data->signed_size);
}

/*****************************************************************************/

int bdb_verify_sig(const struct bdb_key *key,
		   const struct bdb_sig *sig,
		   const uint8_t *digest)
{
	/* Key and signature algorithms must match */
	if (key->sig_alg != sig->sig_alg)
		return BDB_ERROR_SIG_ALG;

	switch (key->sig_alg) {
	case BDB_SIG_ALG_RSA4096:
		if (bdb_rsa4096_verify(key->key_data, sig->sig_data, digest))
			return BDB_ERROR_VERIFY_SIG;
		break;
	case BDB_SIG_ALG_ECSDSA521:
		if (bdb_ecdsa521_verify(key->key_data, sig->sig_data, digest))
			return BDB_ERROR_VERIFY_SIG;
		break;
	case BDB_SIG_ALG_RSA3072B:
		if (bdb_rsa3072b_verify(key->key_data, sig->sig_data, digest))
			return BDB_ERROR_VERIFY_SIG;
		break;
	default:
		return BDB_ERROR_VERIFY_SIG;
	}

	return BDB_SUCCESS;
}

int bdb_verify(const void *buf, size_t size, const uint8_t *bdb_key_digest)
{
	const uint8_t *end = (const uint8_t *)buf + size;
	const struct bdb_header *h;
	const struct bdb_key *bdbkey, *datakey;
	const struct bdb_sig *sig;
	const struct bdb_data *data;
	const void *oem;
	uint8_t digest[BDB_SHA256_DIGEST_SIZE];
	int bdb_digest_mismatch = -1;

	/* Make sure buffer doesn't wrap around address space */
	if (end < (const uint8_t *)buf)
		return BDB_ERROR_BUF_SIZE;

	/*
	 * Check header now that we've actually loaded it.  We can't guarantee
	 * this is the same header which was checked before.
	 */
	h = bdb_get_header(buf);
	if (bdb_check_header(h, size))
		return BDB_ERROR_HEADER;

	/* Sanity-check BDB key */
	bdbkey = bdb_get_bdbkey(buf);
	if (bdb_check_key(bdbkey, end - (const uint8_t *)bdbkey))
		return BDB_ERROR_BDBKEY;

	/* Calculate BDB key digest and compare with expected */
	if (vb2_digest_buffer((uint8_t *)bdbkey, bdbkey->struct_size,
			      VB2_HASH_SHA256, digest, BDB_SHA256_DIGEST_SIZE))
		return BDB_ERROR_DIGEST;

	if (bdb_key_digest)
		bdb_digest_mismatch = memcmp(digest,
					     bdb_key_digest, sizeof(digest));

	/* Make sure OEM area 0 fits */
	oem = bdb_get_oem_area_0(buf);
	if (h->oem_area_0_size > end - (const uint8_t *)oem)
		return BDB_ERROR_OEM_AREA_0;

	/* Sanity-check datakey */
	datakey = bdb_get_datakey(buf);
	if (bdb_check_key(datakey, end - (const uint8_t *)datakey))
		return BDB_ERROR_DATAKEY;

	/* Make sure enough data was signed, and the signed data fits */
	if (h->oem_area_0_size + datakey->struct_size > h->signed_size ||
	    h->signed_size > end - (const uint8_t *)oem)
		return BDB_ERROR_BDB_SIGNED_SIZE;

	/* Sanity-check header signature */
	sig = bdb_get_header_sig(buf);
	if (bdb_check_sig(sig, end - (const uint8_t *)sig))
		return BDB_ERROR_HEADER_SIG;

	/* Make sure it signed the right amount of data */
	if (sig->signed_size != h->signed_size)
		return BDB_ERROR_HEADER_SIG;

	/* Calculate header digest and compare with expected signature */
	if (vb2_digest_buffer((uint8_t *)oem, h->signed_size,
			      VB2_HASH_SHA256, digest, BDB_SHA256_DIGEST_SIZE))
		return BDB_ERROR_DIGEST;
	if (bdb_verify_sig(bdbkey, sig, digest))
		return BDB_ERROR_HEADER_SIG;

	/*
	 * Sanity-check data struct.  This also checks that OEM area 1 and the
	 * hashes fit in the remaining buffer.
	 */
	data = bdb_get_data(buf);
	if (bdb_check_data(data, end - (const uint8_t *)data))
		return BDB_ERROR_DATA;

	/* Sanity-check data signature */
	sig = bdb_get_data_sig(buf);
	if (bdb_check_sig(sig, end - (const uint8_t *)sig))
		return BDB_ERROR_DATA_SIG;
	if (sig->signed_size != data->signed_size)
		return BDB_ERROR_DATA_SIG;

	/* Calculate data digest and compare with expected signature */
	if (vb2_digest_buffer((uint8_t *)data, data->signed_size,
			      VB2_HASH_SHA256, digest, BDB_SHA256_DIGEST_SIZE))
		return BDB_ERROR_DIGEST;
	if (bdb_verify_sig(datakey, sig, digest))
		return BDB_ERROR_DATA_SIG;

	/* Return success or success-other-than-BDB-key-mismatch */
	return bdb_digest_mismatch ? BDB_GOOD_OTHER_THAN_KEY : BDB_SUCCESS;
}
