/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* FIPS 180-2 Tests for message digest functions. */

#include "2sysincludes.h"
#include "2rsa.h"
#include "2sha.h"
#include "2return_codes.h"

#include "sha_test_vectors.h"
#include "test_common.h"

static int vb2_digest(const uint8_t *buf,
		      uint32_t size,
		      enum vb2_hash_algorithm hash_alg,
		      uint8_t *digest,
		      uint32_t digest_size)
{
	struct vb2_digest_context dc;
	int rv;

	rv = vb2_digest_init(&dc, hash_alg);
	if (rv)
		return rv;

	rv = vb2_digest_extend(&dc, buf, size);
	if (rv)
		return rv;

	return vb2_digest_finalize(&dc, digest, digest_size);
}

void sha1_tests(void)
{
	uint8_t digest[VB2_SHA1_DIGEST_SIZE];
	uint8_t *test_inputs[3];
	int i;

	test_inputs[0] = (uint8_t *) oneblock_msg;
	test_inputs[1] = (uint8_t *) multiblock_msg1;
	test_inputs[2] = (uint8_t *) long_msg;

	for (i = 0; i < 3; i++) {
		TEST_SUCC(vb2_digest(test_inputs[i],
				     strlen((char *)test_inputs[i]),
				     VB2_HASH_SHA1, digest, sizeof(digest)),
			  "vb2_digest() SHA1");
		TEST_EQ(memcmp(digest, sha1_results[i], sizeof(digest)),
			0, "SHA1 digest");
	}

	TEST_EQ(vb2_digest(test_inputs[0], strlen((char *)test_inputs[0]),
			   VB2_HASH_SHA1, digest, sizeof(digest) - 1),
		VB2_ERROR_SHA_FINALIZE_DIGEST_SIZE, "vb2_digest() too small");
}

void sha256_tests(void)
{
	uint8_t digest[VB2_SHA256_DIGEST_SIZE];
	uint8_t *test_inputs[3];
	int i;

	test_inputs[0] = (uint8_t *) oneblock_msg;
	test_inputs[1] = (uint8_t *) multiblock_msg1;
	test_inputs[2] = (uint8_t *) long_msg;

	for (i = 0; i < 3; i++) {
		TEST_SUCC(vb2_digest(test_inputs[i],
				     strlen((char *)test_inputs[i]),
				     VB2_HASH_SHA256, digest, sizeof(digest)),
			  "vb2_digest() SHA256");
		TEST_EQ(memcmp(digest, sha256_results[i], sizeof(digest)),
			0, "SHA-256 digest");
	}

	TEST_EQ(vb2_digest(test_inputs[0], strlen((char *)test_inputs[0]),
			   VB2_HASH_SHA256, digest, sizeof(digest) - 1),
		VB2_ERROR_SHA_FINALIZE_DIGEST_SIZE, "vb2_digest() too small");
}

void sha512_tests(void)
{
	uint8_t digest[VB2_SHA512_DIGEST_SIZE];
	uint8_t *test_inputs[3];
	int i;

	test_inputs[0] = (uint8_t *) oneblock_msg;
	test_inputs[1] = (uint8_t *) multiblock_msg2;
	test_inputs[2] = (uint8_t *) long_msg;

	for (i = 0; i < 3; i++) {
		TEST_SUCC(vb2_digest(test_inputs[i],
				     strlen((char *)test_inputs[i]),
				     VB2_HASH_SHA512, digest,
				     sizeof(digest)),
			  "vb2_digest() SHA512");
		TEST_EQ(memcmp(digest, sha512_results[i], sizeof(digest)),
			0, "SHA-512 digest");
	}

	TEST_EQ(vb2_digest(test_inputs[0], strlen((char *)test_inputs[0]),
			   VB2_HASH_SHA512, digest, sizeof(digest) - 1),
		VB2_ERROR_SHA_FINALIZE_DIGEST_SIZE, "vb2_digest() too small");
}

void misc_tests(void)
{
	uint8_t digest[VB2_SHA512_DIGEST_SIZE];
	struct vb2_digest_context dc;

	/* Crypto algorithm to hash algorithm mapping */
	TEST_EQ(vb2_crypto_to_hash(VB2_ALG_RSA1024_SHA1), VB2_HASH_SHA1,
		"Crypto map to SHA1");
	TEST_EQ(vb2_crypto_to_hash(VB2_ALG_RSA2048_SHA256), VB2_HASH_SHA256,
		"Crypto map to SHA256");
	TEST_EQ(vb2_crypto_to_hash(VB2_ALG_RSA4096_SHA256), VB2_HASH_SHA256,
		"Crypto map to SHA256 2");
	TEST_EQ(vb2_crypto_to_hash(VB2_ALG_RSA8192_SHA512), VB2_HASH_SHA512,
		"Crypto map to SHA512");
	TEST_EQ(vb2_crypto_to_hash(VB2_ALG_COUNT), VB2_HASH_INVALID,
		"Crypto map to invalid");

	TEST_EQ(vb2_digest_size(VB2_HASH_INVALID), 0,
		"digest size invalid alg");

	TEST_EQ(vb2_digest((uint8_t *)oneblock_msg, strlen(oneblock_msg),
			   VB2_HASH_INVALID, digest, sizeof(digest)),
		VB2_ERROR_SHA_INIT_ALGORITHM,
		"vb2_digest() invalid alg");

	/* Test bad algorithm inside extend and finalize */
	vb2_digest_init(&dc, VB2_HASH_SHA256);
	dc.hash_alg = VB2_HASH_INVALID;
	TEST_EQ(vb2_digest_extend(&dc, digest, sizeof(digest)),
		VB2_ERROR_SHA_EXTEND_ALGORITHM,
		"vb2_digest_extend() invalid alg");
	TEST_EQ(vb2_digest_finalize(&dc, digest, sizeof(digest)),
		VB2_ERROR_SHA_FINALIZE_ALGORITHM,
		"vb2_digest_finalize() invalid alg");
}

int main(int argc, char *argv[])
{
	/* Initialize long_msg with 'a' x 1,000,000 */
	long_msg = (char *) malloc(1000001);
	memset(long_msg, 'a', 1000000);
	long_msg[1000000]=0;

	sha1_tests();
	sha256_tests();
	sha512_tests();
	misc_tests();

	free(long_msg);

	return gTestSuccess ? 0 : 255;
}
