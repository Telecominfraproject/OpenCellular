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

static void resign_keyblock(struct vb2_keyblock *h,
			    const struct vb2_private_key *key)
{
	struct vb2_signature *sig =
		vb2_calculate_signature((const uint8_t *)h,
					h->keyblock_signature.data_size, key);

	vb2_copy_signature(&h->keyblock_signature, sig);
	free(sig);
}

static void test_check_keyblock(const struct vb2_public_key *public_key,
				const struct vb2_private_key *private_key,
				const struct vb2_packed_key *data_key)
{
	struct vb2_keyblock *hdr;
	struct vb2_keyblock *h;
	struct vb2_signature *sig;
	uint32_t hsize;

	hdr = vb2_create_keyblock(data_key, private_key, 0x1234);
	TEST_NEQ((size_t)hdr, 0, "vb2_verify_keyblock() prerequisites");
	if (!hdr)
		return;
	hsize = hdr->keyblock_size;
	h = (struct vb2_keyblock *)malloc(hsize + 2048);
	sig = &h->keyblock_signature;

	memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_check_keyblock(h, hsize, sig),
		  "vb2_check_keyblock() ok");

	memcpy(h, hdr, hsize);
	TEST_EQ(vb2_check_keyblock(h, hsize - 1, sig),
		VB2_ERROR_KEYBLOCK_SIZE, "vb2_check_keyblock() size--");

	/* Buffer is allowed to be bigger than keyblock */
	memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_check_keyblock(h, hsize + 1, sig),
		  "vb2_check_keyblock() size++");

	memcpy(h, hdr, hsize);
	h->magic[0] &= 0x12;
	TEST_EQ(vb2_check_keyblock(h, hsize, sig),
		VB2_ERROR_KEYBLOCK_MAGIC, "vb2_check_keyblock() magic");

	/* Care about major version but not minor */
	memcpy(h, hdr, hsize);
	h->header_version_major++;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_check_keyblock(h, hsize, sig),
		VB2_ERROR_KEYBLOCK_HEADER_VERSION,
		"vb2_check_keyblock() major++");

	memcpy(h, hdr, hsize);
	h->header_version_major--;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_check_keyblock(h, hsize, sig),
		VB2_ERROR_KEYBLOCK_HEADER_VERSION,
		"vb2_check_keyblock() major--");

	memcpy(h, hdr, hsize);
	h->header_version_minor++;
	resign_keyblock(h, private_key);
	TEST_SUCC(vb2_check_keyblock(h, hsize, sig),
		  "vb2_check_keyblock() minor++");

	memcpy(h, hdr, hsize);
	h->header_version_minor--;
	resign_keyblock(h, private_key);
	TEST_SUCC(vb2_check_keyblock(h, hsize, sig),
		  "vb2_check_keyblock() minor--");

	/* Check signature */
	memcpy(h, hdr, hsize);
	h->keyblock_signature.sig_offset = hsize;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_check_keyblock(h, hsize, sig),
		VB2_ERROR_KEYBLOCK_SIG_OUTSIDE,
		"vb2_check_keyblock() sig off end");

	memcpy(h, hdr, hsize);
	h->keyblock_signature.data_size = h->keyblock_size + 1;
	TEST_EQ(vb2_check_keyblock(h, hsize, sig),
		VB2_ERROR_KEYBLOCK_SIGNED_TOO_MUCH,
		"vb2_check_keyblock() sig data past end of block");

	/* Check that we signed header and data key */
	memcpy(h, hdr, hsize);
	h->keyblock_signature.data_size = 4;
	h->data_key.key_offset = 0;
	h->data_key.key_size = 0;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_check_keyblock(h, hsize, sig),
		VB2_ERROR_KEYBLOCK_SIGNED_TOO_LITTLE,
		"vb2_check_keyblock() didn't sign header");

	memcpy(h, hdr, hsize);
	h->data_key.key_offset = hsize;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_check_keyblock(h, hsize, sig),
		VB2_ERROR_KEYBLOCK_DATA_KEY_OUTSIDE,
		"vb2_check_keyblock() data key off end");

	/* Corner cases for error checking */
	TEST_EQ(vb2_check_keyblock(NULL, 4, sig),
		VB2_ERROR_KEYBLOCK_TOO_SMALL_FOR_HEADER,
		"vb2_check_keyblock size too small");

	/*
	 * TODO: verify parser can support a bigger header (i.e., one where
	 * data_key.key_offset is bigger than expected).
	 */

	free(h);
	free(hdr);
}

