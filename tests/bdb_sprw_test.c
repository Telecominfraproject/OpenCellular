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
#include "bdb_api.h"
#include "bdb_struct.h"
#include "host.h"
#include "test_common.h"
#include "vboot_register.h"

#define TEST_EQ_S(result, expect) TEST_EQ(result, expect, #result "==" #expect)

static struct bdb_header *bdb, *bdb0, *bdb1;
static uint32_t vboot_register;
static uint32_t vboot_register_persist;
static char slot_selected;
static uint8_t aprw_digest[BDB_SHA256_DIGEST_SIZE];
static uint8_t reset_count;

static struct bdb_header *create_bdb(const char *key_dir,
				     struct bdb_hash *hash, int num_hashes)
{
	struct bdb_header *b;
	uint8_t oem_area_0[32] = "Some OEM area.";
	uint8_t oem_area_1[64] = "Some other OEM area.";
	char filename[1024];

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
		.num_hashes = num_hashes,
	};

	uint8_t bdbkey_digest[BDB_SHA256_DIGEST_SIZE];

	/* Load keys */
	sprintf(filename, "%s/bdbkey.keyb", key_dir);
	p.bdbkey = bdb_create_key(filename, 100, "BDB key");
	sprintf(filename, "%s/datakey.keyb", key_dir);
	p.datakey = bdb_create_key(filename, 200, "datakey");
	sprintf(filename, "%s/bdbkey.pem", key_dir);
	p.private_bdbkey = read_pem(filename);
	sprintf(filename, "%s/datakey.pem", key_dir);
	p.private_datakey = read_pem(filename);
	if (!p.bdbkey || !p.datakey || !p.private_bdbkey || !p.private_datakey) {
		fprintf(stderr, "Unable to load test keys\n");
		exit(2);
	}

	vb2_digest_buffer((uint8_t *)p.bdbkey, p.bdbkey->struct_size,
			  VB2_HASH_SHA256,
			  bdbkey_digest, BDB_SHA256_DIGEST_SIZE);

	b = bdb_create(&p);
	if (!b) {
		fprintf(stderr, "Unable to create test BDB\n");
		exit(2);
	}

	/* Free keys and buffers */
	free(p.bdbkey);
	free(p.datakey);
	RSA_free(p.private_bdbkey);
	RSA_free(p.private_datakey);

	return b;
}

static void calculate_aprw_digest(const struct bdb_hash *hash, uint8_t *digest)
{
	/* Locate AP-RW */
	/* Calculate digest as loading AP-RW */
	memcpy(digest, aprw_digest, sizeof(aprw_digest));
}

static void verstage_main(void)
{
	struct vba_context ctx;
	const struct bdb_hash *hash;
	uint8_t digest[BDB_SHA256_DIGEST_SIZE];
	int rv;

	rv = vba_bdb_init(&ctx);
	if (rv) {
		fprintf(stderr, "Initializing context failed for (%d)\n", rv);
		vba_bdb_fail(&ctx);
		/* This return is needed for unit test. vba_bdb_fail calls
		 * vbe_reset, which calls verstage_main. If verstage_main
		 * successfully returns, we return here as well. */
		return;
	}
	fprintf(stderr, "Initialized context. Trying slot %c\n",
		ctx.slot ? 'B' : 'A');

	/* 1. Locate BDB */

	/* 2. Get bdb_hash structure for AP-RW */
	hash = bdb_get_hash(bdb, BDB_DATA_AP_RW);
	fprintf(stderr, "Got hash of AP-RW\n");

	/* 3. Load & calculate digest of AP-RW */
	calculate_aprw_digest(hash, digest);
	fprintf(stderr, "Calculated digest\n");

	/* 4. Compare digests */
	if (memcmp(hash->digest, digest, BDB_SHA256_DIGEST_SIZE)) {
		fprintf(stderr, "Digests do not match\n");
		vba_bdb_fail(&ctx);
		/* This return is needed for unit test. vba_bdb_fail calls
		 * vbe_reset, which calls verstage_main. If verstage_main
		 * successfully returns, we return here as well. */
		return;
	}

	/* 5. Record selected slot. This depends on the firmware */
	slot_selected = ctx.slot ? 'B' : 'A';
	fprintf(stderr, "Selected AP-RW in slot %c\n", slot_selected);

	/* X. This should be done upon AP-RW's request after everything is
	 * successful. We do it here for the unit test. */
	vba_bdb_finalize(&ctx);
}

