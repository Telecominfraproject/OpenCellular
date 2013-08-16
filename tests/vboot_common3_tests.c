/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "host_common.h"
#include "test_common.h"
#include "vboot_common.h"


static void ReChecksumKeyBlock(VbKeyBlockHeader *h)
{
	uint8_t *newchk = DigestBuf((const uint8_t *)h,
				    h->key_block_checksum.data_size,
				    SHA512_DIGEST_ALGORITHM);
	Memcpy(GetSignatureData(&h->key_block_checksum), newchk,
	       SHA512_DIGEST_SIZE);
	VbExFree(newchk);
}

static void KeyBlockVerifyTest(const VbPublicKey *public_key,
                               const VbPrivateKey *private_key,
                               const VbPublicKey *data_key)
{
	VbKeyBlockHeader *hdr;
	VbKeyBlockHeader *h;
	unsigned hsize;

	hdr = KeyBlockCreate(data_key, private_key, 0x1234);
	TEST_NEQ((size_t)hdr, 0, "KeyBlockVerify() prerequisites");
	if (!hdr)
		return;
	hsize = (unsigned) hdr->key_block_size;
	h = (VbKeyBlockHeader *)malloc(hsize + 1024);

	TEST_EQ(KeyBlockVerify(hdr, hsize, NULL, 1), 0,
		"KeyBlockVerify() ok using checksum");
	TEST_EQ(KeyBlockVerify(hdr, hsize, public_key, 0), 0,
		"KeyBlockVerify() ok using key");
	TEST_NEQ(KeyBlockVerify(hdr, hsize, NULL, 0), 0,
		 "KeyBlockVerify() missing key");

	TEST_NEQ(KeyBlockVerify(hdr, hsize - 1, NULL, 1), 0,
		 "KeyBlockVerify() size--");
	TEST_EQ(KeyBlockVerify(hdr, hsize + 1, NULL, 1), 0,
		"KeyBlockVerify() size++");

	Memcpy(h, hdr, hsize);
	h->magic[0] &= 0x12;
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() magic");

	/* Care about major version but not minor */
	Memcpy(h, hdr, hsize);
	h->header_version_major++;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() major++");

	Memcpy(h, hdr, hsize);
	h->header_version_major--;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() major--");

	Memcpy(h, hdr, hsize);
	h->header_version_minor++;
	ReChecksumKeyBlock(h);
	TEST_EQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		"KeyBlockVerify() minor++");

	Memcpy(h, hdr, hsize);
	h->header_version_minor--;
	ReChecksumKeyBlock(h);
	TEST_EQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		"KeyBlockVerify() minor--");

	/* Check hash */
	Memcpy(h, hdr, hsize);
	h->key_block_checksum.sig_offset = hsize;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() checksum off end");

	Memcpy(h, hdr, hsize);
	h->key_block_checksum.sig_size /= 2;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() checksum too small");

	Memcpy(h, hdr, hsize);
	GetPublicKeyData(&h->data_key)[0] ^= 0x34;
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() checksum mismatch");

	/* Check signature */
	Memcpy(h, hdr, hsize);
	h->key_block_signature.sig_offset = hsize;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, public_key, 0), 0,
		 "KeyBlockVerify() sig off end");

	Memcpy(h, hdr, hsize);
	h->key_block_signature.sig_size--;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, public_key, 0), 0,
		 "KeyBlockVerify() sig too small");

	Memcpy(h, hdr, hsize);
	GetPublicKeyData(&h->data_key)[0] ^= 0x34;
	TEST_NEQ(KeyBlockVerify(h, hsize, public_key, 0), 0,
		 "KeyBlockVerify() sig mismatch");

	Memcpy(h, hdr, hsize);
	h->key_block_checksum.data_size = h->key_block_size + 1;
	TEST_NEQ(KeyBlockVerify(h, hsize, public_key, 1), 0,
		 "KeyBlockVerify() checksum data past end of block");

	/* Check that we signed header and data key */
	Memcpy(h, hdr, hsize);
	h->key_block_checksum.data_size = 4;
	h->data_key.key_offset = 0;
	h->data_key.key_size = 0;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() didn't sign header");

	Memcpy(h, hdr, hsize);
	h->data_key.key_offset = hsize;
	ReChecksumKeyBlock(h);
	TEST_NEQ(KeyBlockVerify(h, hsize, NULL, 1), 0,
		 "KeyBlockVerify() data key off end");

	/* Corner cases for error checking */
	TEST_NEQ(KeyBlockVerify(NULL, 4, NULL, 1), 0,
		 "KeyBlockVerify size too small");

	/*
	 * TODO: verify parser can support a bigger header (i.e., one where
	 * data_key.key_offset is bigger than expected).
	 */

	free(h);
	free(hdr);
}

