/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <openssl/pem.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "2sysincludes.h"

#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "host_common.h"
#include "host_signature2.h"
#include "signature_digest.h"

uint8_t* PrependDigestInfo(enum vb2_hash_algorithm hash_alg, uint8_t* digest)
{
	const int digest_size = vb2_digest_size(hash_alg);
	uint32_t digestinfo_size = 0;
	const uint8_t* digestinfo = NULL;

	if (VB2_SUCCESS != vb2_digest_info(hash_alg, &digestinfo,
					   &digestinfo_size))
		return NULL;

	uint8_t* p = malloc(digestinfo_size + digest_size);
	memcpy(p, digestinfo, digestinfo_size);
	memcpy(p + digestinfo_size, digest, digest_size);
	return p;
}

uint8_t* SignatureDigest(const uint8_t* buf, uint64_t len,
                         unsigned int algorithm)
{
	uint8_t* info_digest  = NULL;

	uint8_t digest[VB2_SHA512_DIGEST_SIZE];  /* Longest digest */
	enum vb2_hash_algorithm hash_alg;

	if (algorithm >= VB2_ALG_COUNT) {
		fprintf(stderr,
			"SignatureDigest(): Called with invalid algorithm!\n");
		return NULL;
	}

	hash_alg = vb2_crypto_to_hash(algorithm);

	if (VB2_SUCCESS == vb2_digest_buffer(buf, len, hash_alg,
					     digest, sizeof(digest))) {
		info_digest = PrependDigestInfo(hash_alg, digest);
	}
	return info_digest;
}

uint8_t* SignatureBuf(const uint8_t* buf, uint64_t len, const char* key_file,
                      unsigned int algorithm)
{
	const enum vb2_hash_algorithm hash_alg = vb2_crypto_to_hash(algorithm);
	FILE* key_fp = NULL;
	RSA* key = NULL;
	uint8_t* signature = NULL;
	uint8_t* signature_digest = SignatureDigest(buf, len, algorithm);
	if (!signature_digest) {
		fprintf(stderr, "SignatureBuf(): "
			"Couldn't get signature digest\n");
		return NULL;
	}

	const int digest_size = vb2_digest_size(hash_alg);

	uint32_t digestinfo_size = 0;
	const uint8_t* digestinfo = NULL;
	if (VB2_SUCCESS != vb2_digest_info(hash_alg, &digestinfo,
					   &digestinfo_size)) {
		fprintf(stderr, "SignatureBuf(): Couldn't get digest info\n");
		free(signature_digest);
		return NULL;
	}

	int signature_digest_len = digest_size + digestinfo_size;

	key_fp  = fopen(key_file, "r");
	if (!key_fp) {
		fprintf(stderr, "SignatureBuf(): Couldn't open key file: %s\n",
			key_file);
		free(signature_digest);
		return NULL;
	}
	if ((key = PEM_read_RSAPrivateKey(key_fp, NULL, NULL, NULL)))
		signature = (uint8_t *)malloc(
		    vb2_rsa_sig_size(vb2_crypto_to_signature(algorithm)));
	else
		fprintf(stderr, "SignatureBuf(): "
			"Couldn't read private key from: %s\n", key_file);
	if (signature) {
		if (-1 == RSA_private_encrypt(
				signature_digest_len,  /* Input length. */
				signature_digest,  /* Input data. */
				signature,  /* Output signature. */
				key,  /* Key to use. */
				RSA_PKCS1_PADDING))  /* Padding to use. */
			fprintf(stderr, "SignatureBuf(): "
				"RSA_private_encrypt() failed.\n");
	}
	fclose(key_fp);
	if (key)
		RSA_free(key);
	free(signature_digest);
	return signature;
}
