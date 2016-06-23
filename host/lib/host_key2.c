/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for keys.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <openssl/pem.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "cryptolib.h"
#include "host_common.h"
#include "host_key.h"
#include "host_key2.h"
#include "host_misc.h"
#include "vb2_common.h"
#include "vboot_common.h"

enum vb2_crypto_algorithm vb2_get_crypto_algorithm(
		enum vb2_hash_algorithm hash_alg,
		enum vb2_signature_algorithm sig_alg)
{
	/* Make sure algorithms are in the range supported by crypto alg */
	if (sig_alg < VB2_SIG_RSA1024 || sig_alg > VB2_SIG_RSA8192)
		return VB2_ALG_COUNT;
	if (hash_alg < VB2_HASH_SHA1 || hash_alg > VB2_HASH_SHA512)
		return VB2_ALG_COUNT;

	return (sig_alg - VB2_SIG_RSA1024)
		* (VB2_HASH_SHA512 - VB2_HASH_SHA1 + 1)
		+ (hash_alg - VB2_HASH_SHA1);
};

struct vb2_private_key *vb2_read_private_key(const char *filename)
{
	uint8_t *buf = NULL;
	uint32_t bufsize = 0;
	if (VB2_SUCCESS != vb2_read_file(filename, &buf, &bufsize)) {
		VbExError("unable to read from file %s\n", filename);
		return NULL;
	}

	struct vb2_private_key *key =
		(struct vb2_private_key *)calloc(sizeof(*key), 1);
	if (!key) {
		VbExError("Unable to allocate private key\n");
		free(buf);
		return NULL;
	}

	uint64_t alg = *(uint64_t *)buf;
	key->hash_alg = vb2_crypto_to_hash(alg);
	key->sig_alg = vb2_crypto_to_signature(alg);
	const unsigned char *start = buf + sizeof(alg);

	key->rsa_private_key =
		d2i_RSAPrivateKey(0, &start, bufsize - sizeof(alg));

	if (!key->rsa_private_key) {
		VbExError("Unable to parse RSA private key\n");
		free(buf);
		free(key);
		return NULL;
	}

	free(buf);
	return key;
}

struct vb2_private_key *vb2_read_private_key_pem(
		const char* filename,
		enum vb2_crypto_algorithm algorithm)
{
	if (algorithm >= VB2_ALG_COUNT) {
		VB2_DEBUG("%s() called with invalid algorithm!\n",
			  __FUNCTION__);
		return NULL;
	}

	/* Read private key */
	FILE *f = fopen(filename, "r");
	if (!f) {
		VB2_DEBUG("%s(): Couldn't open key file: %s\n",
			  __FUNCTION__, filename);
		return NULL;
	}
	struct rsa_st *rsa_key = PEM_read_RSAPrivateKey(f, NULL, NULL, NULL);
	fclose(f);
	if (!rsa_key) {
		VB2_DEBUG("%s(): Couldn't read private key from file: %s\n",
			 __FUNCTION__, filename);
		return NULL;
	}

	/* Store key and algorithm in our struct */
	struct vb2_private_key *key =
		(struct vb2_private_key *)calloc(sizeof(*key), 1);
	if (!key) {
		RSA_free(rsa_key);
		return NULL;
	}
	key->rsa_private_key = rsa_key;
	key->hash_alg = vb2_crypto_to_hash(algorithm);
	key->sig_alg = vb2_crypto_to_signature(algorithm);

	/* Return the key */
	return key;
}

void vb2_free_private_key(struct vb2_private_key *key)
{
	if (!key)
		return;
	if (key->rsa_private_key)
		RSA_free(key->rsa_private_key);
	free(key);
}

int vb2_write_private_key(const char *filename,
			  const struct vb2_private_key *key)
{
	/* Convert back to legacy vb1 algorithm enum */
	uint64_t alg = vb2_get_crypto_algorithm(key->hash_alg, key->sig_alg);
	if (alg == VB2_ALG_COUNT)
		return VB2_ERROR_VB1_CRYPTO_ALGORITHM;

	uint8_t *outbuf = NULL;
	int buflen = i2d_RSAPrivateKey(key->rsa_private_key, &outbuf);
	if (buflen <= 0) {
		fprintf(stderr, "Unable to write private key buffer\n");
		return VB2_ERROR_PRIVATE_KEY_WRITE_RSA;
	}

	FILE *f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "Unable to open file %s\n", filename);
		free(outbuf);
		return VB2_ERROR_PRIVATE_KEY_WRITE_FILE;
	}

	if (1 != fwrite(&alg, sizeof(alg), 1, f) ||
	    1 != fwrite(outbuf, buflen, 1, f)) {
		fprintf(stderr, "Unable to write to file %s\n", filename);
		fclose(f);
		unlink(filename);  /* Delete any partial file */
		free(outbuf);
		return VB2_ERROR_PRIVATE_KEY_WRITE_FILE;
	}

	fclose(f);
	free(outbuf);
	return VB2_SUCCESS;
}

