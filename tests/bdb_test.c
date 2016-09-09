/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Unit tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "2sha.h"
#include "bdb.h"
#include "host.h"
#include "test_common.h"

void check_header_tests(void)
{
	struct bdb_header sgood = {
		.struct_magic = BDB_HEADER_MAGIC,
		.struct_major_version = BDB_HEADER_VERSION_MAJOR,
		.struct_minor_version = BDB_HEADER_VERSION_MINOR,
		.struct_size = sizeof(struct bdb_header),
		.bdb_load_address = -1,
		.bdb_size = 1024,
		.signed_size = 512,
		.oem_area_0_size = 256,
	};
	const size_t ssize = sgood.struct_size;
	struct bdb_header s;

	s = sgood;
	TEST_EQ_S(bdb_check_header(&s, ssize), BDB_SUCCESS);
	TEST_EQ_S(bdb_check_header(&s, ssize - 1), BDB_ERROR_BUF_SIZE);

	s = sgood;
	s.struct_size++;
	TEST_EQ_S(bdb_check_header(&s, ssize), BDB_ERROR_BUF_SIZE);

	s = sgood;
	s.struct_size--;
	TEST_EQ_S(bdb_check_header(&s, ssize), BDB_ERROR_STRUCT_SIZE);

	s = sgood;
	s.struct_magic++;
	TEST_EQ_S(bdb_check_header(&s, ssize), BDB_ERROR_STRUCT_MAGIC);

	s = sgood;
	s.struct_major_version++;
	TEST_EQ_S(bdb_check_header(&s, ssize), BDB_ERROR_STRUCT_VERSION);

	s = sgood;
	s.oem_area_0_size++;
	TEST_EQ_S(bdb_check_header(&s, ssize), BDB_ERROR_OEM_AREA_SIZE);

	s = sgood;
	s.bdb_size = ssize - 1;
	TEST_EQ_S(bdb_check_header(&s, ssize), BDB_ERROR_BDB_SIZE);
}

void check_key_tests(void)
{
	struct bdb_key sgood = {
		.struct_magic = BDB_KEY_MAGIC,
		.struct_major_version = BDB_KEY_VERSION_MAJOR,
		.struct_minor_version = BDB_KEY_VERSION_MINOR,
		.struct_size = (sizeof(struct bdb_key) +
				BDB_RSA4096_KEY_DATA_SIZE),
		.hash_alg = BDB_HASH_ALG_SHA256,
		.sig_alg = BDB_SIG_ALG_RSA4096,
		.key_version = 1,
		.description = "Test key",
	};
	const size_t ssize = sgood.struct_size;
	struct bdb_key s;

	s = sgood;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_SUCCESS);
	TEST_EQ_S(bdb_check_key(&s, ssize - 1), BDB_ERROR_BUF_SIZE);

	s = sgood;
	s.struct_size++;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_ERROR_BUF_SIZE);

	s = sgood;
	s.struct_size--;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_ERROR_STRUCT_SIZE);

	s = sgood;
	s.struct_magic++;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_ERROR_STRUCT_MAGIC);

	s = sgood;
	s.struct_major_version++;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_ERROR_STRUCT_VERSION);

	/* Description must contain a null */
	s = sgood;
	memset(s.description, 'x', sizeof(s.description));
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_ERROR_DESCRIPTION);

	/* Data AFTER the null is explicitly allowed, though */
	s = sgood;
	s.description[100] = 'x';
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_SUCCESS);

	/* Limited algorithm choices at present */
	s = sgood;
	s.hash_alg = BDB_HASH_ALG_INVALID;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_ERROR_HASH_ALG);

	/* This works because ECDSA521 signatures are smaller than RSA4096 */
	s = sgood;
	s.sig_alg = BDB_SIG_ALG_ECSDSA521;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_SUCCESS);

	s = sgood;
	s.sig_alg = BDB_SIG_ALG_INVALID;
	TEST_EQ_S(bdb_check_key(&s, ssize), BDB_ERROR_SIG_ALG);
}

