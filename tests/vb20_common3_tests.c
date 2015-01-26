/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdio.h>

#include "2sysincludes.h"
#include "2rsa.h"

#include "file_keys.h"
#include "host_common.h"
#include "host_key.h"
#include "host_keyblock.h"
#include "host_signature.h"
#include "vb2_common.h"
#include "vboot_common.h"
#include "test_common.h"

static void resign_keyblock(struct vb2_keyblock *h, const VbPrivateKey *key)
{
	VbSignature *sig =
		CalculateSignature((const uint8_t *)h,
				   h->keyblock_signature.data_size, key);

	SignatureCopy((VbSignature *)&h->keyblock_signature, sig);
	free(sig);
}

static void test_verify_keyblock(const VbPublicKey *public_key,
				 const VbPrivateKey *private_key,
				 const VbPublicKey *data_key)
{
	uint8_t workbuf[VB2_KEY_BLOCK_VERIFY_WORKBUF_BYTES]
		__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;
	struct vb2_public_key key;
	struct vb2_keyblock *hdr;
	struct vb2_keyblock *h;
	uint32_t hsize;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Unpack public key */
	TEST_SUCC(vb2_unpack_key(&key, (uint8_t *)public_key,
				 public_key->key_offset + public_key->key_size),
		  "vb2_verify_keyblock public key");

	hdr = (struct vb2_keyblock *)
		KeyBlockCreate(data_key, private_key, 0x1234);
	TEST_NEQ((size_t)hdr, 0, "vb2_verify_keyblock() prerequisites");
	if (!hdr)
		return;
	hsize = hdr->keyblock_size;
	h = (struct vb2_keyblock *)malloc(hsize + 2048);

	Memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_keyblock(h, hsize, &key, &wb),
		  "vb2_verify_keyblock() ok using key");

	Memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_keyblock(h, hsize - 1, &key, &wb),
		VB2_ERROR_KEYBLOCK_SIZE, "vb2_verify_keyblock() size--");

	/* Buffer is allowed to be bigger than keyblock */
	Memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_keyblock(h, hsize + 1, &key, &wb),
		  "vb2_verify_keyblock() size++");

	Memcpy(h, hdr, hsize);
	h->magic[0] &= 0x12;
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_MAGIC, "vb2_verify_keyblock() magic");

	/* Care about major version but not minor */
	Memcpy(h, hdr, hsize);
	h->header_version_major++;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_HEADER_VERSION,
		"vb2_verify_keyblock() major++");

	Memcpy(h, hdr, hsize);
	h->header_version_major--;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_HEADER_VERSION,
		"vb2_verify_keyblock() major--");

	Memcpy(h, hdr, hsize);
	h->header_version_minor++;
	resign_keyblock(h, private_key);
	TEST_SUCC(vb2_verify_keyblock(h, hsize, &key, &wb),
		  "vb2_verify_keyblock() minor++");

	Memcpy(h, hdr, hsize);
	h->header_version_minor--;
	resign_keyblock(h, private_key);
	TEST_SUCC(vb2_verify_keyblock(h, hsize, &key, &wb),
		  "vb2_verify_keyblock() minor--");

	/* Check signature */
	Memcpy(h, hdr, hsize);
	h->keyblock_signature.sig_offset = hsize;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_SIG_OUTSIDE,
		"vb2_verify_keyblock() sig off end");

	Memcpy(h, hdr, hsize);
	h->keyblock_signature.sig_size--;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_SIG_INVALID,
		"vb2_verify_keyblock() sig too small");

	Memcpy(h, hdr, hsize);
	((uint8_t *)vb2_packed_key_data(&h->data_key))[0] ^= 0x34;
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_SIG_INVALID,
		"vb2_verify_keyblock() sig mismatch");

	Memcpy(h, hdr, hsize);
	h->keyblock_signature.data_size = h->keyblock_size + 1;
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_SIGNED_TOO_MUCH,
		"vb2_verify_keyblock() sig data past end of block");

	/* Check that we signed header and data key */
	Memcpy(h, hdr, hsize);
	h->keyblock_signature.data_size = 4;
	h->data_key.key_offset = 0;
	h->data_key.key_size = 0;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_SIGNED_TOO_LITTLE,
		"vb2_verify_keyblock() didn't sign header");

	Memcpy(h, hdr, hsize);
	h->data_key.key_offset = hsize;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_verify_keyblock(h, hsize, &key, &wb),
		VB2_ERROR_KEYBLOCK_DATA_KEY_OUTSIDE,
		"vb2_verify_keyblock() data key off end");

	/* Corner cases for error checking */
	TEST_EQ(vb2_verify_keyblock(NULL, 4, &key, &wb),
		VB2_ERROR_KEYBLOCK_TOO_SMALL_FOR_HEADER,
		"vb2_verify_keyblock size too small");

	/*
	 * TODO: verify parser can support a bigger header (i.e., one where
	 * data_key.key_offset is bigger than expected).
	 */

	free(h);
	free(hdr);
}