static void test_verify_keyblock(const struct vb2_public_key *public_key,
				const struct vb2_private_key *private_key,
				const struct vb2_packed_key *data_key)
{
	uint8_t workbuf[VB2_KEY_BLOCK_VERIFY_WORKBUF_BYTES]
		__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;
	struct vb2_keyblock *hdr;
	struct vb2_keyblock *h;
	uint32_t hsize;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	hdr = vb2_create_keyblock(data_key, private_key, 0x1234);
	TEST_NEQ((size_t)hdr, 0, "vb2_verify_keyblock() prerequisites");
	if (!hdr)
		return;
	hsize = hdr->keyblock_size;
	h = (struct vb2_keyblock *)malloc(hsize + 2048);

	memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_keyblock(h, hsize, public_key, &wb),
		  "vb2_verify_keyblock() ok using key");

	/* Failures in keyblock check also cause verify to fail */
	memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_keyblock(h, hsize - 1, public_key, &wb),
		VB2_ERROR_KEYBLOCK_SIZE, "vb2_verify_keyblock() check");

	/* Check signature */
	memcpy(h, hdr, hsize);
	h->keyblock_signature.sig_size--;
	resign_keyblock(h, private_key);
	TEST_EQ(vb2_verify_keyblock(h, hsize, public_key, &wb),
		VB2_ERROR_KEYBLOCK_SIG_INVALID,
		"vb2_verify_keyblock() sig too small");

	memcpy(h, hdr, hsize);
	((uint8_t *)vb2_packed_key_data(&h->data_key))[0] ^= 0x34;
	TEST_EQ(vb2_verify_keyblock(h, hsize, public_key, &wb),
		VB2_ERROR_KEYBLOCK_SIG_INVALID,
		"vb2_verify_keyblock() sig mismatch");

	/*
	 * TODO: verify parser can support a bigger header (i.e., one where
	 * data_key.key_offset is bigger than expected).
	 */

	free(h);
	free(hdr);
}

static void resign_fw_preamble(struct vb2_fw_preamble *h,
			       struct vb2_private_key *key)
{
	struct vb2_signature *sig = vb2_calculate_signature(
		(const uint8_t *)h, h->preamble_signature.data_size, key);

	vb2_copy_signature(&h->preamble_signature, sig);
	free(sig);
}

static void test_verify_fw_preamble(struct vb2_packed_key *public_key,
				    struct vb2_private_key *private_key,
				    struct vb2_packed_key *kernel_subkey)
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
	struct vb2_signature *body_sig = vb2_alloc_signature(56, 78);

	TEST_SUCC(vb2_unpack_key(&rsa, public_key),
		  "vb2_verify_fw_preamble() prereq key");

	hdr = vb2_create_fw_preamble(0x1234, kernel_subkey, body_sig,
				     private_key, 0x5678);
	TEST_PTR_NEQ(hdr, NULL,
		     "vb2_verify_fw_preamble() prereq test preamble");
	if (!hdr) {
		free(body_sig);
		return;
	}

	hsize = (uint32_t) hdr->preamble_size;
	h = (struct vb2_fw_preamble *)malloc(hsize + 16384);

	memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_fw_preamble() ok using key");

	memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_fw_preamble(h, 4, &rsa, &wb),
		VB2_ERROR_PREAMBLE_TOO_SMALL_FOR_HEADER,
		"vb2_verify_fw_preamble() size tiny");

	memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize - 1, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIZE,
		"vb2_verify_fw_preamble() size--");

	/* Buffer is allowed to be bigger than preamble */
	memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_fw_preamble(h, hsize + 1, &rsa, &wb),
		  "vb2_verify_fw_preamble() size++");

	/* Care about major version but not minor */
	memcpy(h, hdr, hsize);
	h->header_version_major++;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_VERSION
		, "vb2_verify_fw_preamble() major++");

	memcpy(h, hdr, hsize);
	h->header_version_major--;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_VERSION,
		"vb2_verify_fw_preamble() major--");

	memcpy(h, hdr, hsize);
	h->header_version_minor++;
	resign_fw_preamble(h, private_key);
	TEST_SUCC(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_fw_preamble() minor++");

	memcpy(h, hdr, hsize);
	h->header_version_minor--;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_OLD,
		"vb2_verify_fw_preamble() 2.0 not supported");

	/* Check signature */
	memcpy(h, hdr, hsize);
	h->preamble_signature.sig_offset = hsize;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_OUTSIDE,
		"vb2_verify_fw_preamble() sig off end");

	memcpy(h, hdr, hsize);
	h->preamble_signature.sig_size--;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_INVALID,
		"vb2_verify_fw_preamble() sig too small");

	memcpy(h, hdr, hsize);
	((uint8_t *)vb2_packed_key_data(&h->kernel_subkey))[0] ^= 0x34;
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_INVALID,
		"vb2_verify_fw_preamble() sig mismatch");

	/* Check that we signed header, kernel subkey, and body sig */
	memcpy(h, hdr, hsize);
	h->preamble_signature.data_size = 4;
	h->kernel_subkey.key_offset = 0;
	h->kernel_subkey.key_size = 0;
	h->body_signature.sig_offset = 0;
	h->body_signature.sig_size = 0;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIGNED_TOO_LITTLE,
		"vb2_verify_fw_preamble() didn't sign header");

	memcpy(h, hdr, hsize);
	h->kernel_subkey.key_offset = hsize;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_KERNEL_SUBKEY_OUTSIDE,
		"vb2_verify_fw_preamble() kernel subkey off end");

	memcpy(h, hdr, hsize);
	h->body_signature.sig_offset = hsize;
	resign_fw_preamble(h, private_key);
	TEST_EQ(vb2_verify_fw_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_BODY_SIG_OUTSIDE,
		"vb2_verify_fw_preamble() body sig off end");

	/* TODO: verify with extra padding at end of header. */

	free(h);
	free(hdr);
	free(body_sig);
}

