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
#include "2common.h"
#include "2rsa.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_signature2.h"
#include "test_common.h"


static const uint8_t test_data[] = "This is some test data to sign.";
static const uint32_t test_size = sizeof(test_data);

static void test_unpack_key(const struct vb2_packed_key *key)
{
	struct vb2_public_key pubk;
	struct vb2_packed_key *key2;
	uint32_t size = key->c.total_size;

	/* Make a copy of the key for testing */
	key2 = (struct vb2_packed_key *)malloc(size);

	memcpy(key2, key, size);
	TEST_SUCC(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		  "vb2_unpack_key() ok");

	memcpy(key2, key, size);
	key2->key_offset += 4;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_COMMON_MEMBER_SIZE,
		"vb2_unpack_key() buffer too small");

	memcpy(key2, key, size);
	key2->c.fixed_size += size;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_COMMON_FIXED_SIZE,
		"vb2_unpack_key() buffer too small for desc");

	memcpy(key2, key, size);
	key2->c.desc_size = 0;
	TEST_SUCC(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		  "vb2_unpack_key() no desc");
	TEST_EQ(strcmp(pubk.desc, ""), 0, "  empty desc string");

	memcpy(key2, key, size);
	key2->c.magic++;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_UNPACK_KEY_MAGIC,
		"vb2_unpack_key() bad magic");

	memcpy(key2, key, size);
	key2->c.struct_version_major++;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_UNPACK_KEY_STRUCT_VERSION,
		"vb2_unpack_key() bad major version");

	/*
	 * Minor version changes are ok.  Note that this test assumes that the
	 * source key struct version is the highest actually known to the
	 * reader.  If the reader does know about minor version + 1 and that
	 * adds fields, this test will likely fail.  But at that point, we
	 * should have already added a test for minor version compatibility to
	 * handle both old and new struct versions, so someone will have
	 * noticed this comment.
	 */
	memcpy(key2, key, size);
	key2->c.struct_version_minor++;
	TEST_SUCC(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		  "vb2_unpack_key() minor version change ok");

	memcpy(key2, key, size);
	key2->sig_alg = VB2_SIG_INVALID;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_UNPACK_KEY_SIG_ALGORITHM,
		"vb2_unpack_key() bad sig algorithm");

	memcpy(key2, key, size);
	key2->hash_alg = VB2_HASH_INVALID;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_UNPACK_KEY_HASH_ALGORITHM,
		"vb2_unpack_key() bad hash algorithm");

	memcpy(key2, key, size);
	key2->key_size -= 4;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_UNPACK_KEY_SIZE,
		"vb2_unpack_key() invalid size");

	memcpy(key2, key, size);
	key2->key_offset--;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_COMMON_MEMBER_UNALIGNED,
		"vb2_unpack_key() unaligned data");

	memcpy(key2, key, size);
	*(uint32_t *)((uint8_t *)key2 + key2->key_offset) /= 2;
	TEST_EQ(vb2_unpack_key(&pubk, (uint8_t *)key2, size),
		VB2_ERROR_UNPACK_KEY_ARRAY_SIZE,
		"vb2_unpack_key() invalid key array size");

	free(key2);
}

static void test_verify_signature(const struct vb2_signature *sig)
{
	struct vb2_signature *sig2;
	uint8_t *buf2;
	uint32_t size;

	/* Make a copy of the signature */
	size = sig->c.total_size;
	buf2 = malloc(size);
	sig2 = (struct vb2_signature *)buf2;

	memcpy(buf2, sig, size);
	TEST_SUCC(vb2_verify_signature(sig2, size), "verify_sig ok");
	sig2->c.magic = VB2_MAGIC_PACKED_KEY;
	TEST_EQ(vb2_verify_signature(sig2, size), VB2_ERROR_SIG_MAGIC,
		"verify_sig magic");

	memcpy(buf2, sig, size);
	sig2->c.total_size += 4;
	TEST_EQ(vb2_verify_signature(sig2, size), VB2_ERROR_COMMON_TOTAL_SIZE,
		"verify_sig common header");

	memcpy(buf2, sig, size);
	sig2->c.struct_version_minor++;
	TEST_SUCC(vb2_verify_signature(sig2, size), "verify_sig minor ver");
	sig2->c.struct_version_major++;
	TEST_EQ(vb2_verify_signature(sig2, size), VB2_ERROR_SIG_VERSION,
		"verify_sig major ver");

	memcpy(buf2, sig, size);
	sig2->c.fixed_size -= 4;
	sig2->c.desc_size += 4;
	TEST_EQ(vb2_verify_signature(sig2, size), VB2_ERROR_SIG_HEADER_SIZE,
		"verify_sig header size");

	memcpy(buf2, sig, size);
	sig2->sig_size += 4;
	TEST_EQ(vb2_verify_signature(sig2, size), VB2_ERROR_COMMON_MEMBER_SIZE,
		"verify_sig sig size");

	memcpy(buf2, sig, size);
	sig2->sig_alg = VB2_SIG_INVALID;
	TEST_EQ(vb2_verify_signature(sig2, size), VB2_ERROR_SIG_ALGORITHM,
		"verify_sig sig alg");

	memcpy(buf2, sig, size);
	sig2->sig_alg = (sig2->sig_alg == VB2_SIG_NONE ?
			 VB2_SIG_RSA1024 : VB2_SIG_NONE);
	TEST_EQ(vb2_verify_signature(sig2, size), VB2_ERROR_SIG_SIZE,
		"verify_sig sig size");

	free(buf2);
}

