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
#include "2hmac.h"
#include "bdb.h"
#include "bdb_api.h"
#include "bdb_struct.h"
#include "host.h"
#include "test_common.h"
#include "vboot_register.h"
#include "secrets.h"

static struct bdb_header *bdb, *bdb0, *bdb1;
static uint32_t vboot_register;
static uint32_t vboot_register_persist;
static char slot_selected;
static uint8_t aprw_digest[BDB_SHA256_DIGEST_SIZE];
static uint8_t reset_count;

/* NVM-RW image in storage (e.g. EEPROM) */
static uint8_t nvmrw1[NVM_RW_MAX_STRUCT_SIZE];
static uint8_t nvmrw2[NVM_RW_MAX_STRUCT_SIZE];

struct bdb_ro_secrets secrets = {
	.nvm_rw = {0x00, },
};

static int vbe_write_nvm_failure = 0;

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
	TEST_EQ(reset_count, 1, NULL);
	TEST_EQ(slot_selected, 'A', NULL);
	TEST_FALSE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_PRIMARY,
		   NULL);
	TEST_FALSE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_SECONDARY,
		   NULL);

	/* (slotA, slotB) = (bad, good) */
	reset_count = 0;
	vboot_register_persist = 0;
	slot_selected = 'X';
	memcpy(aprw_digest, hash1.digest, 4);
	vbe_reset();
	TEST_EQ(reset_count, 3, NULL);
	TEST_EQ(slot_selected, 'B', NULL);
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_PRIMARY,
		  NULL);
	TEST_FALSE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_SECONDARY,
		   NULL);

	/* (slotA, slotB) = (bad, bad) */
	reset_count = 0;
	vboot_register_persist = 0;
	slot_selected = 'X';
	memset(aprw_digest, 0, BDB_SHA256_DIGEST_SIZE);
	vbe_reset();
	TEST_EQ(reset_count, 5, NULL);
	TEST_EQ(slot_selected, 'X', NULL);
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_PRIMARY,
		  NULL);
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_FAILED_RW_SECONDARY,
		  NULL);
	TEST_TRUE(vboot_register_persist & VBOOT_REGISTER_RECOVERY_REQUEST,
		  NULL);

	/* Clean up */
	free(bdb0);
	free(bdb1);
}

int vbe_read_nvm(enum nvm_type type, uint8_t *buf, uint32_t size)
{
	/* Read NVM-RW contents (from EEPROM for example) */
	switch (type) {
	case NVM_TYPE_RW_PRIMARY:
		if (sizeof(nvmrw1) < size)
			return -1;
		memcpy(buf, nvmrw1, size);
		break;
	case NVM_TYPE_RW_SECONDARY:
		if (sizeof(nvmrw2) < size)
			return -1;
		memcpy(buf, nvmrw2, size);
		break;
	default:
		return -1;
	}
	return 0;
}

int vbe_write_nvm(enum nvm_type type, void *buf, uint32_t size)
{
	if (vbe_write_nvm_failure > 0) {
		fprintf(stderr, "Failed to write NVM (type=%d failure=%d)\n",
			type, vbe_write_nvm_failure);
		vbe_write_nvm_failure--;
		return -1;
	}

	/* Write NVM-RW contents (to EEPROM for example) */
	switch (type) {
	case NVM_TYPE_RW_PRIMARY:
		memcpy(nvmrw1, buf, size);
		break;
	case NVM_TYPE_RW_SECONDARY:
		memcpy(nvmrw2, buf, size);
		break;
	default:
		return -1;
	}
	return 0;
}

