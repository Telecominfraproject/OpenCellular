/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "2sysincludes.h"
#include "2rsa.h"
#include "file_keys.h"
#include "host_common.h"
#include "vb2_common.h"
#include "vboot_common.h"
#include "test_common.h"


static const uint8_t test_data[] = "This is some test data to sign.";
static const uint32_t test_size = sizeof(test_data);

static void test_unpack_key(const struct vb2_packed_key *key1)
{
	struct vb2_public_key pubk;

	/*
	 * Key data follows the header for a newly allocated key, so we can
	 * calculate the buffer size by looking at how far the key data goes.
	 */
	uint32_t size = key1->key_offset + key1->key_size;
	uint8_t *buf = malloc(size);
	struct vb2_packed_key *key = (struct vb2_packed_key *)buf;

	memcpy(key, key1, size);
	TEST_SUCC(vb2_unpack_key(&pubk, buf, size), "vb2_unpack_key() ok");

	TEST_EQ(pubk.sig_alg, vb2_crypto_to_signature(key->algorithm),
		"vb2_unpack_key() sig_alg");
	TEST_EQ(pubk.hash_alg, vb2_crypto_to_hash(key->algorithm),
		"vb2_unpack_key() hash_alg");


	memcpy(key, key1, size);
	key->algorithm = VB2_ALG_COUNT;
	TEST_EQ(vb2_unpack_key(&pubk, buf, size),
		VB2_ERROR_UNPACK_KEY_SIG_ALGORITHM,
		"vb2_unpack_key() invalid algorithm");

	memcpy(key, key1, size);
	key->key_size--;
	TEST_EQ(vb2_unpack_key(&pubk, buf, size),
		VB2_ERROR_UNPACK_KEY_SIZE,
		"vb2_unpack_key() invalid size");

	memcpy(key, key1, size);
	key->key_offset++;
	TEST_EQ(vb2_unpack_key(&pubk, buf, size + 1),
		VB2_ERROR_UNPACK_KEY_ALIGN,
		"vb2_unpack_key() unaligned data");

	memcpy(key, key1, size);
	*(uint32_t *)(buf + key->key_offset) /= 2;
	TEST_EQ(vb2_unpack_key(&pubk, buf, size),
		VB2_ERROR_UNPACK_KEY_ARRAY_SIZE,
		"vb2_unpack_key() invalid key array size");

	memcpy(key, key1, size);
	TEST_EQ(vb2_unpack_key(&pubk, buf, size - 1),
		VB2_ERROR_INSIDE_DATA_OUTSIDE,
		"vb2_unpack_key() buffer too small");

	free(key);
}

static void test_verify_data(const struct vb2_packed_key *key1,
			     const struct vb2_signature *sig)
{
	uint8_t workbuf[VB2_VERIFY_DATA_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;

	uint32_t pubkey_size = key1->key_offset + key1->key_size;
	struct vb2_public_key pubk, pubk_orig;
	uint32_t sig_total_size = sig->sig_offset + sig->sig_size;
	struct vb2_signature *sig2;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Allocate signature copy for tests */
	sig2 = (struct vb2_signature *)malloc(sig_total_size);

	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key1, pubkey_size),
		0, "vb2_verify_data() unpack key");
	pubk_orig = pubk;

	memcpy(sig2, sig, sig_total_size);
	pubk.sig_alg = VB2_SIG_INVALID;
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		 0, "vb2_verify_data() bad sig alg");
	pubk.sig_alg = pubk_orig.sig_alg;

	memcpy(sig2, sig, sig_total_size);
	pubk.hash_alg = VB2_HASH_INVALID;
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		 0, "vb2_verify_data() bad hash alg");
	pubk.hash_alg = pubk_orig.hash_alg;

	vb2_workbuf_init(&wb, workbuf, 4);
	memcpy(sig2, sig, sig_total_size);
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		 0, "vb2_verify_data() workbuf too small");
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	memcpy(sig2, sig, sig_total_size);
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		0, "vb2_verify_data() ok");

	memcpy(sig2, sig, sig_total_size);
	sig2->sig_size -= 16;
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		 0, "vb2_verify_data() wrong sig size");

	memcpy(sig2, sig, sig_total_size);
	TEST_NEQ(vb2_verify_data(test_data, test_size - 1, sig2, &pubk, &wb),
		 0, "vb2_verify_data() input buffer too small");

	memcpy(sig2, sig, sig_total_size);
	vb2_signature_data(sig2)[0] ^= 0x5A;
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		 0, "vb2_verify_data() wrong sig");

	free(sig2);
}


int test_algorithm(int key_algorithm, const char *keys_dir)
{
	char filename[1024];
	int rsa_len = siglen_map[key_algorithm] * 8;

	VbPrivateKey *private_key = NULL;
	struct vb2_signature *sig = NULL;
	struct vb2_packed_key *key1;

	printf("***Testing algorithm: %s\n", algo_strings[key_algorithm]);

	sprintf(filename, "%s/key_rsa%d.pem", keys_dir, rsa_len);
	private_key = PrivateKeyReadPem(filename, key_algorithm);
	if (!private_key) {
		fprintf(stderr, "Error reading private_key: %s\n", filename);
		return 1;
	}

	sprintf(filename, "%s/key_rsa%d.keyb", keys_dir, rsa_len);
	key1 = (struct vb2_packed_key *)
		PublicKeyReadKeyb(filename, key_algorithm, 1);
	if (!key1) {
		fprintf(stderr, "Error reading public_key: %s\n", filename);
		return 1;
	}

	/* Calculate good signatures */
	sig = (struct vb2_signature *)
		CalculateSignature(test_data, sizeof(test_data), private_key);
	TEST_PTR_NEQ(sig, 0, "Calculate signature");
	if (!sig)
		return 1;

	test_unpack_key(key1);
	test_verify_data(key1, sig);

	free(key1);
	free(private_key);
	free(sig);

	return 0;
}

/* Test only the algorithms we use */
const int key_algs[] = {
	VB2_ALG_RSA2048_SHA256,
	VB2_ALG_RSA4096_SHA256,
	VB2_ALG_RSA8192_SHA512,
};

int main(int argc, char *argv[]) {

	if (argc == 2) {
		int i;

		for (i = 0; i < ARRAY_SIZE(key_algs); i++) {
			if (test_algorithm(key_algs[i], argv[1]))
				return 1;
		}

	} else if (argc == 3 && !strcasecmp(argv[2], "--all")) {
		/* Test all the algorithms */
		int alg;

		for (alg = 0; alg < kNumAlgorithms; alg++) {
			if (test_algorithm(alg, argv[1]))
				return 1;
		}

	} else {
		fprintf(stderr, "Usage: %s <keys_dir> [--all]", argv[0]);
		return -1;
	}

	return gTestSuccess ? 0 : 255;
}
