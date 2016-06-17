/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for signature generation.
 */

#include <openssl/rsa.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "2sysincludes.h"

#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "cryptolib.h"
#include "file_keys.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_signature2.h"
#include "vb2_common.h"
#include "vboot_common.h"

struct vb2_signature *vb2_alloc_signature(uint32_t sig_size,
					  uint32_t data_size)
{
	struct vb2_signature *sig = (struct vb2_signature *)
		calloc(sizeof(*sig) + sig_size, 1);
	if (!sig)
		return NULL;

	sig->sig_offset = sizeof(*sig);
	sig->sig_size = sig_size;
	sig->data_size = data_size;

	return sig;
}

void vb2_init_signature(struct vb2_signature *sig, uint8_t *sig_data,
			uint32_t sig_size, uint32_t data_size)
{
	sig->sig_offset = OffsetOf(sig, sig_data);
	sig->sig_size = sig_size;
	sig->data_size = data_size;
}

int vb2_copy_signature(struct vb2_signature *dest,
		       const struct vb2_signature *src)
{
	if (dest->sig_size < src->sig_size)
		return VB2_ERROR_SIG_SIZE;

	dest->sig_size = src->sig_size;
	dest->data_size = src->data_size;

	memcpy(vb2_signature_data(dest),
	       vb2_signature_data((struct vb2_signature *)src),
	       src->sig_size);

	return VB2_SUCCESS;
}

struct vb2_signature *vb2_sha512_signature(const uint8_t *data, uint32_t size)
{
	uint8_t digest[VB2_SHA512_DIGEST_SIZE];
	if (VB2_SUCCESS != vb2_digest_buffer(data, size, VB2_HASH_SHA512,
					     digest, sizeof(digest)))
		return NULL;

	struct vb2_signature *sig =
		vb2_alloc_signature(VB2_SHA512_DIGEST_SIZE, size);
	if (!sig)
		return NULL;

	memcpy(vb2_signature_data(sig), digest, VB2_SHA512_DIGEST_SIZE);
	return sig;
}

struct vb2_signature *vb2_calculate_signature(
		const uint8_t *data, uint32_t size,
		const struct vb2_private_key *key)
{
	uint8_t digest[VB2_MAX_DIGEST_SIZE];
	uint32_t digest_size = vb2_digest_size(key->hash_alg);

	uint32_t digest_info_size = 0;
	const uint8_t *digest_info = NULL;
	if (VB2_SUCCESS != vb2_digest_info(key->hash_alg,
					   &digest_info, &digest_info_size))
		return NULL;

	/* Calculate the digest */
	if (VB2_SUCCESS != vb2_digest_buffer(data, size, key->hash_alg,
					     digest, digest_size))
		return NULL;

	/* Prepend the digest info to the digest */
	int signature_digest_len = digest_size + digest_info_size;
	uint8_t *signature_digest = malloc(signature_digest_len);
	if (!signature_digest)
		return NULL;

	memcpy(signature_digest, digest_info, digest_info_size);
	memcpy(signature_digest + digest_info_size, digest, digest_size);

	/* Allocate output signature */
	struct vb2_signature *sig = (struct vb2_signature *)
		vb2_alloc_signature(vb2_rsa_sig_size(key->sig_alg), size);
	if (!sig) {
		free(signature_digest);
		return NULL;
	}

	/* Sign the signature_digest into our output buffer */
	int rv = RSA_private_encrypt(signature_digest_len,    /* Input length */
				     signature_digest,        /* Input data */
				     vb2_signature_data(sig), /* Output sig */
				     key->rsa_private_key,    /* Key to use */
				     RSA_PKCS1_PADDING);      /* Padding */
	free(signature_digest);

	if (-1 == rv) {
		VB2_DEBUG("%s: RSA_private_encrypt() failed.\n", __func__);
		free(sig);
		return NULL;
	}

	/* Return the signature */
	return sig;
}