void check_sig_tests(void)
{
	struct bdb_sig sgood = {
		.struct_magic = BDB_SIG_MAGIC,
		.struct_major_version = BDB_SIG_VERSION_MAJOR,
		.struct_minor_version = BDB_SIG_VERSION_MINOR,
		.struct_size = sizeof(struct bdb_sig) + BDB_RSA4096_SIG_SIZE,
		.hash_alg = BDB_HASH_ALG_SHA256,
		.sig_alg = BDB_SIG_ALG_RSA4096,
		.signed_size = 123,
		.description = "Test sig",
	};
	const size_t ssize = sgood.struct_size;
	struct bdb_sig s;

	s = sgood;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_SUCCESS);
	TEST_EQ_S(bdb_check_sig(&s, ssize - 1), BDB_ERROR_BUF_SIZE);

	s = sgood;
	s.struct_size++;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_ERROR_BUF_SIZE);

	s = sgood;
	s.struct_size--;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_ERROR_STRUCT_SIZE);

	s = sgood;
	s.struct_magic++;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_ERROR_STRUCT_MAGIC);

	s = sgood;
	s.struct_major_version++;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_ERROR_STRUCT_VERSION);

	/* Description must contain a null */
	s = sgood;
	memset(s.description, 'x', sizeof(s.description));
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_ERROR_DESCRIPTION);

	/* Data AFTER the null is explicitly allowed, though */
	s = sgood;
	s.description[100] = 'x';
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_SUCCESS);

	/* Limited algorithm choices at present */
	s = sgood;
	s.hash_alg = BDB_HASH_ALG_INVALID;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_ERROR_HASH_ALG);

	/* This works because ECDSA521 signatures are smaller than RSA4096 */
	s = sgood;
	s.sig_alg = BDB_SIG_ALG_ECSDSA521;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_SUCCESS);

	s = sgood;
	s.sig_alg = BDB_SIG_ALG_INVALID;
	TEST_EQ_S(bdb_check_sig(&s, ssize), BDB_ERROR_SIG_ALG);
}

void check_data_tests(void)
{
	struct bdb_data sgood = {
		.struct_magic = BDB_DATA_MAGIC,
		.struct_major_version = BDB_DATA_VERSION_MAJOR,
		.struct_minor_version = BDB_DATA_VERSION_MINOR,
		.struct_size = sizeof(struct bdb_data),
		.data_version = 1,
		.oem_area_1_size = 256,
		.num_hashes = 3,
		.hash_entry_size = sizeof(struct bdb_hash),
		.signed_size = 2048,
		.description = "Test data",
	};
	const size_t ssize = sgood.signed_size;
	struct bdb_data s;

	s = sgood;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_SUCCESS);
	TEST_EQ_S(bdb_check_data(&s, ssize - 1), BDB_ERROR_BUF_SIZE);

	s = sgood;
	s.struct_size--;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_ERROR_STRUCT_SIZE);

	s = sgood;
	s.struct_magic++;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_ERROR_STRUCT_MAGIC);

	s = sgood;
	s.struct_major_version++;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_ERROR_STRUCT_VERSION);

	/* Description must contain a null */
	s = sgood;
	memset(s.description, 'x', sizeof(s.description));
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_ERROR_DESCRIPTION);

	/* Data AFTER the null is explicitly allowed, though */
	s = sgood;
	s.description[100] = 'x';
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_SUCCESS);

	s = sgood;
	s.hash_entry_size--;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_ERROR_HASH_ENTRY_SIZE);

	s = sgood;
	s.oem_area_1_size++;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_ERROR_OEM_AREA_SIZE);

	/* Check exact size needed */
	s = sgood;
	s.signed_size = sizeof(s) + s.num_hashes * sizeof(struct bdb_hash) +
		s.oem_area_1_size;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_SUCCESS);
	s.signed_size--;
	TEST_EQ_S(bdb_check_data(&s, ssize), BDB_ERROR_SIGNED_SIZE);

	/*
	 * TODO: Verify wraparound check works.  That can only be tested on a
	 * platform where size_t is uint32_t, because otherwise a 32-bit
	 * oem_area_1_size can't cause wraparound.
	 */
}

/**
 * Test bdb_verify() and bdb_create()
 */