static void test_verify_data(const struct vb2_public_key *pubk_orig,
			      const struct vb2_signature *sig)
{
	uint8_t workbuf[VB2_VERIFY_DATA_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;

	struct vb2_public_key pubk;
	struct vb2_signature *sig2;
	uint8_t *buf2;
	uint32_t size;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	pubk = *pubk_orig;

	/* Allocate signature copy for tests */
	size = sig->c.total_size;
	buf2 = malloc(size);
	sig2 = (struct vb2_signature *)buf2;

	memcpy(buf2, sig, size);
	pubk.sig_alg = VB2_SIG_INVALID;
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		VB2_ERROR_VDATA_ALGORITHM, "vb2_verify_data() bad sig alg");
	pubk = *pubk_orig;

	memcpy(buf2, sig, size);
	pubk.hash_alg = VB2_HASH_INVALID;
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		VB2_ERROR_VDATA_DIGEST_SIZE,
		"vb2_verify_data() bad hash alg");
	pubk = *pubk_orig;

	vb2_workbuf_init(&wb, workbuf, 4);
	memcpy(buf2, sig, size);
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		VB2_ERROR_VDATA_WORKBUF_DIGEST,
		"vb2_verify_data() workbuf too small");
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	memcpy(buf2, sig, size);
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		0, "vb2_verify_data() ok");

	memcpy(buf2, sig, size);
	sig2->sig_size -= 16;
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		VB2_ERROR_VDATA_SIG_SIZE, "vb2_verify_data() wrong sig size");

	memcpy(buf2, sig, size);
	TEST_EQ(vb2_verify_data(test_data, test_size - 1, sig2, &pubk, &wb),
		VB2_ERROR_VDATA_SIZE, "vb2_verify_data() wrong data size");

	memcpy(buf2, sig, size);
	sig2->hash_alg = (sig2->hash_alg == VB2_HASH_SHA1 ?
			  VB2_HASH_SHA256 : VB2_HASH_SHA1);
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		VB2_ERROR_VDATA_ALGORITHM_MISMATCH,
		"vb2_verify_data() alg mismatch");


	memcpy(buf2, sig, size);
	buf2[sig2->sig_offset] ^= 0x5A;
	TEST_EQ(vb2_verify_data(test_data, test_size, sig2, &pubk, &wb),
		VB2_ERROR_RSA_PADDING, "vb2_verify_data() wrong sig");

	free(buf2);
}

int test_algorithm(int key_algorithm, const char *keys_dir)
{
	char filename[1024];
	int rsa_len = siglen_map[key_algorithm] * 8;

	enum vb2_signature_algorithm sig_alg =
		vb2_crypto_to_signature(key_algorithm);
	enum vb2_hash_algorithm hash_alg = vb2_crypto_to_hash(key_algorithm);

	struct vb2_private_key *prik = NULL;
	struct vb2_signature *sig2 = NULL;
	struct vb2_public_key *pubk = NULL;
	struct vb2_packed_key *key2 = NULL;

	printf("***Testing algorithm: %s\n", algo_strings[key_algorithm]);

	sprintf(filename, "%s/key_rsa%d.pem", keys_dir, rsa_len);
	TEST_SUCC(vb2_private_key_read_pem(&prik, filename),
		  "Read private key");
	prik->hash_alg = hash_alg;
	prik->sig_alg = sig_alg;
	vb2_private_key_set_desc(prik, "private key");

	sprintf(filename, "%s/key_rsa%d.keyb", keys_dir, rsa_len);
	TEST_SUCC(vb2_public_key_read_keyb(&pubk, filename),
		  "Read public key");
	pubk->hash_alg = hash_alg;
	vb2_public_key_set_desc(pubk, "public key");
	TEST_SUCC(vb2_public_key_pack(&key2, pubk), "Pack public key");

	/* Calculate good signatures */
	TEST_SUCC(vb2_sign_data(&sig2, test_data, test_size, prik, ""),
		  "Make test signature");

	test_unpack_key(key2);
	test_verify_data(pubk, sig2);
	test_verify_signature(sig2);

	free(key2);
	free(sig2);
	vb2_private_key_free(prik);
	vb2_public_key_free(pubk);

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
