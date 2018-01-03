/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for kernel verification library
 */

#include <stdio.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "2secdata.h"
#include "vb2_common.h"
#include "test_common.h"

/* Common context for tests */
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE]
	__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
static struct vb2_workbuf wb;
static struct vb2_context cc;
static struct vb2_shared_data *sd;

/* Mocked function data */

static struct {
	struct vb2_gbb_header h;
	struct vb2_packed_key recovery_key;
	char recovery_key_data[32];
} mock_gbb;

static struct {
	/* Keyblock */
	struct {
		struct vb2_keyblock kb;
		char data_key_data[16];
		uint8_t kbdata[128];
		uint8_t hash[VB2_SHA512_DIGEST_SIZE];
	} k;
	/* Preamble follows keyblock */
	struct {
		struct vb2_kernel_preamble pre;
		uint8_t predata[128];
	} p;
} mock_vblock;

static int mock_read_res_fail_on_call;
static int mock_unpack_key_retval;
static int mock_verify_keyblock_retval;
static int mock_verify_preamble_retval;

/* Type of test to reset for */
enum reset_type {
	FOR_KEYBLOCK,
	FOR_PREAMBLE
};

static void rehash_keyblock(void)
{
	struct vb2_keyblock *kb = &mock_vblock.k.kb;
	struct vb2_signature *hashsig = &mock_vblock.k.kb.keyblock_hash;
	struct vb2_digest_context dc;


	hashsig->sig_offset = vb2_offset_of(hashsig, mock_vblock.k.hash);
	hashsig->sig_size = sizeof(mock_vblock.k.hash);
	hashsig->data_size = hashsig->sig_offset;
	vb2_digest_init(&dc, VB2_HASH_SHA512);
	vb2_digest_extend(&dc, (const uint8_t *)kb, hashsig->data_size);
	vb2_digest_finalize(&dc, mock_vblock.k.hash, hashsig->sig_size);
}

static void reset_common_data(enum reset_type t)
{
	struct vb2_keyblock *kb = &mock_vblock.k.kb;
	struct vb2_kernel_preamble *pre = &mock_vblock.p.pre;

	memset(workbuf, 0xaa, sizeof(workbuf));

	memset(&cc, 0, sizeof(cc));
	cc.workbuf = workbuf;
	cc.workbuf_size = sizeof(workbuf);
	vb2_workbuf_from_ctx(&cc, &wb);

	vb2_init_context(&cc);
	sd = vb2_get_sd(&cc);

	vb2_nv_init(&cc);

	vb2_secdatak_create(&cc);
	vb2_secdatak_init(&cc);

	mock_read_res_fail_on_call = 0;
	mock_unpack_key_retval = VB2_SUCCESS;
	mock_verify_keyblock_retval = VB2_SUCCESS;
	mock_verify_preamble_retval = VB2_SUCCESS;

	/* Set up mock data for verifying keyblock */
	sd->kernel_version_secdatak = 0x20002;
	vb2_secdatak_set(&cc, VB2_SECDATAK_VERSIONS, 0x20002);

	mock_gbb.recovery_key.algorithm = 11;
	mock_gbb.recovery_key.key_offset =
		vb2_offset_of(&mock_gbb.recovery_key,
			      &mock_gbb.recovery_key_data);
	mock_gbb.recovery_key.key_size = sizeof(mock_gbb.recovery_key_data);

	kb->keyblock_size = sizeof(mock_vblock.k);
	memcpy(kb->magic, KEY_BLOCK_MAGIC, KEY_BLOCK_MAGIC_SIZE);

	kb->keyblock_flags = VB2_KEY_BLOCK_FLAG_DEVELOPER_1 |
		VB2_KEY_BLOCK_FLAG_DEVELOPER_0 |
		VB2_KEY_BLOCK_FLAG_RECOVERY_1 | VB2_KEY_BLOCK_FLAG_RECOVERY_0;
	kb->header_version_major = KEY_BLOCK_HEADER_VERSION_MAJOR;
	kb->header_version_minor = KEY_BLOCK_HEADER_VERSION_MINOR;
	kb->data_key.algorithm = 7;
	kb->data_key.key_version = 2;
	kb->data_key.key_offset =
		vb2_offset_of(&mock_vblock.k, &mock_vblock.k.data_key_data) -
		vb2_offset_of(&mock_vblock.k, &kb->data_key);
	kb->data_key.key_size = sizeof(mock_vblock.k.data_key_data);
	strcpy(mock_vblock.k.data_key_data, "data key data!!");
	rehash_keyblock();

	pre->preamble_size = sizeof(mock_vblock.p);
	pre->kernel_version = 2;

	/* If verifying preamble, verify keyblock first to set up data key */
	if (t == FOR_PREAMBLE)
		vb2_load_kernel_keyblock(&cc);
};