static void resign_fw_preamble(struct vb2_fw_preamble *h,
			       const VbPrivateKey *key)
{
	VbSignature *sig = CalculateSignature(
		(const uint8_t *)h, h->preamble_signature.data_size, key);

	SignatureCopy((VbSignature *)&h->preamble_signature, sig);
	free(sig);
}

static void test_verify_fw_preamble(const VbPublicKey *public_key,
				    const VbPrivateKey *private_key,
				    const VbPublicKey *kernel_subkey)
{
	struct vb2_fw_preamble *hdr;
	struct vb2_fw_preamble *h;
	struct vb2_public_key rsa;
	uint8_t workbuf[VB2_VERIFY_FIRMWARE_PREAMBLE_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;
	uint32_t hsize;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Create a dummy signature */
	VbSignature *body_sig = SignatureAlloc(56, 78);

	TEST_SUCC(vb2_unpack_key(&rsa, (uint8_t *)public_key,
				 public_key->key_offset + public_key->key_size),
		  "vb2_verify_fw_preamble() prereq key");

	hdr = (struct vb2_fw_preamble *)
		CreateFirmwarePreamble(0x1234, kernel_subkey, body_sig,
				       private_key, 0x5678);
	TEST_PTR_NEQ(hdr, NULL,
		     "VerifyFirmwarePreamble() prereq test preamble");
	if (!hdr)
		return;
	hsize = (uint32_t) hdr->preamble_size;
	h = (struct vb2_fw_preamble *)malloc(hsize + 16384);

	Memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_fw_preamble() ok using key");

	Memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_fw_preamble(h, 4, &rsa, &wb),
		VB2_ERROR_PREAMBLE_TOO_SMALL_FOR_HEADER,
		"vb2_verify_fw_preamble() size tiny");

	Memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize - 1, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIZE,
		"vb2_verify_fw_preamble() size--");

	/* Buffer is allowed to be bigger than preamble */
	Memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_fw_preamble(h, hsize + 1, &rsa, &wb),
		  "vb2_verify_fw_preamble() size++");

	/* Care about major version but not minor */
	Memcpy(h, hdr, hsize);
	h->header_version_major++;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_VERSION
		, "vb2_verify_fw_preamble() major++");

	Memcpy(h, hdr, hsize);
	h->header_version_major--;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_VERSION,
		"vb2_verify_fw_preamble() major--");

	Memcpy(h, hdr, hsize);
	h->header_version_minor++;
	resign_fw_preamble(h, private_key);
	TEST_SUCC(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_fw_preamble() minor++");

	Memcpy(h, hdr, hsize);
	h->header_version_minor--;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_OLD,
		"vb2_verify_fw_preamble() 2.0 not supported");

	/* Check signature */
	Memcpy(h, hdr, hsize);
	h->preamble_signature.sig_offset = hsize;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_OUTSIDE,
		"vb2_verify_fw_preamble() sig off end");

	Memcpy(h, hdr, hsize);
	h->preamble_signature.sig_size--;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_INVALID,
		"vb2_verify_fw_preamble() sig too small");

	Memcpy(h, hdr, hsize);
	((uint8_t *)vb2_packed_key_data(&h->kernel_subkey))[0] ^= 0x34;
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_INVALID,
		"vb2_verify_fw_preamble() sig mismatch");

	/* Check that we signed header, kernel subkey, and body sig */
	Memcpy(h, hdr, hsize);
	h->preamble_signature.data_size = 4;
	h->kernel_subkey.key_offset = 0;
	h->kernel_subkey.key_size = 0;
	h->body_signature.sig_offset = 0;
	h->body_signature.sig_size = 0;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIGNED_TOO_LITTLE,
		"vb2_verify_fw_preamble() didn't sign header");

	Memcpy(h, hdr, hsize);
	h->kernel_subkey.key_offset = hsize;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_KERNEL_SUBKEY_OUTSIDE,
		"vb2_verify_fw_preamble() kernel subkey off end");

	Memcpy(h, hdr, hsize);
	h->body_signature.sig_offset = hsize;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_BODY_SIG_OUTSIDE,
		"vb2_verify_fw_preamble() body sig off end");

	/* TODO: verify with extra padding at end of header. */

	free(h);
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

	test_verify_keyblock(signing_public_key, signing_private_key,
			     data_public_key);
	test_verify_fw_preamble(signing_public_key, signing_private_key,
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

/* Permutations of signing and data key algorithms in active use */
const struct test_perm test_perms[] = {
	{VB2_ALG_RSA4096_SHA256, VB2_ALG_RSA2048_SHA256},
	{VB2_ALG_RSA8192_SHA512, VB2_ALG_RSA2048_SHA256},
	{VB2_ALG_RSA8192_SHA512, VB2_ALG_RSA4096_SHA256},
};

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

		for (sign_alg = 0; sign_alg < VB2_ALG_COUNT; sign_alg++) {
			for (data_alg = 0; data_alg < VB2_ALG_COUNT;
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

	return gTestSuccess ? 0 : 255;
}
