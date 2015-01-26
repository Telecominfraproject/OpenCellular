/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for host library vboot2 key functions
 */

#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_signature2.h"
#include "test_common.h"

/* Test only the algorithms we use */
struct alg_combo {
	const char *name;
	enum vb2_signature_algorithm sig_alg;
	enum vb2_hash_algorithm hash_alg;
};

static const struct alg_combo test_algs[] = {
	{"RSA2048/SHA-256", VB2_SIG_RSA2048, VB2_HASH_SHA256},
	{"RSA4096/SHA-256", VB2_SIG_RSA4096, VB2_HASH_SHA256},
	{"RSA8192/SHA-512", VB2_SIG_RSA8192, VB2_HASH_SHA512},
};

const struct vb2_guid test_guid = {.raw = {0xaa}};
const char *test_desc = "The test key";
const char *test_sig_desc = "The test signature";
const uint8_t test_data[] = "Some test data";
const uint32_t test_size = sizeof(test_data);

static void sig_tests(const struct alg_combo *combo,
		      const char *pemfile,
		      const char *keybfile)
{
	struct vb2_private_key *prik, prik2;
	const struct vb2_private_key *prihash, *priks[2];
	struct vb2_public_key *pubk, pubhash;
	struct vb2_signature *sig, *sig2;
	uint32_t size;

	uint8_t workbuf[VB2_VERIFY_DATA_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;

	uint8_t *buf;
	uint32_t bufsize;
	struct vb2_struct_common *c;
	uint32_t c_sig_offs;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Create test keys */
	/* TODO: should read these from .vbprik2, .vbpubk2 files */
	TEST_SUCC(vb2_private_key_read_pem(&prik, pemfile), "Read private key");
	prik->guid = test_guid;
	prik->hash_alg = combo->hash_alg;
	prik->sig_alg = combo->sig_alg;
	vb2_private_key_set_desc(prik, test_desc);

	TEST_SUCC(vb2_public_key_read_keyb(&pubk, keybfile), "Read pub key");
	pubk->guid = &test_guid;
	pubk->hash_alg = combo->hash_alg;
	vb2_public_key_set_desc(pubk, test_desc);

	TEST_SUCC(vb2_private_key_hash(&prihash, combo->hash_alg),
		  "Private hash key");
	TEST_SUCC(vb2_public_key_hash(&pubhash, combo->hash_alg),
		  "Public hash key");

	priks[0] = prik;
	priks[1] = prihash;

	/* Sign test data */
	TEST_SUCC(vb2_sign_data(&sig, test_data, test_size, prik, NULL),
		  "Sign good");
	TEST_PTR_NEQ(sig, NULL, "  sig_ptr");
	TEST_EQ(0, strcmp(vb2_common_desc(sig), test_desc), "  desc");
	TEST_EQ(0, memcmp(&sig->guid, &test_guid, sizeof(test_guid)), "  guid");
	TEST_EQ(sig->data_size, test_size, "  data_size");
	TEST_SUCC(vb2_sig_size_for_key(&size, prik, NULL), "Sig size");
	TEST_EQ(size, sig->c.total_size, "  size");
	TEST_SUCC(vb2_verify_data(test_data, test_size, sig, pubk, &wb),
		  "Verify good");
	free(sig);

	TEST_SUCC(vb2_sign_data(&sig, test_data, test_size, prik,
				test_sig_desc),
		  "Sign with desc");
	TEST_EQ(0, strcmp(vb2_common_desc(sig),	test_sig_desc), "  desc");
	free(sig);

	TEST_SUCC(vb2_sign_data(&sig, test_data, test_size, prik, ""),
		  "Sign with no desc");
	TEST_EQ(sig->c.desc_size, 0, "  desc");
	TEST_SUCC(vb2_sig_size_for_key(&size, prik, ""), "Sig size");
	TEST_EQ(size, sig->c.total_size, "  size");
	free(sig);

	TEST_SUCC(vb2_sign_data(&sig, test_data, test_size, prihash, NULL),
		  "Sign with hash");
	TEST_SUCC(vb2_verify_data(test_data, test_size, sig, &pubhash, &wb),
		  "Verify with hash");
	free(sig);

	prik2 = *prik;
	prik2.sig_alg = VB2_SIG_INVALID;
	TEST_EQ(vb2_sign_data(&sig, test_data, test_size, &prik2, NULL),
		VB2_SIGN_DATA_SIG_SIZE, "Sign bad sig alg");

	/* Sign an object with a little (24 bytes) data */
	c_sig_offs = sizeof(*c) + 24;
	TEST_SUCC(vb2_sig_size_for_key(&size, prik, NULL), "Sig size");
	bufsize = c_sig_offs + size;
	buf = calloc(1, bufsize);
	memset(buf + sizeof(*c), 0x12, 24);
	c = (struct vb2_struct_common *)buf;
	c->total_size = bufsize;

	TEST_SUCC(vb2_sign_object(buf, c_sig_offs, prik, NULL), "Sign object");
	sig = (struct vb2_signature *)(buf + c_sig_offs);
	TEST_SUCC(vb2_verify_data(buf, c_sig_offs, sig, pubk, &wb),
		  "Verify object");

	TEST_EQ(vb2_sign_object(buf, c_sig_offs + 4, prik, NULL),
		VB2_SIGN_OBJECT_OVERFLOW, "Sign object overflow");
	free(buf);

	/* Multiply sign an object */
	TEST_SUCC(vb2_sig_size_for_keys(&size, priks, 2), "Sigs size");
	bufsize = c_sig_offs + size;
	buf = calloc(1, bufsize);
	memset(buf + sizeof(*c), 0x12, 24);
	c = (struct vb2_struct_common *)buf;
	c->total_size = bufsize;

	TEST_SUCC(vb2_sign_object_multiple(buf, c_sig_offs, priks, 2),
		  "Sign multiple");
	sig = (struct vb2_signature *)(buf + c_sig_offs);
	TEST_SUCC(vb2_verify_data(buf, c_sig_offs, sig, pubk, &wb),
		  "Verify object with sig 1");
	sig2 = (struct vb2_signature *)(buf + c_sig_offs + sig->c.total_size);
	TEST_SUCC(vb2_verify_data(buf, c_sig_offs, sig2, &pubhash, &wb),
		  "Verify object with sig 2");

	c->total_size -= 4;
	TEST_EQ(vb2_sign_object_multiple(buf, c_sig_offs, priks, 2),
		VB2_SIGN_OBJECT_OVERFLOW, "Sign multple overflow");

	TEST_EQ(size, sig->c.total_size + sig2->c.total_size,
		"Sigs size total");

	free(buf);

	vb2_private_key_free(prik);
	vb2_public_key_free(pubk);
}

static int test_algorithm(const struct alg_combo *combo, const char *keys_dir)
{
	int rsa_bits = vb2_rsa_sig_size(combo->sig_alg) * 8;
	char pemfile[1024];
	char keybfile[1024];

	printf("***Testing algorithm: %s\n", combo->name);

	sprintf(pemfile, "%s/key_rsa%d.pem", keys_dir, rsa_bits);
	sprintf(keybfile, "%s/key_rsa%d.keyb", keys_dir, rsa_bits);

	sig_tests(combo, pemfile, keybfile);

	return 0;
}

int main(int argc, char *argv[]) {

	if (argc == 2) {
		int i;

		for (i = 0; i < ARRAY_SIZE(test_algs); i++) {
			if (test_algorithm(test_algs + i, argv[1]))
				return 1;
		}
	} else {
		fprintf(stderr, "Usage: %s <keys_dir>", argv[0]);
		return -1;
	}

	return gTestSuccess ? 0 : 255;
}