static void install_nvm(enum nvm_type type,
			uint32_t min_kernel_data_key_version,
			uint32_t min_kernel_version,
			uint32_t update_count)
{
	struct nvmrw nvm = {
		.struct_magic = NVM_RW_MAGIC,
		.struct_major_version = NVM_HEADER_VERSION_MAJOR,
		.struct_minor_version = NVM_HEADER_VERSION_MINOR,
		.struct_size = sizeof(struct nvmrw),
		.min_kernel_data_key_version = min_kernel_data_key_version,
		.min_kernel_version = min_kernel_version,
		.update_count = update_count,
	};

	/* Compute HMAC */
	hmac(VB2_HASH_SHA256, secrets.nvm_rw, BDB_SECRET_SIZE,
	     &nvm, nvm.struct_size - sizeof(nvm.hmac),
	     nvm.hmac, sizeof(nvm.hmac));

	/* Install NVM-RWs (in EEPROM for example) */
	switch (type) {
	case NVM_TYPE_RW_PRIMARY:
		memset(nvmrw1, 0, sizeof(nvmrw1));
		memcpy(nvmrw1, &nvm, sizeof(nvm));
		break;
	case NVM_TYPE_RW_SECONDARY:
		memset(nvmrw2, 0, sizeof(nvmrw2));
		memcpy(nvmrw2, &nvm, sizeof(nvm));
		break;
	default:
		fprintf(stderr, "Unsupported NVM type (%d)\n", type);
		exit(2);
		return;
	}
}

static void test_nvm_read(void)
{
	struct vba_context ctx = {
		.bdb = NULL,
		.ro_secrets = &secrets,
	};
	struct nvmrw *nvm;
	uint8_t nvmrw1_copy[NVM_RW_MAX_STRUCT_SIZE];
	uint8_t nvmrw2_copy[NVM_RW_MAX_STRUCT_SIZE];

	install_nvm(NVM_TYPE_RW_PRIMARY, 0, 1, 0);
	install_nvm(NVM_TYPE_RW_SECONDARY, 1, 0, 0);
	memcpy(nvmrw1_copy, nvmrw1, sizeof(nvmrw1));
	memcpy(nvmrw2_copy, nvmrw2, sizeof(nvmrw2));

	/* Test nvm_read: both good -> pick primary, no sync */
	memset(&ctx.nvmrw, 0, sizeof(ctx.nvmrw));
	TEST_SUCC(nvmrw_read(&ctx), NULL);
	TEST_SUCC(memcmp(&ctx.nvmrw, nvmrw1, sizeof(*nvm)), NULL);
	TEST_SUCC(memcmp(nvmrw1, nvmrw1_copy, sizeof(nvmrw1)), NULL);
	TEST_SUCC(memcmp(nvmrw2, nvmrw2_copy, sizeof(nvmrw2)), NULL);

	/* Test nvm_read: primary bad -> pick secondary */
	install_nvm(NVM_TYPE_RW_PRIMARY, 0, 1, 0);
	install_nvm(NVM_TYPE_RW_SECONDARY, 1, 0, 0);
	memcpy(nvmrw2_copy, nvmrw2, sizeof(*nvm));
	nvm = (struct nvmrw *)nvmrw1;
	nvm->hmac[0] ^= 0xff;
	memset(&ctx.nvmrw, 0, sizeof(ctx.nvmrw));
	TEST_SUCC(nvmrw_read(&ctx), NULL);
	TEST_SUCC(memcmp(&ctx.nvmrw, nvmrw2, sizeof(*nvm)), NULL);
	TEST_SUCC(memcmp(nvmrw1, nvmrw2_copy, sizeof(nvmrw2)), NULL);
	TEST_SUCC(memcmp(nvmrw2, nvmrw2_copy, sizeof(nvmrw2)), NULL);

	/* Test nvm_read: secondary bad -> pick primary */
	install_nvm(NVM_TYPE_RW_PRIMARY, 0, 1, 0);
	install_nvm(NVM_TYPE_RW_SECONDARY, 1, 0, 0);
	memcpy(nvmrw1_copy, nvmrw1, sizeof(*nvm));
	nvm = (struct nvmrw *)nvmrw2;
	nvm->hmac[0] ^= 0xff;
	memset(&ctx.nvmrw, 0, sizeof(ctx.nvmrw));
	TEST_SUCC(nvmrw_read(&ctx), NULL);
	TEST_SUCC(memcmp(&ctx.nvmrw, nvmrw1, sizeof(*nvm)), NULL);
	TEST_SUCC(memcmp(nvmrw1, nvmrw1_copy, sizeof(nvmrw1)), NULL);
	TEST_SUCC(memcmp(nvmrw2, nvmrw1_copy, sizeof(nvmrw1)), NULL);

	/* Test nvm_read: both bad */
	nvm = (struct nvmrw *)nvmrw1;
	nvm->hmac[0] ^= 0xff;
	nvm = (struct nvmrw *)nvmrw2;
	nvm->hmac[0] ^= 0xff;
	memset(&ctx.nvmrw, 0, sizeof(ctx.nvmrw));
	TEST_EQ(nvmrw_read(&ctx), BDB_ERROR_NVM_RW_INVALID_HMAC, NULL);

	/* Test update count: secondary new -> pick secondary */
	install_nvm(NVM_TYPE_RW_PRIMARY, 0, 1, 0);
	install_nvm(NVM_TYPE_RW_SECONDARY, 1, 0, 1);
	memcpy(nvmrw2_copy, nvmrw2, sizeof(*nvm));
	memset(&ctx.nvmrw, 0, sizeof(ctx.nvmrw));
	TEST_SUCC(nvmrw_read(&ctx), NULL);
	TEST_SUCC(memcmp(&ctx.nvmrw, nvmrw2, sizeof(*nvm)), NULL);
	TEST_SUCC(memcmp(nvmrw1, nvmrw2_copy, sizeof(nvmrw1)), NULL);
	TEST_SUCC(memcmp(nvmrw2, nvmrw2_copy, sizeof(nvmrw2)), NULL);

	/* Test old reader -> minor version downgrade */
	install_nvm(NVM_TYPE_RW_PRIMARY, 0, 1, 0);
	install_nvm(NVM_TYPE_RW_SECONDARY, 1, 0, 1);
	memset(&ctx.nvmrw, 0, sizeof(ctx.nvmrw));
	nvm = (struct nvmrw *)nvmrw1;
	nvm->struct_minor_version++;
	nvm->struct_size++;
	TEST_SUCC(nvmrw_read(&ctx), NULL);
	TEST_EQ(ctx.nvmrw.struct_minor_version, NVM_HEADER_VERSION_MINOR, NULL);
	TEST_EQ(ctx.nvmrw.struct_size, sizeof(*nvm), NULL);
}

