/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for host library vboot2 keyblock functions
 */

#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_key2.h"
#include "host_keyblock2.h"

#include "test_common.h"

static void keyblock_tests(const char *keys_dir)
{
	struct vb2_public_key *pubk2048, *pubk4096, *pubk8192, pubkhash;
	struct vb2_private_key *prik4096, *prik8192;
	struct vb2_packed_key *pak, *pakgood;
	struct vb2_keyblock *kb;
	const struct vb2_private_key *prikhash;
	const struct vb2_private_key *prik[2];
	char fname[1024];
	const char test_desc[] = "Test keyblock";

	uint8_t workbuf[VB2_KEY_BLOCK_VERIFY_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Read keys */
	sprintf(fname, "%s/key_rsa2048.keyb", keys_dir);
	TEST_SUCC(vb2_public_key_read_keyb(&pubk2048, fname),
					   "Read public key 2");
	vb2_public_key_set_desc(pubk2048, "Test RSA2048 public key");
	pubk2048->hash_alg = VB2_HASH_SHA256;

	sprintf(fname, "%s/key_rsa4096.keyb", keys_dir);
	TEST_SUCC(vb2_public_key_read_keyb(&pubk4096, fname),
					   "Read public key 1");
	vb2_public_key_set_desc(pubk4096, "Test RSA4096 public key");
	pubk4096->hash_alg = VB2_HASH_SHA256;

	sprintf(fname, "%s/key_rsa8192.keyb", keys_dir);
	TEST_SUCC(vb2_public_key_read_keyb(&pubk8192, fname),
					   "Read public key 2");
	vb2_public_key_set_desc(pubk8192, "Test RSA8192 public key");
	pubk8192->hash_alg = VB2_HASH_SHA512;

	sprintf(fname, "%s/key_rsa4096.pem", keys_dir);
	TEST_SUCC(vb2_private_key_read_pem(&prik4096, fname),
		  "Read private key 2");
	vb2_private_key_set_desc(prik4096, "Test RSA4096 private key");
	prik4096->sig_alg = VB2_SIG_RSA4096;
	prik4096->hash_alg = VB2_HASH_SHA256;

	sprintf(fname, "%s/key_rsa8192.pem", keys_dir);
	TEST_SUCC(vb2_private_key_read_pem(&prik8192, fname),
		  "Read private key 1");
	vb2_private_key_set_desc(prik8192, "Test RSA8192 private key");
	prik8192->sig_alg = VB2_SIG_RSA8192;
	prik8192->hash_alg = VB2_HASH_SHA512;

	TEST_SUCC(vb2_private_key_hash(&prikhash, VB2_HASH_SHA512),
		  "Create private hash key");

	TEST_SUCC(vb2_public_key_hash(&pubkhash, VB2_HASH_SHA512),
		  "Create public hash key");

	TEST_SUCC(vb2_public_key_pack(&pakgood, pubk2048), "Test packed key");

	/* Sign a keyblock with one key */
	prik[0] = prik4096;
	TEST_SUCC(vb2_keyblock_create(&kb, pubk2048, prik, 1, 0x1234, NULL),
		  "Keyblock single");
	TEST_PTR_NEQ(kb, NULL, "  kb_ptr");
	TEST_SUCC(vb2_verify_keyblock(kb, kb->c.total_size, pubk4096, &wb),
		  "  verify");
	TEST_EQ(strcmp(vb2_common_desc(kb), pubk2048->desc), 0,	"  desc");
	TEST_EQ(kb->flags, 0x1234, "  flags");

	pak = (struct vb2_packed_key *)((uint8_t *)kb + kb->key_offset);
	TEST_EQ(0, memcmp(pak, pakgood, pakgood->c.total_size), "  data key");
	free(kb);

	/* Sign a keyblock with two keys */
	prik[0] = prik8192;
	prik[1] = prikhash;
	TEST_SUCC(vb2_keyblock_create(&kb, pubk4096, prik, 2, 0, test_desc),
		  "Keyblock multiple");
	TEST_SUCC(vb2_verify_keyblock(kb, kb->c.total_size, pubk8192, &wb),
		  "  verify 1");
	TEST_SUCC(vb2_verify_keyblock(kb, kb->c.total_size, &pubkhash, &wb),
		  "  verify 2");
	TEST_EQ(strcmp(vb2_common_desc(kb), test_desc), 0,	"  desc");
	TEST_EQ(kb->flags, 0, "  flags");
	free(kb);

	/* Test errors */
	prik[0] = prik8192;
	prik8192->hash_alg = VB2_HASH_INVALID;
	TEST_EQ(vb2_keyblock_create(&kb, pubk4096, prik, 1, 0, NULL),
		VB2_KEYBLOCK_CREATE_SIG_SIZE, "Keyblock bad sig size");
	TEST_PTR_EQ(kb, NULL, "  kb_ptr");

	prik[0] = prik4096;
	pubk4096->sig_alg = VB2_SIG_INVALID;
	TEST_EQ(vb2_keyblock_create(&kb, pubk4096, prik, 1, 0, NULL),
		VB2_KEYBLOCK_CREATE_DATA_KEY, "Keyblock bad data key");

	/* Free keys */
	free(pakgood);
	vb2_public_key_free(pubk2048);
	vb2_public_key_free(pubk4096);
	vb2_public_key_free(pubk8192);
	vb2_private_key_free(prik4096);
	vb2_private_key_free(prik8192);
}

int main(int argc, char *argv[]) {

	if (argc == 2) {
		keyblock_tests(argv[1]);
	} else {
		fprintf(stderr, "Usage: %s <keys_dir>", argv[0]);
		return -1;
	}

	return gTestSuccess ? 0 : 255;
}
