/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for host library vboot2 preamble functions
 */

#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2rsa.h"

#include "vb2_common.h"

#include "host_common.h"
#include "host_fw_preamble2.h"
#include "host_key2.h"
#include "host_signature2.h"

#include "test_common.h"

const uint8_t test_data1[] = "Some test data";
const uint8_t test_data2[] = "Some more test data";
const uint8_t test_data3[] = "Even more test data";

static void preamble_tests(const char *keys_dir)
{
	struct vb2_private_key *prik4096;
	struct vb2_public_key *pubk4096;
	struct vb2_fw_preamble *fp;
	const struct vb2_private_key *prikhash;
	struct vb2_signature *hashes[3];
	char fname[1024];
	const char test_desc[] = "Test fw preamble";
	const uint32_t test_version = 2061;
	const uint32_t test_flags = 0x11223344;
	uint32_t hash_next;
	int i;

	uint8_t workbuf[VB2_VERIFY_FIRMWARE_PREAMBLE_WORKBUF_BYTES]
		 __attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
	struct vb2_workbuf wb;

	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Read keys */
	sprintf(fname, "%s/key_rsa4096.keyb", keys_dir);
	TEST_SUCC(vb2_public_key_read_keyb(&pubk4096, fname),
					   "Read public key 1");
	vb2_public_key_set_desc(pubk4096, "Test RSA4096 public key");
	pubk4096->hash_alg = VB2_HASH_SHA256;

	sprintf(fname, "%s/key_rsa4096.pem", keys_dir);
	TEST_SUCC(vb2_private_key_read_pem(&prik4096, fname),
		  "Read private key 2");
	vb2_private_key_set_desc(prik4096, "Test RSA4096 private key");
	prik4096->sig_alg = VB2_SIG_RSA4096;
	prik4096->hash_alg = VB2_HASH_SHA256;

	TEST_SUCC(vb2_private_key_hash(&prikhash, VB2_HASH_SHA256),
			  "Create private hash key");

	/* Create some signatures */
	TEST_SUCC(vb2_sign_data(hashes + 0, test_data1, sizeof(test_data1),
				prikhash, "Hash 1"),
		  "Hash 1");
	TEST_SUCC(vb2_sign_data(hashes + 1, test_data2, sizeof(test_data2),
				prikhash, "Hash 2"),
		  "Hash 2");
	TEST_SUCC(vb2_sign_data(hashes + 2, test_data3, sizeof(test_data3),
				prikhash, "Hash 3"),
			  "Hash 3");

	/* Test good preamble */
	TEST_SUCC(vb2_fw_preamble_create(&fp, prik4096,
					 (const struct vb2_signature **)hashes,
					 3, test_version, test_flags,
					 test_desc),
		  "Create preamble good");
	TEST_PTR_NEQ(fp, NULL, "  fp_ptr");
	TEST_SUCC(vb2_verify_fw_preamble(fp, fp->c.total_size, pubk4096, &wb),
		  "Verify preamble good");
	TEST_EQ(strcmp(vb2_common_desc(fp), test_desc), 0, "  desc");
	TEST_EQ(fp->fw_version, test_version, "  fw_version");
	TEST_EQ(fp->flags, test_flags, "  flags");
	TEST_EQ(fp->hash_count, 3, "  hash_count");

	hash_next = fp->hash_offset;
	for (i = 0; i < 3; i++) {
		TEST_EQ(0, memcmp((uint8_t *)fp + hash_next, hashes[i],
				  hashes[i]->c.total_size), "  hash[i]");
		hash_next += hashes[i]->c.total_size;
	}

	free(fp);

	/* Test errors */
	prik4096->hash_alg = VB2_HASH_INVALID;
	TEST_EQ(vb2_fw_preamble_create(&fp, prik4096,
				       (const struct vb2_signature **)hashes,
				       3, test_version, test_flags,
				       test_desc),
		VB2_FW_PREAMBLE_CREATE_SIG_SIZE,
		"Create preamble bad sig");
	TEST_PTR_EQ(fp, NULL, "  fp_ptr");

	/* Free keys */
	vb2_public_key_free(pubk4096);
	vb2_private_key_free(prik4096);
}

int main(int argc, char *argv[]) {

	if (argc == 2) {
		preamble_tests(argv[1]);
	} else {
		fprintf(stderr, "Usage: %s <keys_dir>", argv[0]);
		return -1;
	}

	return gTestSuccess ? 0 : 255;
}
