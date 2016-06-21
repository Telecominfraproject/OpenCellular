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
	RSA *rsa_key;
	FILE *f;

	if (algorithm >= VB2_ALG_COUNT) {
		VB2_DEBUG("%s() called with invalid algorithm!\n",
			  __FUNCTION__);
		return NULL;
	}

	/* Read private key */
	f = fopen(filename, "r");
	if (!f) {
		VB2_DEBUG("%s(): Couldn't open key file: %s\n",
			  __FUNCTION__, filename);
		return NULL;
	}
	rsa_key = PEM_read_RSAPrivateKey(f, NULL, NULL, NULL);
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

void vb2_init_packed_key(struct vb2_packed_key *key, uint8_t *key_data,
			 uint32_t key_size)
{
	memset(key, 0, sizeof(*key));
	key->key_offset = vb2_offset_of(key, key_data);
	key->key_size = key_size;
	key->algorithm = VB2_ALG_COUNT; /* Key not present yet */
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
