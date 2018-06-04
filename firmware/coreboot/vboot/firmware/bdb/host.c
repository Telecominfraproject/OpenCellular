/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for signing
 */

#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2sha.h"
#include "bdb.h"
#include "host.h"

char *strzcpy(char *dest, const char *src, size_t size)
{
	strncpy(dest, src, size);
	dest[size - 1] = 0;
	return dest;
}

uint8_t *read_file(const char *filename, uint32_t *size_ptr)
{
	FILE *f;
	uint8_t *buf;
	long size;

	*size_ptr = 0;

	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "Unable to open file %s\n", filename);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);

	if (size < 0 || size > UINT32_MAX) {
		fclose(f);
		return NULL;
	}

	buf = malloc(size);
	if (!buf) {
		fclose(f);
		return NULL;
	}

	if (1 != fread(buf, size, 1, f)) {
		fprintf(stderr, "Unable to read file %s\n", filename);
		fclose(f);
		free(buf);
		return NULL;
	}

	fclose(f);

	*size_ptr = size;
	return buf;
}

int write_file(const char *filename, const void *buf, uint32_t size)
{
	FILE *f = fopen(filename, "wb");

	if (!f) {
		fprintf(stderr, "Unable to open file %s\n", filename);
		return 1;
	}

	if (1 != fwrite(buf, size, 1, f)) {
		fprintf(stderr, "Unable to write to file %s\n", filename);
		fclose(f);
		unlink(filename);  /* Delete any partial file */
		return 1;
	}

	fclose(f);
	return 0;
}

struct rsa_st *read_pem(const char *filename)
{
	struct rsa_st *pem;
	FILE *f;

	/* Read private key */
	f = fopen(filename, "rb");
	if (!f) {
		fprintf(stderr, "%s: unable to read key from %s\n",
			__func__, filename);
		return NULL;
	}

	pem = PEM_read_RSAPrivateKey(f, NULL, NULL, NULL);
	fclose(f);

	return pem;
}

struct bdb_key *bdb_create_key(const char *filename,
			       uint32_t key_version,
			       const char *desc)
{
	uint32_t sig_alg;
	size_t key_size = sizeof(struct bdb_key);
	struct bdb_key *k;
	uint8_t *kdata;
	uint32_t kdata_size = 0;

	/*
	 * Read key data.  Somewhat lame assumption that we can determine the
	 * signature algorithm from the key size, but it's true right now.
	 */
	kdata = read_file(filename, &kdata_size);
	if (kdata_size == BDB_RSA4096_KEY_DATA_SIZE) {
		sig_alg = BDB_SIG_ALG_RSA4096;
	} else if (kdata_size == BDB_RSA3072B_KEY_DATA_SIZE) {
		sig_alg = BDB_SIG_ALG_RSA3072B;
	} else {
		fprintf(stderr, "%s: bad key size from %s\n",
			__func__, filename);
		free(kdata);
		return NULL;
	}
	key_size += kdata_size;

	/* Allocate buffer */
	k = (struct bdb_key *)calloc(key_size, 1);
	if (!k) {
		free(kdata);
		return NULL;
	}

	k->struct_magic = BDB_KEY_MAGIC;
	k->struct_major_version = BDB_KEY_VERSION_MAJOR;
	k->struct_minor_version = BDB_KEY_VERSION_MINOR;
	k->struct_size = key_size;
	k->hash_alg = BDB_HASH_ALG_SHA256;
	k->sig_alg = sig_alg;
	k->key_version = key_version;

	/* Copy description, if any */
	if (desc)
		strzcpy(k->description, desc, sizeof(k->description));

	/* Copy key data */
	memcpy(k->key_data, kdata, kdata_size);
	free(kdata);

	return k;
}