/* Mocked functions */

int vb2ex_read_resource(struct vb2_context *ctx,
			enum vb2_resource_index index,
			uint32_t offset,
			void *buf,
			uint32_t size)
{
	uint8_t *rptr;
	uint32_t rsize;

	if (--mock_read_res_fail_on_call == 0)
		return VB2_ERROR_MOCK;

	switch(index) {
	case VB2_RES_GBB:
		rptr = (uint8_t *)&mock_gbb;
		rsize = sizeof(mock_gbb);
		break;
	case VB2_RES_KERNEL_VBLOCK:
		rptr = (uint8_t *)&mock_vblock;
		rsize = sizeof(mock_vblock);
		break;
	default:
		return VB2_ERROR_EX_READ_RESOURCE_INDEX;
	}

	if (offset > rsize || offset + size > rsize)
		return VB2_ERROR_EX_READ_RESOURCE_SIZE;

	memcpy(buf, rptr + offset, size);
	return VB2_SUCCESS;
}

int vb2_unpack_key_buffer(struct vb2_public_key *key,
		   const uint8_t *buf,
		   uint32_t size)
{
	key->arrsize = 0;
	return mock_unpack_key_retval;
}

int vb2_verify_keyblock(struct vb2_keyblock *block,
			uint32_t size,
			const struct vb2_public_key *key,
			const struct vb2_workbuf *wb)
{
	return mock_verify_keyblock_retval;
}

int vb2_verify_kernel_preamble(struct vb2_kernel_preamble *preamble,
			       uint32_t size,
			       const struct vb2_public_key *key,
			       const struct vb2_workbuf *wb)
{
	return mock_verify_preamble_retval;
}

/* Tests */

static void verify_keyblock_hash_tests(void)
{
	struct vb2_keyblock *kb = &mock_vblock.k.kb;

	/* Test successful call */
	reset_common_data(FOR_KEYBLOCK);
	TEST_SUCC(vb2_verify_keyblock_hash(kb, kb->keyblock_size, &wb),
		  "Keyblock hash good");

	/* Sanity check keyblock */
	reset_common_data(FOR_KEYBLOCK);
	kb->magic[0] ^= 0xd0;
	TEST_EQ(vb2_verify_keyblock_hash(kb, kb->keyblock_size, &wb),
		VB2_ERROR_KEYBLOCK_MAGIC, "Keyblock sanity check");

	/*
	 * Sanity check should be looking at the keyblock hash struct, not the
	 * keyblock signature struct.
	 */
	reset_common_data(FOR_KEYBLOCK);
	kb->keyblock_hash.data_size = sizeof(*kb) - 1;
	TEST_EQ(vb2_verify_keyblock_hash(kb, kb->keyblock_size, &wb),
		VB2_ERROR_KEYBLOCK_SIGNED_TOO_LITTLE,
		"Keyblock check hash sig");

	reset_common_data(FOR_KEYBLOCK);
	wb.size = VB2_SHA512_DIGEST_SIZE - 1;
	TEST_EQ(vb2_verify_keyblock_hash(kb, kb->keyblock_size, &wb),
		VB2_ERROR_VDATA_WORKBUF_DIGEST,
		"Keyblock check hash workbuf digest");

	reset_common_data(FOR_KEYBLOCK);
	wb.size = VB2_SHA512_DIGEST_SIZE +
		sizeof(struct vb2_digest_context) - 1;
	TEST_EQ(vb2_verify_keyblock_hash(kb, kb->keyblock_size, &wb),
		VB2_ERROR_VDATA_WORKBUF_HASHING,
		"Keyblock check hash workbuf hashing");

	reset_common_data(FOR_KEYBLOCK);
	mock_vblock.k.data_key_data[0] ^= 0xa0;
	TEST_EQ(vb2_verify_keyblock_hash(kb, kb->keyblock_size, &wb),
		VB2_ERROR_KEYBLOCK_SIG_INVALID,
		"Keyblock check hash invalid");
}