void vb2_init_packed_key(struct vb2_packed_key *key, uint8_t *key_data,
			 uint32_t key_size)
{
	memset(key, 0, sizeof(*key));
	key->key_offset = vb2_offset_of(key, key_data);
	key->key_size = key_size;
	key->algorithm = VB2_ALG_COUNT; /* Key not present yet */
}

struct vb2_packed_key *vb2_alloc_packed_key(uint32_t key_size,
					    uint32_t algorithm,
					    uint32_t version)
{
	struct vb2_packed_key *key =
		(struct vb2_packed_key *)calloc(sizeof(*key) + key_size, 1);
	if (!key)
		return NULL;

	key->algorithm = algorithm;
	key->key_version = version;
	key->key_size = key_size;
	key->key_offset = sizeof(*key);
	return key;
}

int vb2_copy_packed_key(struct vb2_packed_key *dest,
			const struct vb2_packed_key *src)
{
	if (dest->key_size < src->key_size)
		return VB2_ERROR_COPY_KEY_SIZE;

	dest->key_size = src->key_size;
	dest->algorithm = src->algorithm;
	dest->key_version = src->key_version;
	memcpy((uint8_t *)vb2_packed_key_data(dest),
	       vb2_packed_key_data(src), src->key_size);
	return VB2_SUCCESS;
}

struct vb2_packed_key *vb2_read_packed_key(const char *filename)
{
	struct vb2_packed_key *key;
	uint32_t file_size;

	if (VB2_SUCCESS !=
	    vb2_read_file(filename, (uint8_t **)&key, &file_size)) {
		return NULL;
	}

	if (packed_key_looks_ok(key, file_size))
		return key;

	/* Error */
	free(key);
	return NULL;
}

struct vb2_packed_key *vb2_read_packed_keyb(const char *filename,
					    uint32_t algorithm,
					    uint32_t version)
{
	if (algorithm >= VB2_ALG_COUNT) {
		fprintf(stderr, "%s() - invalid algorithm\n", __func__);
		return NULL;
	}
	if (version > VB2_MAX_KEY_VERSION) {
		/* Currently, TPM only supports 16-bit version */
		fprintf(stderr, "%s() - invalid version 0x%x\n", __func__,
			version);
		return NULL;
	}

	uint8_t *key_data = NULL;
	uint32_t key_size = 0;
	if (VB2_SUCCESS != vb2_read_file(filename, &key_data, &key_size))
		return NULL;

	uint64_t expected_key_size;
	if (!RSAProcessedKeySize(algorithm, &expected_key_size) ||
	    expected_key_size != key_size) {
		fprintf(stderr, "%s() - wrong key size %u for algorithm %u\n",
			__func__, key_size, algorithm);
		free(key_data);
		return NULL;
	}

	struct vb2_packed_key *key =
		vb2_alloc_packed_key(key_size, algorithm, version);
	if (!key) {
		free(key_data);
		return NULL;
	}
	memcpy((uint8_t *)vb2_packed_key_data(key), key_data, key_size);

	free(key_data);
	return key;
}

int vb2_write_packed_key(const char *filename,
			 const struct vb2_packed_key *key)
{
	/* Copy the key, so its data is contiguous with the header */
	struct vb2_packed_key *kcopy =
		vb2_alloc_packed_key(key->key_size, 0, 0);
	if (!kcopy)
		return VB2_ERROR_PACKED_KEY_ALLOC;
	if (VB2_SUCCESS != vb2_copy_packed_key(kcopy, key)) {
		free(kcopy);
		return VB2_ERROR_PACKED_KEY_COPY;
	}

	/* Write the copy, then free it */
	int rv = vb2_write_file(filename, kcopy,
				kcopy->key_offset + kcopy->key_size);
	free(kcopy);
	return rv;
}
