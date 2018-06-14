/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* FIPS 180-2 Tests for message digest functions. */

#include <stdio.h>

#include "2sysincludes.h"
#include "2rsa.h"
#include "2sha.h"
#include "2return_codes.h"

#include "sha_test_vectors.h"
#include "test_common.h"

void sha1_tests(void)
{
	uint8_t digest[VB2_SHA1_DIGEST_SIZE];
	uint8_t *test_inputs[3];
	int i;

	test_inputs[0] = (uint8_t *) oneblock_msg;
	test_inputs[1] = (uint8_t *) multiblock_msg1;
	test_inputs[2] = (uint8_t *) long_msg;

	for (i = 0; i < 3; i++) {
		TEST_SUCC(vb2_digest_buffer(test_inputs[i],
					    strlen((char *)test_inputs[i]),
					    VB2_HASH_SHA1,
					    digest, sizeof(digest)),
			  "vb2_digest_buffer() SHA1");
		TEST_EQ(memcmp(digest, sha1_results[i], sizeof(digest)),
			0, "SHA1 digest");
	}

	TEST_EQ(vb2_digest_buffer(test_inputs[0],
				  strlen((char *)test_inputs[0]),
				  VB2_HASH_SHA1, digest, sizeof(digest) - 1),
		VB2_ERROR_SHA_FINALIZE_DIGEST_SIZE,
		"vb2_digest_buffer() too small");

	TEST_EQ(vb2_hash_block_size(VB2_HASH_SHA1), VB2_SHA1_BLOCK_SIZE,
		"vb2_hash_block_size(VB2_HASH_SHA1)");
}

void sha256_tests(void)
{
	uint8_t digest[VB2_SHA256_DIGEST_SIZE];
	uint8_t *test_inputs[3];
	struct vb2_sha256_context ctx;
	const uint8_t expect_multiple[VB2_SHA256_DIGEST_SIZE] = {
			0x07, 0x08, 0xb4, 0xca, 0x46, 0x4c, 0x40, 0x39,
			0x07, 0x06, 0x88, 0x80, 0x30, 0x55, 0x5d, 0x86,
			0x0e, 0x4a, 0x0d, 0x2b, 0xc6, 0xc4, 0x87, 0x39,
			0x2c, 0x16, 0x55, 0xb0, 0x82, 0x13, 0x16, 0x29 };
	const uint8_t extend_from[VB2_SHA256_DIGEST_SIZE] = { 0x00, };
	const uint8_t extend_by[VB2_SHA256_BLOCK_SIZE] = { 0x00, };
	const uint8_t expected_extend[VB2_SHA256_DIGEST_SIZE] = {
		0x7c, 0xa5, 0x16, 0x14, 0x42, 0x5c, 0x3b, 0xa8, 0xce, 0x54,
		0xdd, 0x2f, 0xc2, 0x02, 0x0a, 0xe7, 0xb6, 0xe5, 0x74, 0xd1,
		0x98, 0x13, 0x6d, 0x0f, 0xae, 0x7e, 0x26, 0xcc, 0xbf, 0x0b,
		0xe7, 0xa6 };
	int i;

	test_inputs[0] = (uint8_t *) oneblock_msg;
	test_inputs[1] = (uint8_t *) multiblock_msg1;
	test_inputs[2] = (uint8_t *) long_msg;

	for (i = 0; i < 3; i++) {
		TEST_SUCC(vb2_digest_buffer(test_inputs[i],
					    strlen((char *)test_inputs[i]),
					    VB2_HASH_SHA256,
					    digest, sizeof(digest)),
			  "vb2_digest_buffer() SHA256");
		TEST_EQ(memcmp(digest, sha256_results[i], sizeof(digest)),
			0, "SHA-256 digest");
	}

	TEST_EQ(vb2_digest_buffer(test_inputs[0],
				  strlen((char *)test_inputs[0]),
				  VB2_HASH_SHA256, digest, sizeof(digest) - 1),
		VB2_ERROR_SHA_FINALIZE_DIGEST_SIZE,
		"vb2_digest_buffer() too small");

	/* Test multiple small extends */
	vb2_sha256_init(&ctx);
	vb2_sha256_update(&ctx, (uint8_t *)"test1", 5);
	vb2_sha256_update(&ctx, (uint8_t *)"test2", 5);
	vb2_sha256_update(&ctx, (uint8_t *)"test3", 5);
	vb2_sha256_finalize(&ctx, digest);
	TEST_EQ(memcmp(digest, expect_multiple, sizeof(digest)), 0,
		"SHA-256 multiple extends");

	TEST_EQ(vb2_hash_block_size(VB2_HASH_SHA256), VB2_SHA256_BLOCK_SIZE,
		"vb2_hash_block_size(VB2_HASH_SHA256)");

	/* Test SHA256 hash extend */
	vb2_sha256_extend(extend_from, extend_by, digest);
	TEST_SUCC(memcmp(digest, expected_extend, sizeof(digest)), NULL);
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
		TEST_SUCC(vb2_digest_buffer(test_inputs[i],
					    strlen((char *)test_inputs[i]),
					    VB2_HASH_SHA512,
					    digest, sizeof(digest)),
			  "vb2_digest_buffer() SHA512");
		TEST_EQ(memcmp(digest, sha512_results[i], sizeof(digest)),
			0, "SHA-512 digest");
	}

	TEST_EQ(vb2_digest_buffer(test_inputs[0],
				  strlen((char *)test_inputs[0]),
				  VB2_HASH_SHA512, digest, sizeof(digest) - 1),
		VB2_ERROR_SHA_FINALIZE_DIGEST_SIZE,
		"vb2_digest_buffer() too small");

	TEST_EQ(vb2_hash_block_size(VB2_HASH_SHA512), VB2_SHA512_BLOCK_SIZE,
		"vb2_hash_block_size(VB2_HASH_SHA512)");
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

	TEST_EQ(vb2_hash_block_size(VB2_HASH_INVALID), 0,
		"vb2_hash_block_size(VB2_HASH_INVALID)");

	TEST_EQ(vb2_digest_buffer((uint8_t *)oneblock_msg, strlen(oneblock_msg),
				  VB2_HASH_INVALID, digest, sizeof(digest)),
		VB2_ERROR_SHA_INIT_ALGORITHM,
		"vb2_digest_buffer() invalid alg");

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

static void hash_algorithm_name_tests(void)
{
	enum vb2_hash_algorithm alg;
	char test_name[256];

	for (alg = 1; alg < VB2_HASH_ALG_COUNT; alg++) {
		sprintf(test_name, "%s: %s (alg=%d)",
			__func__, vb2_get_hash_algorithm_name(alg), alg);
		TEST_STR_NEQ(vb2_get_hash_algorithm_name(alg),
			     VB2_INVALID_ALG_NAME, test_name);
	}

	TEST_STR_EQ(vb2_get_hash_algorithm_name(VB2_HASH_INVALID),
		    VB2_INVALID_ALG_NAME, "hash alg name invalid");
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
	hash_algorithm_name_tests();

	free(long_msg);

	return gTestSuccess ? 0 : 255;
}