static void load_kernel_keyblock_tests(void)
{
	struct vb2_keyblock *kb = &mock_vblock.k.kb;
	struct vb2_packed_key *k;
	int wb_used_before;

	/* Test successful call */
	reset_common_data(FOR_KEYBLOCK);
	wb_used_before = cc.workbuf_used;
	TEST_SUCC(vb2_load_kernel_keyblock(&cc), "Kernel keyblock good");
	TEST_NEQ(sd->flags & VB2_SD_FLAG_KERNEL_SIGNED, 0, "  Kernel signed");
	TEST_EQ(sd->kernel_version, 0x20000, "keyblock version");
	TEST_EQ(sd->vblock_preamble_offset, sizeof(mock_vblock.k),
		"preamble offset");
	TEST_EQ(sd->workbuf_data_key_offset, wb_used_before,
		"keyblock data key offset");
	TEST_EQ(cc.workbuf_used,
		vb2_wb_round_up(sd->workbuf_data_key_offset +
				sd->workbuf_data_key_size),
		"workbuf used");

	/* Make sure data key was properly saved */
	k = (struct vb2_packed_key *)(cc.workbuf + sd->workbuf_data_key_offset);
	TEST_EQ(k->algorithm, 7, "data key algorithm");
	TEST_EQ(k->key_version, 2, "data key version");
	TEST_EQ(k->key_size, sizeof(mock_vblock.k.data_key_data),
		"data key size");
	TEST_EQ(memcmp(cc.workbuf + sd->workbuf_data_key_offset +
		       k->key_offset, mock_vblock.k.data_key_data,
		       sizeof(mock_vblock.k.data_key_data)),
		0, "data key data");
	TEST_EQ(cc.workbuf_used,
		vb2_wb_round_up(sd->workbuf_data_key_offset +
				sd->workbuf_data_key_size),
		"workbuf used after");

	/* Test failures */
	reset_common_data(FOR_KEYBLOCK);
	mock_unpack_key_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_MOCK, "Kernel keyblock unpack key");

	reset_common_data(FOR_KEYBLOCK);
	cc.workbuf_used = cc.workbuf_size + VB2_WORKBUF_ALIGN -
			vb2_wb_round_up(sizeof(*kb));
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_WORKBUF_HEADER,
		"Kernel keyblock workbuf header");

	reset_common_data(FOR_KEYBLOCK);
	mock_read_res_fail_on_call = 1;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_MOCK, "Kernel keyblock read header");

	reset_common_data(FOR_KEYBLOCK);
	cc.workbuf_used = cc.workbuf_size + VB2_WORKBUF_ALIGN -
			vb2_wb_round_up(kb->keyblock_size);
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_WORKBUF,
		"Kernel keyblock workbuf");

	reset_common_data(FOR_KEYBLOCK);
	mock_read_res_fail_on_call = 2;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_MOCK, "Kernel keyblock read");

	/* Normally, require signed keyblock */
	reset_common_data(FOR_KEYBLOCK);
	mock_verify_keyblock_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_MOCK, "Verify keyblock");

	/* Not in dev mode */
	reset_common_data(FOR_KEYBLOCK);
	cc.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	mock_verify_keyblock_retval = VB2_ERROR_MOCK;
	TEST_SUCC(vb2_load_kernel_keyblock(&cc), "Kernel keyblock hash good");
	TEST_EQ(sd->flags & VB2_SD_FLAG_KERNEL_SIGNED, 0, "  Kernel signed");

	/* But we do in dev+rec mode */
	reset_common_data(FOR_KEYBLOCK);
	cc.flags |= VB2_CONTEXT_DEVELOPER_MODE | VB2_CONTEXT_RECOVERY_MODE;
	mock_verify_keyblock_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_MOCK, "Kernel keyblock dev+rec");

	/* Test keyblock flags matching mode */
	reset_common_data(FOR_KEYBLOCK);
	kb->keyblock_flags &= ~VB2_KEY_BLOCK_FLAG_DEVELOPER_0;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_DEV_FLAG,
		"Kernel keyblock dev only");

	reset_common_data(FOR_KEYBLOCK);
	kb->keyblock_flags &= ~VB2_KEY_BLOCK_FLAG_RECOVERY_0;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_REC_FLAG,
		"Kernel keyblock rec only");

	reset_common_data(FOR_KEYBLOCK);
	kb->keyblock_flags &= ~VB2_KEY_BLOCK_FLAG_RECOVERY_1;
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_REC_FLAG,
		"Kernel keyblock not rec");

	reset_common_data(FOR_KEYBLOCK);
	kb->keyblock_flags &= ~VB2_KEY_BLOCK_FLAG_DEVELOPER_0;
	kb->keyblock_flags &= ~VB2_KEY_BLOCK_FLAG_RECOVERY_0;
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_DEV_FLAG,
		"Kernel keyblock rec but not dev+rec");

	reset_common_data(FOR_KEYBLOCK);
	kb->keyblock_flags &= ~VB2_KEY_BLOCK_FLAG_DEVELOPER_0;
	kb->keyblock_flags &= ~VB2_KEY_BLOCK_FLAG_RECOVERY_0;
	cc.flags |= VB2_CONTEXT_DEVELOPER_MODE | VB2_CONTEXT_RECOVERY_MODE;
	TEST_SUCC(vb2_load_kernel_keyblock(&cc),
		  "Kernel keyblock flags dev+rec");

	/* System in dev mode ignores flags */
	reset_common_data(FOR_KEYBLOCK);
	cc.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	kb->keyblock_flags = 0;
	TEST_SUCC(vb2_load_kernel_keyblock(&cc), "Kernel keyblock dev flags");

	/* Test rollback */
	reset_common_data(FOR_KEYBLOCK);
	kb->data_key.key_version = 0x10000;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_VERSION_RANGE,
		"Kernel keyblock version range");

	reset_common_data(FOR_KEYBLOCK);
	kb->data_key.key_version = 1;
	TEST_EQ(vb2_load_kernel_keyblock(&cc),
		VB2_ERROR_KERNEL_KEYBLOCK_VERSION_ROLLBACK,
		"Kernel keyblock rollback");

	/* Rollback ok in developer mode */
	reset_common_data(FOR_KEYBLOCK);
	kb->data_key.key_version = 1;
	cc.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	TEST_SUCC(vb2_load_kernel_keyblock(&cc),
		  "Kernel keyblock rollback dev");

	/*
	 * Recovery keyblocks aren't versioned (and even if they were, it
	 * wouldn't be with the same version as a normal kernel).
	 */
	reset_common_data(FOR_KEYBLOCK);
	kb->data_key.key_version = 1;
	cc.flags |= VB2_CONTEXT_RECOVERY_MODE;
	TEST_SUCC(vb2_load_kernel_keyblock(&cc),
		  "Kernel keyblock rollback rec");
}