void check_bdb_verify(const char *key_dir)
{
	uint8_t oem_area_0[32] = "Some OEM area.";
	uint8_t oem_area_1[64] = "Some other OEM area.";
	char filename[1024];

	struct bdb_hash hash[2] = {
		{
			.offset = 0x10000,
			.size = 0x18000,
			.partition = 1,
			.type = BDB_DATA_SP_RW,
			.load_address = 0x100000,
			.digest = {0x11, 0x11, 0x11, 0x10},
		},
		{
			.offset = 0x28000,
			.size = 0x20000,
			.partition = 1,
			.type = BDB_DATA_AP_RW,
			.load_address = 0x200000,
			.digest = {0x22, 0x22, 0x22, 0x20},
		},
	};

	struct bdb_create_params p = {
		.bdb_load_address = 0x11223344,
		.oem_area_0 = oem_area_0,
		.oem_area_0_size = sizeof(oem_area_0),
		.oem_area_1 = oem_area_1,
		.oem_area_1_size = sizeof(oem_area_1),
		.header_sig_description = "The header sig",
		.data_sig_description = "The data sig",
		.data_description = "Test BDB data",
		.data_version = 3,
		.hash = hash,
		.num_hashes = 2,
	};

	uint8_t bdbkey_digest[BDB_SHA256_DIGEST_SIZE];
	struct bdb_header *hgood, *h;
	size_t hsize;

	/* Load keys */
	snprintf(filename, sizeof(filename), "%s/bdbkey.keyb", key_dir);
	p.bdbkey = bdb_create_key(filename, 100, "BDB key");
	snprintf(filename, sizeof(filename), "%s/datakey.keyb", key_dir);
	p.datakey = bdb_create_key(filename, 200, "datakey");
	snprintf(filename, sizeof(filename), "%s/bdbkey.pem", key_dir);
	p.private_bdbkey = read_pem(filename);
	snprintf(filename, sizeof(filename), "%s/datakey.pem", key_dir);
	p.private_datakey = read_pem(filename);
	if (!p.bdbkey || !p.datakey || !p.private_bdbkey || !p.private_datakey) {
		fprintf(stderr, "Unable to load test keys\n");
		exit(2);
	}

	vb2_digest_buffer((uint8_t *)p.bdbkey, p.bdbkey->struct_size,
			  VB2_HASH_SHA256,
			  bdbkey_digest, BDB_SHA256_DIGEST_SIZE);

	/* Create the test BDB */
	hgood = bdb_create(&p);
	if (!hgood) {
		fprintf(stderr, "Unable to create test BDB\n");
		exit(2);
	}
	hsize = hgood->bdb_size;

	/* Allocate a copy we can mangle */
	h = calloc(hsize, 1);

	/* As created, it should pass */
	memcpy(h, hgood, hsize);
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_SUCCESS);

	/* Mangle each component in turn */
	memcpy(h, hgood, hsize);
	h->struct_magic++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_HEADER);

	memcpy(h, hgood, hsize);
	((struct bdb_key *)bdb_get_bdbkey(h))->struct_magic++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_BDBKEY);

	memcpy(h, hgood, hsize);
	((struct bdb_key *)bdb_get_bdbkey(h))->key_version++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_GOOD_OTHER_THAN_KEY);

	memcpy(h, hgood, hsize);
	h->oem_area_0_size += hsize;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_OEM_AREA_0);

	memcpy(h, hgood, hsize);
	((struct bdb_key *)bdb_get_datakey(h))->struct_magic++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATAKEY);

	memcpy(h, hgood, hsize);
	((struct bdb_key *)bdb_get_datakey(h))->struct_size += 4;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_BDB_SIGNED_SIZE);

	memcpy(h, hgood, hsize);
	((struct bdb_sig *)bdb_get_header_sig(h))->struct_magic++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_HEADER_SIG);

	memcpy(h, hgood, hsize);
	((struct bdb_sig *)bdb_get_header_sig(h))->signed_size--;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_HEADER_SIG);

	memcpy(h, hgood, hsize);
	((struct bdb_sig *)bdb_get_header_sig(h))->sig_data[0] ^= 0x42;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_HEADER_SIG);

	/* Also make sure the header sig really covers all the fields */
	memcpy(h, hgood, hsize);
	((struct bdb_key *)bdb_get_datakey(h))->key_version++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_HEADER_SIG);

	memcpy(h, hgood, hsize);
	((uint8_t *)bdb_get_oem_area_0(h))[0] ^= 0x42;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_HEADER_SIG);

	memcpy(h, hgood, hsize);
	((uint8_t *)bdb_get_oem_area_0(h))[p.oem_area_0_size - 1] ^= 0x24;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_HEADER_SIG);

	/* Check data header */
	memcpy(h, hgood, hsize);
	((struct bdb_data *)bdb_get_data(h))->struct_magic++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA);

	memcpy(h, hgood, hsize);
	((struct bdb_sig *)bdb_get_data_sig(h))->struct_magic++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	memcpy(h, hgood, hsize);
	((struct bdb_sig *)bdb_get_data_sig(h))->signed_size--;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	memcpy(h, hgood, hsize);
	((struct bdb_sig *)bdb_get_data_sig(h))->sig_data[0] ^= 0x42;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	/* Also make sure the data sig really covers all the fields */
	memcpy(h, hgood, hsize);
	((struct bdb_data *)bdb_get_data(h))->data_version--;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	memcpy(h, hgood, hsize);
	((uint8_t *)bdb_get_oem_area_1(h))[0] ^= 0x42;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	memcpy(h, hgood, hsize);
	((uint8_t *)bdb_get_oem_area_1(h))[p.oem_area_1_size - 1] ^= 0x24;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	memcpy(h, hgood, hsize);
	((struct bdb_hash *)bdb_get_hash(h, BDB_DATA_SP_RW))->offset++;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	memcpy(h, hgood, hsize);
	((struct bdb_hash *)bdb_get_hash(h, BDB_DATA_AP_RW))->digest[0] ^= 0x96;
	TEST_EQ_S(bdb_verify(h, hsize, bdbkey_digest), BDB_ERROR_DATA_SIG);

	/*
	 * This is also a convenient place to test that all the parameters we
	 * fed into bdb_create() also worked.  That also tests all the
	 * bdb_get_*() functions.
	 */
	memcpy(h, hgood, hsize);
	TEST_EQ_S(h->bdb_load_address, p.bdb_load_address);

	TEST_EQ_S(strcmp(bdb_get_bdbkey(h)->description, p.bdbkey->description),
		  0);
	TEST_EQ_S(bdb_get_bdbkey(h)->key_version, p.bdbkey->key_version);

	TEST_EQ_S(h->oem_area_0_size, p.oem_area_0_size);
	TEST_EQ_S(memcmp(bdb_get_oem_area_0(h), oem_area_0, sizeof(oem_area_0)),
		0);

	TEST_EQ_S(strcmp(bdb_get_datakey(h)->description, p.datakey->description),
		0);
	TEST_EQ_S(bdb_get_datakey(h)->key_version, p.datakey->key_version);

	TEST_EQ_S(strcmp(bdb_get_header_sig(h)->description,
		       p.header_sig_description), 0);

	TEST_EQ_S(strcmp(bdb_get_data(h)->description, p.data_description), 0);
	TEST_EQ_S(bdb_get_data(h)->data_version, p.data_version);
	TEST_EQ_S(bdb_get_data(h)->num_hashes, p.num_hashes);

	TEST_EQ_S(bdb_get_data(h)->oem_area_1_size, p.oem_area_1_size);
	TEST_EQ_S(memcmp(bdb_get_oem_area_1(h), oem_area_1, sizeof(oem_area_1)),
		0);

	TEST_EQ_S(strcmp(bdb_get_data_sig(h)->description,
		       p.data_sig_description), 0);

	/* Test getting hash entries */
	memcpy(h, hgood, hsize);
	TEST_EQ_S(bdb_get_hash(h, BDB_DATA_SP_RW)->offset, hash[0].offset);
	TEST_EQ_S(bdb_get_hash(h, BDB_DATA_AP_RW)->offset, hash[1].offset);
	/* And a non-existent one */
	TEST_EQ_S(bdb_get_hash(h, BDB_DATA_MCU)!=NULL, 0);

	/*
	 * TODO: Verify wraparound checks works.  That can only be tested on a
	 * platform where size_t is uint32_t, because otherwise a 32-bit
	 * oem_area_1_size can't cause wraparound.
	 */

	/* Free keys and buffers */
	free(p.bdbkey);
	free(p.datakey);
	RSA_free(p.private_bdbkey);
	RSA_free(p.private_datakey);
	free(hgood);
	free(h);
}

/*****************************************************************************/

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <keys_dir>", argv[0]);
		return -1;
	}
	printf("Running BDB tests...\n");

	check_header_tests();
	check_key_tests();
	check_sig_tests();
	check_data_tests();
	check_bdb_verify(argv[1]);

	printf("All tests passed!\n");

	return gTestSuccess ? 0 : 255;
}