static void resign_kernel_preamble(struct vb2_kernel_preamble *h,
				   const struct vb2_private_key *key)
{
	struct vb2_signature *sig = vb2_calculate_signature(
		(const uint8_t *)h, h->preamble_signature.data_size, key);

	vb2_copy_signature(&h->preamble_signature, sig);
	free(sig);
}

static void test_verify_kernel_preamble(
		const struct vb2_packed_key *public_key,
		const struct vb2_private_key *private_key)
{
	struct vb2_public_key rsa;
	// TODO: how many workbuf bytes?
	uint8_t workbuf[VB2_VERIFY_FIRMWARE_PREAMBLE_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;
	uint32_t hsize;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Create a dummy signature */
	struct vb2_signature *body_sig = vb2_alloc_signature(56, 0x214000);

	TEST_SUCC(vb2_unpack_key(&rsa, public_key),
		  "vb2_verify_kernel_preamble() prereq key");

	struct vb2_kernel_preamble *hdr =
		vb2_create_kernel_preamble(0x1234, 0x100000, 0x300000, 0x4000,
					   body_sig, 0x304000, 0x10000, 0, 0,
					   private_key);
	TEST_PTR_NEQ(hdr, NULL,
		     "vb2_verify_kernel_preamble() prereq test preamble");
	if (!hdr) {
		free(body_sig);
		return;
	}

	hsize = (uint32_t) hdr->preamble_size;
	struct vb2_kernel_preamble *h =
		(struct vb2_kernel_preamble *)malloc(hsize + 16384);

	memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_kernel_preamble() ok using key");

	memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_kernel_preamble(h, 4, &rsa, &wb),
		VB2_ERROR_PREAMBLE_TOO_SMALL_FOR_HEADER,
		"vb2_verify_kernel_preamble() size tiny");

	memcpy(h, hdr, hsize);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize - 1, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIZE,
		"vb2_verify_kernel_preamble() size--");

	/* Buffer is allowed to be bigger than preamble */
	memcpy(h, hdr, hsize);
	TEST_SUCC(vb2_verify_kernel_preamble(h, hsize + 1, &rsa, &wb),
		  "vb2_verify_kernel_preamble() size++");

	/* Care about major version but not minor */
	memcpy(h, hdr, hsize);
	h->header_version_major++;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_VERSION
		, "vb2_verify_kernel_preamble() major++");

	memcpy(h, hdr, hsize);
	h->header_version_major--;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_HEADER_VERSION,
		"vb2_verify_kernel_preamble() major--");

	memcpy(h, hdr, hsize);
	h->header_version_minor++;
	resign_kernel_preamble(h, private_key);
	TEST_SUCC(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_kernel_preamble() minor++");

	/* Check signature */
	memcpy(h, hdr, hsize);
	h->preamble_signature.sig_offset = hsize;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_OUTSIDE,
		"vb2_verify_kernel_preamble() sig off end");

	memcpy(h, hdr, hsize);
	h->preamble_signature.sig_size--;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_INVALID,
		"vb2_verify_kernel_preamble() sig too small");

	memcpy(h, hdr, hsize);
	h->flags++;
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIG_INVALID,
		"vb2_verify_kernel_preamble() sig mismatch");

	/* Check that we signed header and body sig */
	memcpy(h, hdr, hsize);
	h->preamble_signature.data_size = 4;
	h->body_signature.sig_offset = 0;
	h->body_signature.sig_size = 0;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_SIGNED_TOO_LITTLE,
		"vb2_verify_kernel_preamble() didn't sign header");

	memcpy(h, hdr, hsize);
	h->body_signature.sig_offset = hsize;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_BODY_SIG_OUTSIDE,
		"vb2_verify_kernel_preamble() body sig off end");

	/* Check bootloader inside signed body */
	memcpy(h, hdr, hsize);
	h->bootloader_address = h->body_load_address - 1;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_BOOTLOADER_OUTSIDE,
		"vb2_verify_kernel_preamble() bootloader before body");

	memcpy(h, hdr, hsize);
	h->bootloader_address = h->body_load_address +
		h->body_signature.data_size + 1;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_BOOTLOADER_OUTSIDE,
		"vb2_verify_kernel_preamble() bootloader off end of body");

	memcpy(h, hdr, hsize);
	h->bootloader_address = h->body_load_address +
		h->body_signature.data_size + 1;
	h->bootloader_size = 0;
	resign_kernel_preamble(h, private_key);
	TEST_SUCC(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_kernel_preamble() no bootloader");

	/* Check vmlinuz inside signed body */
	memcpy(h, hdr, hsize);
	h->vmlinuz_header_address = h->body_load_address - 1;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_VMLINUZ_HEADER_OUTSIDE,
		"vb2_verify_kernel_preamble() vmlinuz_header before body");

	memcpy(h, hdr, hsize);
	h->vmlinuz_header_address = h->body_load_address +
		h->body_signature.data_size + 1;
	resign_kernel_preamble(h, private_key);
	TEST_EQ(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		VB2_ERROR_PREAMBLE_VMLINUZ_HEADER_OUTSIDE,
		"vb2_verify_kernel_preamble() vmlinuz_header off end of body");

	memcpy(h, hdr, hsize);
	h->vmlinuz_header_address = h->body_load_address +
		h->body_signature.data_size + 1;
	h->vmlinuz_header_size = 0;
	resign_kernel_preamble(h, private_key);
	TEST_SUCC(vb2_verify_kernel_preamble(h, hsize, &rsa, &wb),
		  "vb2_verify_kernel_preamble() no vmlinuz_header");

	/* TODO: verify with extra padding at end of header. */

	free(h);
	free(hdr);
	free(body_sig);
}