static void ReSignFirmwarePreamble(VbFirmwarePreambleHeader *h,
                                   const VbPrivateKey *key)
{
	VbSignature *sig = CalculateSignature(
		      (const uint8_t *)h, h->preamble_signature.data_size, key);

	SignatureCopy(&h->preamble_signature, sig);
	free(sig);
}

static void VerifyFirmwarePreambleTest(const VbPublicKey *public_key,
                                       const VbPrivateKey *private_key,
                                       const VbPublicKey *kernel_subkey)
{
	VbFirmwarePreambleHeader *hdr;
	VbFirmwarePreambleHeader *h;
	RSAPublicKey *rsa;
	unsigned hsize;

	/* Create a dummy signature */
	VbSignature* body_sig = SignatureAlloc(56, 78);

	rsa = PublicKeyToRSA(public_key);
	hdr = CreateFirmwarePreamble(0x1234, kernel_subkey, body_sig,
				     private_key, 0x5678);
	TEST_NEQ(hdr && rsa, 0, "VerifyFirmwarePreamble() prerequisites");
	if (!hdr)
		return;
	hsize = (unsigned) hdr->preamble_size;
	h = (VbFirmwarePreambleHeader *)malloc(hsize + 16384);

	TEST_EQ(VerifyFirmwarePreamble(hdr, hsize, rsa), 0,
		"VerifyFirmwarePreamble() ok using key");
	TEST_NEQ(VerifyFirmwarePreamble(hdr, 4, rsa), 0,
		 "VerifyFirmwarePreamble() size tiny");
	TEST_NEQ(VerifyFirmwarePreamble(hdr, hsize - 1, rsa), 0,
		 "VerifyFirmwarePreamble() size--");
	TEST_EQ(VerifyFirmwarePreamble(hdr, hsize + 1, rsa), 0,
		"VerifyFirmwarePreamble() size++");

	/* Care about major version but not minor */
	Memcpy(h, hdr, hsize);
	h->header_version_major++;
	ReSignFirmwarePreamble(h, private_key);
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() major++");

	Memcpy(h, hdr, hsize);
	h->header_version_major--;
	ReSignFirmwarePreamble(h, private_key);
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() major--");

	Memcpy(h, hdr, hsize);
	h->header_version_minor++;
	ReSignFirmwarePreamble(h, private_key);
	TEST_EQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		"VerifyFirmwarePreamble() minor++");

	Memcpy(h, hdr, hsize);
	h->header_version_minor--;
	ReSignFirmwarePreamble(h, private_key);
	TEST_EQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		"VerifyFirmwarePreamble() minor--");

	/* Check signature */
	Memcpy(h, hdr, hsize);
	h->preamble_signature.sig_offset = hsize;
	ReSignFirmwarePreamble(h, private_key);
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() sig off end");

	Memcpy(h, hdr, hsize);
	h->preamble_signature.sig_size--;
	ReSignFirmwarePreamble(h, private_key);
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() sig too small");

	Memcpy(h, hdr, hsize);
	GetPublicKeyData(&h->kernel_subkey)[0] ^= 0x34;
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() sig mismatch");

	/* Check that we signed header, kernel subkey, and body sig */
	Memcpy(h, hdr, hsize);
	h->preamble_signature.data_size = 4;
	h->kernel_subkey.key_offset = 0;
	h->kernel_subkey.key_size = 0;
	h->body_signature.sig_offset = 0;
	h->body_signature.sig_size = 0;
	ReSignFirmwarePreamble(h, private_key);
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() didn't sign header");

	Memcpy(h, hdr, hsize);
	h->kernel_subkey.key_offset = hsize;
	ReSignFirmwarePreamble(h, private_key);
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() kernel subkey off end");

	Memcpy(h, hdr, hsize);
	h->body_signature.sig_offset = hsize;
	ReSignFirmwarePreamble(h, private_key);
	TEST_NEQ(VerifyFirmwarePreamble(h, hsize, rsa), 0,
		 "VerifyFirmwarePreamble() body sig off end");

	/* Check that we return flags properly for new and old structs */
	Memcpy(h, hdr, hsize);
	TEST_EQ(VbGetFirmwarePreambleFlags(h), 0x5678,
		"VbGetFirmwarePreambleFlags() v2.1");
	h->header_version_minor = 0;
	TEST_EQ(VbGetFirmwarePreambleFlags(h), 0,
		"VbGetFirmwarePreambleFlags() v2.0");

	/* TODO: verify with extra padding at end of header. */

	free(h);
	RSAPublicKeyFree(rsa);
	free(hdr);
}