static void verify_nvm_write(struct vba_context *ctx,
			     int expected_result)
{
	struct nvmrw *nvmrw;
	struct nvmrw *nvm = &ctx->nvmrw;

	TEST_EQ(nvmrw_write(ctx, NVM_TYPE_RW_PRIMARY), expected_result, NULL);

	if (expected_result != BDB_SUCCESS)
		return;

	nvmrw = (struct nvmrw *)nvmrw1;
	TEST_EQ(nvmrw->min_kernel_data_key_version,
		nvm->min_kernel_data_key_version, NULL);
	TEST_EQ(nvmrw->min_kernel_version, nvm->min_kernel_version, NULL);
	TEST_EQ(nvmrw->update_count, nvm->update_count, NULL);
}

static void test_nvm_write(void)
{
	struct vba_context ctx = {
		.bdb = NULL,
		.ro_secrets = &secrets,
	};
	struct nvmrw nvm = {
		.struct_magic = NVM_RW_MAGIC,
		.struct_major_version = NVM_HEADER_VERSION_MAJOR,
		.struct_minor_version = NVM_HEADER_VERSION_MINOR,
		.struct_size = sizeof(struct nvmrw),
		.min_kernel_data_key_version = 1,
		.min_kernel_version = 2,
		.update_count = 3,
	};

	/* Test normal case */
	memcpy(&ctx.nvmrw, &nvm, sizeof(nvm));
	vbe_write_nvm_failure = 0;
	verify_nvm_write(&ctx, BDB_SUCCESS);

	/* Test write failure: once */
	memcpy(&ctx.nvmrw, &nvm, sizeof(nvm));
	vbe_write_nvm_failure = 1;
	verify_nvm_write(&ctx, BDB_SUCCESS);

	/* Test write failure: twice */
	memcpy(&ctx.nvmrw, &nvm, sizeof(nvm));
	vbe_write_nvm_failure = 2;
	verify_nvm_write(&ctx, BDB_ERROR_NVM_WRITE);

	/* Test invalid struct magic */
	memcpy(&ctx.nvmrw, &nvm, sizeof(nvm));
	ctx.nvmrw.struct_magic ^= 0xff;
	verify_nvm_write(&ctx, BDB_ERROR_NVM_RW_MAGIC);

	/* Test struct size too small */
	memcpy(&ctx.nvmrw, &nvm, sizeof(nvm));
	ctx.nvmrw.struct_size = NVM_RW_MIN_STRUCT_SIZE - 1;
	verify_nvm_write(&ctx, BDB_ERROR_NVM_STRUCT_SIZE);

	/* Test struct size too large */
	memcpy(&ctx.nvmrw, &nvm, sizeof(nvm));
	ctx.nvmrw.struct_size = NVM_RW_MAX_STRUCT_SIZE + 1;
	verify_nvm_write(&ctx, BDB_ERROR_NVM_STRUCT_SIZE);

	/* Test invalid struct version */
	memcpy(&ctx.nvmrw, &nvm, sizeof(nvm));
	ctx.nvmrw.struct_major_version = NVM_HEADER_VERSION_MAJOR - 1;
	verify_nvm_write(&ctx, BDB_ERROR_NVM_STRUCT_VERSION);

	vbe_write_nvm_failure = 0;
}