uint32_t vbe_get_vboot_register(enum vboot_register type)
{
	switch (type) {
	case VBOOT_REGISTER:
		return vboot_register;
	case VBOOT_REGISTER_PERSIST:
		return vboot_register_persist;
	default:
		fprintf(stderr, "Invalid vboot register type (%d)\n", type);
		exit(2);
	}
}

void vbe_set_vboot_register(enum vboot_register type, uint32_t val)
{
	switch (type) {
	case VBOOT_REGISTER:
		vboot_register = val;
		break;
	case VBOOT_REGISTER_PERSIST:
		vboot_register_persist = val;
		break;
	default:
		fprintf(stderr, "Invalid vboot register type (%d)\n", type);
		exit(2);
	}
}

void vbe_reset(void)
{
	uint32_t val = vbe_get_vboot_register(VBOOT_REGISTER_PERSIST);

	fprintf(stderr, "Booting ...\n");

	if (++reset_count > 5) {
		fprintf(stderr, "Reset counter exceeded maximum value\n");
		exit(2);
	}

	/* Emulate warm reset */
	vboot_register = 0;
	if (val & VBOOT_REGISTER_RECOVERY_REQUEST) {
		fprintf(stderr, "Recovery requested\n");
		return;
	}
	/* Selected by SP-RO */
	bdb = (val & VBOOT_REGISTER_TRY_SECONDARY_BDB) ? bdb1 : bdb0;
	verstage_main();
}

static void test_verify_aprw(const char *key_dir)
{
	struct bdb_hash hash0 = {
		.offset = 0x28000,
		.size = 0x20000,
		.partition = 1,
		.type = BDB_DATA_AP_RW,
		.load_address = 0x200000,
		.digest = {0x11, 0x11, 0x11, 0x11},
	};
	struct bdb_hash hash1 = {
		.offset = 0x28000,
		.size = 0x20000,
		.partition = 1,
		.type = BDB_DATA_AP_RW,
		.load_address = 0x200000,
		.digest = {0x22, 0x22, 0x22, 0x22},
	};

	bdb0 = create_bdb(key_dir, &hash0, 1);
	bdb1 = create_bdb(key_dir, &hash1, 1);
	memset(aprw_digest, 0, BDB_SHA256_DIGEST_SIZE);

	/* (slotA, slotB) = (good, bad) */
	reset_count = 0;
	vboot_register_persist = 0;
	slot_selected = 'X';
	memcpy(aprw_digest, hash0.digest, 4);
	vbe_reset();
	TEST_EQ_S(reset_count, 1);
	TEST_EQ_S(slot_selected, 'A');
	TEST_FALSE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_PRIMARY,
		   "VBOOT_REGISTER_FAILED_RW_PRIMARY==false");
	TEST_FALSE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_SECONDARY,
		   "VBOOT_REGISTER_FAILED_RW_SECONDARY==false");

	/* (slotA, slotB) = (bad, good) */
	reset_count = 0;
	vboot_register_persist = 0;
	slot_selected = 'X';
	memcpy(aprw_digest, hash1.digest, 4);
	vbe_reset();
	TEST_EQ_S(reset_count, 3);
	TEST_EQ_S(slot_selected, 'B');
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_PRIMARY,
		  "VBOOT_REGISTER_FAILED_RW_PRIMARY==true");
	TEST_FALSE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_SECONDARY,
		   "VBOOT_REGISTER_FAILED_RW_SECONDARY==false");

	/* (slotA, slotB) = (bad, bad) */
	reset_count = 0;
	vboot_register_persist = 0;
	slot_selected = 'X';
	memset(aprw_digest, 0, BDB_SHA256_DIGEST_SIZE);
	vbe_reset();
	TEST_EQ_S(reset_count, 5);
	TEST_EQ_S(slot_selected, 'X');
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_PRIMARY,
		  "VBOOT_REGISTER_FAILED_RW_PRIMARY==true");
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_SECONDARY,
		  "VBOOT_REGISTER_FAILED_RW_SECONDARY==true");
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_RECOVERY_REQUEST,
		  "Recovery request");

	/* Clean up */
	free(bdb0);
	free(bdb1);
}

/*****************************************************************************/

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <keys_dir>", argv[0]);
		return -1;
	}
	printf("Running BDB SP-RW tests...\n");

	test_verify_aprw(argv[1]);

	return gTestSuccess ? 0 : 255;
}
