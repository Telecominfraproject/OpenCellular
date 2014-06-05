/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdint.h>
#include <stdio.h>

#define _STUB_IMPLEMENTATION_

#include "cryptolib.h"
#include "file_keys.h"
#include "rsa_padding_test.h"
#include "test_common.h"
#include "utility.h"
#include "vboot_api.h"

#include "2common.h"
#include "2rsa.h"

/*
 * Internal functions from 2rsa.c that have error conditions we can't trigger
 * from the public APIs.  These include checks for bad algorithms where the
 * next call level up already checks for bad algorithms, etc.
 *
 * These functions aren't in 2rsa.h because they're not part of the public
 * APIs.
 */
int vb2_mont_ge(const struct vb2_public_key *key, uint32_t *a);
int vb2_check_padding(uint8_t *sig, int algorithm);
int vb2_safe_memcmp(const void *s1, const void *s2, size_t size);

/**
 * Test RSA utility funcs
 */
static void test_utils(void)
{
	/* Verify old and new algorithm count constants match */
	TEST_EQ(kNumAlgorithms, VB2_ALG_COUNT, "Algorithm counts");

	/* Sig size */
	TEST_EQ(vb2_rsa_sig_size(VB2_ALG_RSA1024_SHA1), RSA1024NUMBYTES,
		"Sig size VB2_ALG_RSA1024_SHA1");
	TEST_EQ(vb2_rsa_sig_size(VB2_ALG_RSA2048_SHA1), RSA2048NUMBYTES,
		"Sig size VB2_ALG_RSA2048_SHA1");
	TEST_EQ(vb2_rsa_sig_size(VB2_ALG_RSA4096_SHA256), RSA4096NUMBYTES,
		"Sig size VB2_ALG_RSA4096_SHA256");
	TEST_EQ(vb2_rsa_sig_size(VB2_ALG_RSA8192_SHA512), RSA8192NUMBYTES,
		"Sig size VB2_ALG_RSA8192_SHA512");
	TEST_EQ(vb2_rsa_sig_size(VB2_ALG_COUNT), 0,
		"Sig size invalid algorithm");

	/* Packed key size */
	TEST_EQ(vb2_packed_key_size(VB2_ALG_RSA1024_SHA1),
		RSA1024NUMBYTES * 2 + sizeof(uint32_t) * 2,
		"Packed key size VB2_ALG_RSA1024_SHA1");
	TEST_EQ(vb2_packed_key_size(VB2_ALG_RSA2048_SHA1),
		RSA2048NUMBYTES * 2 + sizeof(uint32_t) * 2,
		"Packed key size VB2_ALG_RSA2048_SHA1");
	TEST_EQ(vb2_packed_key_size(VB2_ALG_RSA4096_SHA256),
		RSA4096NUMBYTES * 2 + sizeof(uint32_t) * 2,
		"Packed key size VB2_ALG_RSA4096_SHA256");
	TEST_EQ(vb2_packed_key_size(VB2_ALG_RSA8192_SHA512),
		RSA8192NUMBYTES * 2 + sizeof(uint32_t) * 2,
		"Packed key size VB2_ALG_RSA8192_SHA512");
	TEST_EQ(vb2_packed_key_size(VB2_ALG_COUNT), 0,
		"Packed key size invalid algorithm");

	uint8_t sig[RSA1024NUMBYTES];

	/* Test padding check with bad algorithm */
	Memcpy(sig, signatures[0], sizeof(sig));
	TEST_EQ(vb2_check_padding(sig, VB2_ALG_COUNT),
		VB2_ERROR_RSA_PADDING_ALGORITHM,
		"vb2_check_padding() bad alg");

	/* Test safe memcmp */
	TEST_EQ(vb2_safe_memcmp("foo", "foo", 3), 0, "vb2_safe_memcmp() good");
	TEST_NEQ(vb2_safe_memcmp("foo", "bar", 3), 0, "vb2_safe_memcmp() bad");
	TEST_EQ(vb2_safe_memcmp("foo", "bar", 0), 0, "vb2_safe_memcmp() zero");

	/* Test Montgomery >= */
	{
		uint32_t n[4] = {4, 4, 4, 4};
		uint32_t a[4] = {4, 4, 4, 4};
		struct vb2_public_key k = {
			.arrsize = 4,
			.n = n,
		};
		TEST_EQ(vb2_mont_ge(&k, a), 1, "mont_ge equal");

		a[2] = 3;
		TEST_EQ(vb2_mont_ge(&k, a), 0, "mont_ge less");

		a[1] = 5;
		TEST_EQ(vb2_mont_ge(&k, a), 0, "mont_ge greater");
	}
}

int main(int argc, char* argv[])
{
	/* Run tests */
	test_utils();

	return gTestSuccess ? 0 : 255;
}