static void verify_kernel_version(uint32_t min_kernel_data_key_version,
				  uint32_t new_kernel_data_key_version,
				  uint32_t min_kernel_version,
				  uint32_t new_kernel_version,
				  int expected_result)
{
	struct vba_context ctx = {
		.bdb = NULL,
		.ro_secrets = &secrets,
	};
	struct nvmrw *nvm = (struct nvmrw *)nvmrw1;
	uint32_t expected_kernel_data_key_version = min_kernel_data_key_version;
	uint32_t expected_kernel_version = min_kernel_version;
	int should_update = 0;

	if (min_kernel_data_key_version < new_kernel_data_key_version) {
		expected_kernel_data_key_version = new_kernel_data_key_version;
		should_update = 1;
	}
	if (min_kernel_version < new_kernel_version) {
		expected_kernel_version = new_kernel_version;
		should_update = 1;
	}

	install_nvm(NVM_TYPE_RW_PRIMARY, min_kernel_data_key_version,
		    min_kernel_version, 0);
	install_nvm(NVM_TYPE_RW_SECONDARY, 0, 0, 0);

	TEST_EQ(vba_update_kernel_version(&ctx, new_kernel_data_key_version,
					  new_kernel_version),
		expected_result, NULL);

	if (expected_result != BDB_SUCCESS)
		return;

	/* Check data key version */
	TEST_EQ(nvm->min_kernel_data_key_version,
		expected_kernel_data_key_version, NULL);
	/* Check kernel version */
	TEST_EQ(nvm->min_kernel_version, expected_kernel_version, NULL);
	/* Check update_count */
	TEST_EQ(nvm->update_count, 0 + should_update, NULL);
	/* Check sync if update is expected */
	if (should_update)
		TEST_SUCC(memcmp(nvmrw2, nvmrw1, sizeof(nvmrw1)), NULL);
}

static void test_update_kernel_version(void)
{
	/* Test update: data key version */
	verify_kernel_version(0, 1, 0, 0, BDB_SUCCESS);
	/* Test update: kernel version */
	verify_kernel_version(0, 0, 0, 1, BDB_SUCCESS);
	/* Test no update: data key version */
	verify_kernel_version(1, 0, 0, 0, BDB_SUCCESS);
	/* Test no update: kernel version */
	verify_kernel_version(0, 0, 1, 0, BDB_SUCCESS);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <keys_dir>", argv[0]);
		return -1;
	}
	printf("Running BDB SP-RW tests...\n");

	test_verify_aprw(argv[1]);
	test_nvm_read();
	test_nvm_write();
	test_update_kernel_version();

	return gTestSuccess ? 0 : 255;
}