struct bdb_sig *bdb_create_sig(const void *data,
			       size_t size,
			       struct rsa_st *key,
			       uint32_t sig_alg,
			       const char *desc)
{
	static const uint8_t info[] = {
		0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
		0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
		0x00, 0x04, 0x20
	};

	size_t sig_size = sizeof(struct bdb_sig);
	uint8_t digest[sizeof(info) + BDB_SHA256_DIGEST_SIZE];
	struct bdb_sig *sig;

	if (size >= UINT32_MAX)
		return NULL;

	switch(sig_alg) {
	case BDB_SIG_ALG_RSA4096:
		sig_size += BDB_RSA4096_SIG_SIZE;
		break;
	case BDB_SIG_ALG_RSA3072B:
		sig_size += BDB_RSA3072B_SIG_SIZE;
		break;
	default:
		fprintf(stderr, "%s: bad signature algorithm %d\n",
			__func__, sig_alg);
		return NULL;
	}

	/* Allocate buffer */
	sig = (struct bdb_sig *)calloc(sig_size, 1);
	if (!sig)
		return NULL;

	sig->struct_magic = BDB_SIG_MAGIC;
	sig->struct_major_version = BDB_SIG_VERSION_MAJOR;
	sig->struct_minor_version = BDB_SIG_VERSION_MINOR;
	sig->struct_size = sig_size;
	sig->hash_alg = BDB_HASH_ALG_SHA256;
	sig->sig_alg = sig_alg;
	sig->signed_size = size;

	/* Copy description, if any */
	if (desc)
		strzcpy(sig->description, desc, sizeof(sig->description));

	/* Calculate info-padded digest */
	memcpy(digest, info, sizeof(info));
	if (vb2_digest_buffer((uint8_t *)data, size,
			      VB2_HASH_SHA256,
			      digest + sizeof(info), BDB_SHA256_DIGEST_SIZE)) {
		free(sig);
		return NULL;
	}

	/* RSA-encrypt the signature */
	if (RSA_private_encrypt(sizeof(digest),
				digest,
				sig->sig_data,
				key,
				RSA_PKCS1_PADDING) == -1) {
		free(sig);
		return NULL;
	}
	return sig;
}

int bdb_sign_datakey(uint8_t **bdb, struct rsa_st *key)
{
	const struct bdb_header *header = bdb_get_header(*bdb);
	const struct bdb_key *bdbkey = bdb_get_bdbkey(*bdb);
	const void *oem = bdb_get_oem_area_0(*bdb);
	const struct bdb_sig *sig = bdb_get_header_sig(*bdb);
	struct bdb_sig *new_sig;
	uint8_t *new_bdb, *src, *dst;
	size_t len;

	new_sig = bdb_create_sig(oem, header->signed_size,
				 key, bdbkey->sig_alg, NULL);
	new_bdb = calloc(1, header->bdb_size
			 + (new_sig->struct_size - sig->struct_size));
	if (!new_bdb)
		return BDB_ERROR_UNKNOWN;

	/* copy up to sig */
	src = *bdb;
	dst = new_bdb;
	len = bdb_offset_of_header_sig(*bdb);
	memcpy(dst, src, len);

	/* copy new sig */
	src += len;
	dst += len;
	memcpy(dst, new_sig, new_sig->struct_size);

	/* copy the rest */
	src += sig->struct_size;
	dst += new_sig->struct_size;
	len = bdb_size_of(*bdb) - vb2_offset_of(*bdb, src);
	memcpy(dst, src, len);

	free(*bdb);
	free(new_sig);
	*bdb = new_bdb;

	return BDB_SUCCESS;
}

int bdb_sign_data(uint8_t **bdb, struct rsa_st *key)
{
	const struct bdb_key *datakey = bdb_get_datakey(*bdb);
	const struct bdb_data *data = bdb_get_data(*bdb);
	const uint64_t sig_offset = vb2_offset_of(*bdb, bdb_get_data_sig(*bdb));
	struct bdb_sig *new_sig;
	uint8_t *new_bdb;

	new_sig = bdb_create_sig(data, data->signed_size,
				 key, datakey->sig_alg, NULL);
	new_bdb = calloc(1, sig_offset + new_sig->struct_size);
	if (!new_bdb)
		return BDB_ERROR_UNKNOWN;

	/* copy all data up to the data sig */
	memcpy(new_bdb, *bdb, sig_offset);

	/* copy the new signature */
	memcpy(new_bdb + sig_offset, new_sig, new_sig->struct_size);

	free(*bdb);
	free(new_sig);
	*bdb = new_bdb;

	return BDB_SUCCESS;
}

struct bdb_header *bdb_create(struct bdb_create_params *p)
{
	size_t bdb_size = 0;
	size_t sig_size = sizeof(struct bdb_sig) + BDB_RSA4096_SIG_SIZE;
	size_t hashes_size = sizeof(struct bdb_hash) * p->num_hashes;
	uint8_t *buf, *bnext;
	struct bdb_header *h;
	struct bdb_sig *sig;
	struct bdb_data *data;
	const void *oem;

