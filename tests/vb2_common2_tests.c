/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "file_keys.h"
#include "host_common.h"
#include "vboot_common.h"
#include "test_common.h"

#include "2common.h"
#include "2rsa.h"

static void test_unpack_key(const VbPublicKey *orig_key)
{
	struct vb2_public_key rsa;
	VbPublicKey *key = PublicKeyAlloc(orig_key->key_size, 0, 0);

	/* vb2_packed_key and VbPublicKey are bit-identical */
	struct vb2_packed_key *key2 = (struct vb2_packed_key *)key;
	uint8_t *buf = (uint8_t *)key;

	/*
	 * Key data follows the header for a newly allocated key, so we can
	 * calculate the buffer size by looking at how far the key data goes.
	 */
	uint32_t size = key2->key_offset + key2->key_size;

	PublicKeyCopy(key, orig_key);
	TEST_SUCC(vb2_unpack_key(&rsa, buf, size), "vb2_unpack_key() ok");

	TEST_EQ(rsa.algorithm, key2->algorithm, "vb2_unpack_key() algorithm");
	TEST_EQ(rsa.hash_alg, vb2_crypto_to_hash(key2->algorithm),
		"vb2_unpack_key() hash_alg");


	PublicKeyCopy(key, orig_key);
	key2->algorithm = VB2_ALG_COUNT;
	TEST_EQ(vb2_unpack_key(&rsa, buf, size),
		VB2_ERROR_UNPACK_KEY_ALGORITHM,
		"vb2_unpack_key() invalid algorithm");

	PublicKeyCopy(key, orig_key);
	key2->key_size--;
	TEST_EQ(vb2_unpack_key(&rsa, buf, size),
		VB2_ERROR_UNPACK_KEY_SIZE,
		"vb2_unpack_key() invalid size");
	key2->key_size++;

	PublicKeyCopy(key, orig_key);
	key2->key_offset++;
	TEST_EQ(vb2_unpack_key(&rsa, buf, size + 1),
		VB2_ERROR_UNPACK_KEY_ALIGN,
		"vb2_unpack_key() unaligned data");
	key2->key_offset--;

	PublicKeyCopy(key, orig_key);
	*(uint32_t *)(buf + key2->key_offset) /= 2;
	TEST_EQ(vb2_unpack_key(&rsa, buf, size),
		VB2_ERROR_UNPACK_KEY_ARRAY_SIZE,
		"vb2_unpack_key() invalid key array size");

	PublicKeyCopy(key, orig_key);
	TEST_EQ(vb2_unpack_key(&rsa, buf, size - 1),
		VB2_ERROR_INSIDE_DATA_OUTSIDE,
		"vb2_unpack_key() buffer too small");

	free(key);
}

static void test_verify_data(const VbPublicKey *public_key,
			     const VbPrivateKey *private_key)
{
	const uint8_t test_data[] = "This is some test data to sign.";
	const uint64_t test_size = sizeof(test_data);
	uint8_t workbuf[VB2_VERIFY_DATA_WORKBUF_BYTES];
	struct vb2_workbuf wb;
	VbSignature *sig;

	struct vb2_public_key rsa;
	struct vb2_signature *sig2;

	struct vb2_packed_key *public_key2;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Vb2 structs are bit-identical to the old ones */
	public_key2 = (struct vb2_packed_key *)public_key;
	uint32_t pubkey_size = public_key2->key_offset + public_key2->key_size;

	/* Calculate good signature */
	sig = CalculateSignature(test_data, test_size, private_key);
	TEST_PTR_NEQ(sig, 0, "VerifyData() calculate signature");
	if (!sig)
		return;

	/* Allocate signature copy for tests */
	sig2 = (struct vb2_signature *)
		SignatureAlloc(siglen_map[public_key2->algorithm], 0);

	TEST_EQ(vb2_unpack_key(&rsa, (uint8_t *)public_key2, pubkey_size),
		0, "vb2_verify_data() unpack key");

	memcpy(sig2, sig, sizeof(VbSignature) + sig->sig_size);
	rsa.algorithm += VB2_ALG_COUNT;
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &rsa,
				 &wb),
		 0, "vb2_verify_data() bad key");
	rsa.algorithm -= VB2_ALG_COUNT;

	vb2_workbuf_init(&wb, workbuf, 4);
	memcpy(sig2, sig, sizeof(VbSignature) + sig->sig_size);
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &rsa, &wb),
		 0, "vb2_verify_data() workbuf too small");
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	memcpy(sig2, sig, sizeof(VbSignature) + sig->sig_size);
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &rsa, &wb),
		0, "vb2_verify_data() ok");

	memcpy(sig2, sig, sizeof(VbSignature) + sig->sig_size);
	sig2->sig_size -= 16;
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &rsa, &wb),
		 0, "vb2_verify_data() wrong sig size");

	memcpy(sig2, sig, sizeof(VbSignature) + sig->sig_size);
	TEST_NEQ(vb2_verify_data(test_data, test_size - 1, sig2, &rsa, &wb),
		 0, "vb2_verify_data() input buffer too small");

	memcpy(sig2, sig, sizeof(VbSignature) + sig->sig_size);
	vb2_signature_data(sig2)[0] ^= 0x5A;
	TEST_NEQ(vb2_verify_data(test_data, test_size, sig2, &rsa, &wb),
		 0, "vb2_verify_data() wrong sig");

	free(sig);
	free(sig2);
}

int test_algorithm(int key_algorithm, const char *keys_dir)
{
	char filename[1024];
	int rsa_len = siglen_map[key_algorithm] * 8;

	VbPrivateKey *private_key = NULL;
	VbPublicKey *public_key = NULL;

	printf("***Testing algorithm: %s\n", algo_strings[key_algorithm]);

	sprintf(filename, "%s/key_rsa%d.pem", keys_dir, rsa_len);
	private_key = PrivateKeyReadPem(filename, key_algorithm);
	if (!private_key) {
		fprintf(stderr, "Error reading private_key: %s\n", filename);
		return 1;
	}

	sprintf(filename, "%s/key_rsa%d.keyb", keys_dir, rsa_len);
	public_key = PublicKeyReadKeyb(filename, key_algorithm, 1);
	if (!public_key) {
		fprintf(stderr, "Error reading public_key: %s\n", filename);
		return 1;
	}

	test_unpack_key(public_key);
	test_verify_data(public_key, private_key);

	if (public_key)
		free(public_key);
	if (private_key)
		free(private_key);

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