int test_permutation(int signing_key_algorithm, int data_key_algorithm,
		     const char *keys_dir)
{
	char filename[1024];
	int retval = 1;

	struct vb2_private_key *signing_private_key = NULL;
	struct vb2_packed_key *signing_public_key = NULL;
	struct vb2_packed_key *data_public_key = NULL;

	printf("***Testing signing algorithm: %s\n",
	       vb2_get_crypto_algorithm_name(signing_key_algorithm));
	printf("***With data key algorithm: %s\n",
	       vb2_get_crypto_algorithm_name(data_key_algorithm));

	snprintf(filename, sizeof(filename), "%s/key_%s.pem",
		 keys_dir,
		 vb2_get_crypto_algorithm_file(signing_key_algorithm));
	signing_private_key =
		vb2_read_private_key_pem(filename, signing_key_algorithm);
	if (!signing_private_key) {
		fprintf(stderr, "Error reading signing_private_key: %s\n",
			filename);
		goto cleanup_permutation;
	}

	snprintf(filename, sizeof(filename), "%s/key_%s.keyb",
		 keys_dir,
		 vb2_get_crypto_algorithm_file(signing_key_algorithm));
	signing_public_key =
		vb2_read_packed_keyb(filename, signing_key_algorithm, 1);
	if (!signing_public_key) {
		fprintf(stderr, "Error reading signing_public_key: %s\n",
			filename);
		goto cleanup_permutation;
	}

	snprintf(filename, sizeof(filename), "%s/key_%s.keyb",
		 keys_dir,
		 vb2_get_crypto_algorithm_file(data_key_algorithm));
	data_public_key =
		vb2_read_packed_keyb(filename, data_key_algorithm, 1);
	if (!data_public_key) {
		fprintf(stderr, "Error reading data_public_key: %s\n",
			filename);
		goto cleanup_permutation;
	}

	/* Unpack public key */
	struct vb2_public_key signing_public_key2;
	if (VB2_SUCCESS !=
	    vb2_unpack_key_buffer(&signing_public_key2,
			   (uint8_t *)signing_public_key,
			   signing_public_key->key_offset +
			   signing_public_key->key_size)) {
		fprintf(stderr, "Error unpacking signing_public_key: %s\n",
			filename);
		goto cleanup_permutation;
	}

	test_check_keyblock(&signing_public_key2, signing_private_key,
			    data_public_key);
	test_verify_keyblock(&signing_public_key2, signing_private_key,
			     data_public_key);
	test_verify_fw_preamble(signing_public_key, signing_private_key,
				data_public_key);
	test_verify_kernel_preamble(signing_public_key, signing_private_key);

	retval = 0;

cleanup_permutation:
	if (signing_public_key)
		free(signing_public_key);
	if (signing_private_key)
		free(signing_private_key);
	if (data_public_key)
		free(data_public_key);

	return retval;
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