static void load_kernel_preamble_tests(void)
{
	struct vb2_kernel_preamble *pre = &mock_vblock.p.pre;
	int wb_used_before;
	//uint32_t v;

	/* Test successful call */
	reset_common_data(FOR_PREAMBLE);
	wb_used_before = cc.workbuf_used;
	TEST_SUCC(vb2_load_kernel_preamble(&cc), "preamble good");
	TEST_EQ(sd->kernel_version, 0x20002, "combined version");
	TEST_EQ(sd->workbuf_preamble_offset, wb_used_before,
		"preamble offset");
	TEST_EQ(sd->workbuf_preamble_size, pre->preamble_size, "preamble size");
	TEST_EQ(cc.workbuf_used,
		vb2_wb_round_up(sd->workbuf_preamble_offset +
				sd->workbuf_preamble_size),
		"workbuf used");

	/* Expected failures */
	reset_common_data(FOR_PREAMBLE);
	sd->workbuf_data_key_size = 0;
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_KERNEL_PREAMBLE2_DATA_KEY,
		"preamble no data key");

	reset_common_data(FOR_PREAMBLE);
	mock_unpack_key_retval = VB2_ERROR_UNPACK_KEY_HASH_ALGORITHM;
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_UNPACK_KEY_HASH_ALGORITHM,
		"preamble unpack data key");

	reset_common_data(FOR_PREAMBLE);
	cc.workbuf_used = cc.workbuf_size + VB2_WORKBUF_ALIGN -
			vb2_wb_round_up(sizeof(struct vb2_kernel_preamble));
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_KERNEL_PREAMBLE2_WORKBUF_HEADER,
		"preamble not enough workbuf for header");

	reset_common_data(FOR_PREAMBLE);
	sd->vblock_preamble_offset = sizeof(mock_vblock);
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_EX_READ_RESOURCE_SIZE,
		"preamble read header");

	reset_common_data(FOR_PREAMBLE);
	cc.workbuf_used = cc.workbuf_size + VB2_WORKBUF_ALIGN -
			vb2_wb_round_up(sizeof(mock_vblock.p));
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_KERNEL_PREAMBLE2_WORKBUF,
		"preamble not enough workbuf");

	reset_common_data(FOR_PREAMBLE);
	pre->preamble_size = sizeof(mock_vblock);
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_EX_READ_RESOURCE_SIZE,
		"preamble read full");

	reset_common_data(FOR_PREAMBLE);
	mock_verify_preamble_retval = VB2_ERROR_MOCK;
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_MOCK,
		"preamble verify");

	reset_common_data(FOR_PREAMBLE);
	pre->kernel_version = 0x10000;
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_KERNEL_PREAMBLE_VERSION_RANGE,
		"preamble version range");

	reset_common_data(FOR_PREAMBLE);
	pre->kernel_version = 1;
	TEST_EQ(vb2_load_kernel_preamble(&cc),
		VB2_ERROR_KERNEL_PREAMBLE_VERSION_ROLLBACK,
		"preamble version rollback");
}

int main(int argc, char* argv[])
{
	verify_keyblock_hash_tests();
	load_kernel_keyblock_tests();
	load_kernel_preamble_tests();

	return gTestSuccess ? 0 : 255;
}