int test_permutation(int signing_key_algorithm, int data_key_algorithm,
		     const char *keys_dir)
{
	char filename[1024];
	int signing_rsa_len = siglen_map[signing_key_algorithm] * 8;
	int data_rsa_len = siglen_map[data_key_algorithm] * 8;

	VbPrivateKey *signing_private_key = NULL;
	VbPublicKey *signing_public_key = NULL;
	VbPublicKey *data_public_key = NULL;

	printf("***Testing signing algorithm: %s\n",
	       algo_strings[signing_key_algorithm]);
	printf("***With data key algorithm: %s\n",
	       algo_strings[data_key_algorithm]);

	sprintf(filename, "%s/key_rsa%d.pem", keys_dir, signing_rsa_len);
	signing_private_key = PrivateKeyReadPem(filename,
						signing_key_algorithm);
	if (!signing_private_key) {
		fprintf(stderr, "Error reading signing_private_key: %s\n",
			filename);
		return 1;
	}

	sprintf(filename, "%s/key_rsa%d.keyb", keys_dir, signing_rsa_len);
	signing_public_key = PublicKeyReadKeyb(filename,
					       signing_key_algorithm, 1);
	if (!signing_public_key) {
		fprintf(stderr, "Error reading signing_public_key: %s\n",
			filename);
		return 1;
	}

	sprintf(filename, "%s/key_rsa%d.keyb", keys_dir, data_rsa_len);
	data_public_key = PublicKeyReadKeyb(filename,
					       data_key_algorithm, 1);
	if (!data_public_key) {
		fprintf(stderr, "Error reading data_public_key: %s\n",
			filename);
		return 1;
	}

	KeyBlockVerifyTest(signing_public_key, signing_private_key,
			   data_public_key);
	VerifyFirmwarePreambleTest(signing_public_key, signing_private_key,
				   data_public_key);

	if (signing_public_key)
		free(signing_public_key);
	if (signing_private_key)
		free(signing_private_key);
	if (data_public_key)
		free(data_public_key);

	return 0;
}

struct test_perm
{
	int signing_algorithm;
	int data_key_algorithm;
};

/*
 * Permutations of signing and data key algorithms in active use:
 * 7 (rsa4096 sha256) - 4 (rsa2048 sha256)
 * 11 (rsa8192 sha512) - 4 (rsa2048 sha256)
 * 11 (rsa8192 sha512) - 7 (rsa4096 sha256)
 */
const struct test_perm test_perms[] = {{7, 4}, {11, 4}, {11, 7}};

int main(int argc, char *argv[])
{
	if (argc == 2) {
		/* Test only the algorithms we use */
		int i;

		for (i = 0; i < ARRAY_SIZE(test_perms); i++) {
			if (test_permutation(test_perms[i].signing_algorithm,
					     test_perms[i].data_key_algorithm,
					     argv[1]))
				return 1;
		}

	} else if (argc == 3 && !strcasecmp(argv[2], "--all")) {
		/* Test all the algorithms */
		int sign_alg, data_alg;

		for (sign_alg = 0; sign_alg < kNumAlgorithms; sign_alg++) {
			for (data_alg = 0; data_alg < kNumAlgorithms;
			     data_alg++) {
				if (test_permutation(sign_alg, data_alg,
						     argv[1]))
					return 1;
			}
		}
	} else {
		fprintf(stderr,	"Usage: %s <keys_dir> [--all]",	argv[0]);
		return -1;
	}

	if (vboot_api_stub_check_memory())
		return 255;

	return gTestSuccess ? 0 : 255;
}