	/* We can do some checks before we even allocate the buffer */

	/* Make sure OEM sizes are aligned */
	if ((p->oem_area_0_size & 3) || (p->oem_area_1_size & 3)) {
		fprintf(stderr, "%s: OEM areas not 32-bit aligned\n",
			__func__);
		return NULL;
	}

	/* Hash count must fit in uint8_t */
	if (p->num_hashes > 255) {
		fprintf(stderr, "%s: too many hashes\n", __func__);
		return NULL;
	}

	/* Calculate BDB size */
	bdb_size = sizeof(struct bdb_header);
	bdb_size += p->bdbkey->struct_size;
	bdb_size += p->oem_area_0_size;
	bdb_size += p->datakey->struct_size;
	bdb_size += sig_size;
	bdb_size += sizeof(struct bdb_data);
	bdb_size += p->oem_area_1_size;
	bdb_size += sizeof(struct bdb_hash) * p->num_hashes;
	bdb_size += sig_size;

	/* Make sure it fits */
	if (bdb_size > UINT32_MAX) {
		fprintf(stderr, "%s: BDB size > UINT32_MAX\n", __func__);
		return NULL;
	}

	/* Allocate a buffer */
	bnext = buf = calloc(bdb_size, 1);
	if (!buf) {
		fprintf(stderr, "%s: can't allocate buffer\n", __func__);
		return NULL;
	}

	/* Fill in the header */
	h = (struct bdb_header *)bnext;
	h->struct_magic = BDB_HEADER_MAGIC;
	h->struct_major_version = BDB_HEADER_VERSION_MAJOR;
	h->struct_minor_version = BDB_HEADER_VERSION_MINOR;
	h->struct_size = sizeof(*h);
	h->bdb_load_address = p->bdb_load_address;
	h->bdb_size = bdb_size;
	h->signed_size = p->oem_area_0_size + p->datakey->struct_size;
	h->oem_area_0_size = p->oem_area_0_size;
	bnext += h->struct_size;

	/* Copy BDB key */
	memcpy(bnext, p->bdbkey, p->bdbkey->struct_size);
	bnext += p->bdbkey->struct_size;

	/* Copy OEM area 0 */
	oem = bnext;
	if (p->oem_area_0_size) {
		memcpy(bnext, p->oem_area_0, p->oem_area_0_size);
		bnext += p->oem_area_0_size;
	}

	/* Copy datakey */
	memcpy(bnext, p->datakey, p->datakey->struct_size);
	bnext += p->datakey->struct_size;

	/*
	 * Create header signature using private BDB key.
	 *
	 * TODO: create the header signature in a totally separate step.  That
	 * way, the private BDB key is not required each time a BDB is created.
	 */
	sig = bdb_create_sig(oem, h->signed_size, p->private_bdbkey,
			     p->bdbkey->sig_alg, p->header_sig_description);
	memcpy(bnext, sig, sig->struct_size);
	bnext += sig->struct_size;

	/* Fill in the data */
	data = (struct bdb_data *)bnext;
	data->struct_magic = BDB_DATA_MAGIC;
	data->struct_major_version = BDB_DATA_VERSION_MAJOR;
	data->struct_minor_version = BDB_DATA_VERSION_MINOR;
	data->struct_size = sizeof(struct bdb_data);
	data->data_version = p->data_version;
	data->oem_area_1_size = p->oem_area_1_size;
	data->num_hashes = p->num_hashes;
	data->hash_entry_size = sizeof(struct bdb_hash);
	data->signed_size = data->struct_size + data->oem_area_1_size +
		hashes_size;
	if (p->data_description) {
		strzcpy(data->description, p->data_description,
			sizeof(data->description));
	}
	bnext += data->struct_size;

	/* Copy OEM area 1 */
	oem = bnext;
	if (p->oem_area_1_size) {
		memcpy(bnext, p->oem_area_1, p->oem_area_1_size);
		bnext += p->oem_area_1_size;
	}

	/* Copy hashes */
	memcpy(bnext, p->hash, hashes_size);
	bnext += hashes_size;

	/* Create data signature using private datakey */
	sig = bdb_create_sig(data, data->signed_size, p->private_datakey,
			     p->datakey->sig_alg, p->data_sig_description);
	memcpy(bnext, sig, sig->struct_size);

	/* Return the BDB */
	return h;
}
