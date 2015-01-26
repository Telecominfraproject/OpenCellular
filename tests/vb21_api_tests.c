/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for api library, new style structs
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

#include "host_key2.h"
#include "host_signature2.h"

#include "test_common.h"

/* Common context for tests */
static uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE]
	__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
static struct vb2_context ctx;
static struct vb2_shared_data *sd;

static const uint8_t mock_body[320] = "Mock body";
static const int mock_body_size = sizeof(mock_body);
static const int mock_hash_alg = VB2_HASH_SHA256;
static int mock_sig_size;

static const struct vb2_guid test_guid[4] = {
	{.raw = {0x11}},
	{.raw = {0x22}},
	{.raw = {0x33}},
	{.raw = {0x44}},
};

/* Mocked function data */

static enum {
	HWCRYPTO_DISABLED,
	HWCRYPTO_ENABLED,
	HWCRYPTO_FORBIDDEN,
} hwcrypto_state;

static struct vb2_digest_context hwcrypto_emulation_dc;

static int retval_hwcrypto;
static int retval_vb2_load_fw_keyblock;
static int retval_vb2_load_fw_preamble;

/* Type of test to reset for */
enum reset_type {
	FOR_MISC,
	FOR_EXTEND_HASH,
	FOR_CHECK_HASH,
};

static void reset_common_data(enum reset_type t)
{
	const struct vb2_private_key *hash_key;
	struct vb2_fw_preamble *pre;
	struct vb2_signature *sig;
	uint32_t sig_offset;

	int i;

	memset(workbuf, 0xaa, sizeof(workbuf));

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);

	vb2_init_context(&ctx);
	sd = vb2_get_sd(&ctx);

	vb2_nv_init(&ctx);

	vb2_secdata_create(&ctx);
	vb2_secdata_init(&ctx);

	memset(&hwcrypto_emulation_dc, 0, sizeof(hwcrypto_emulation_dc));
	retval_hwcrypto = VB2_SUCCESS;
	retval_vb2_load_fw_keyblock = VB2_SUCCESS;
	retval_vb2_load_fw_preamble = VB2_SUCCESS;

	vb2_private_key_hash(&hash_key, mock_hash_alg);

	sd->workbuf_preamble_offset = ctx.workbuf_used;
	pre = (struct vb2_fw_preamble *)
		(ctx.workbuf + sd->workbuf_preamble_offset);
	pre->hash_count = 3;
	pre->hash_offset = sig_offset = sizeof(*pre);
	if (hwcrypto_state == HWCRYPTO_FORBIDDEN)
		pre->flags = VB2_FIRMWARE_PREAMBLE_DISALLOW_HWCRYPTO;
	else
		pre->flags = 0;

	for (i = 0; i < 3; i++) {
		vb2_sign_data(&sig, mock_body, mock_body_size - 16 * i,
			      hash_key, NULL);
		memcpy(&sig->guid, test_guid + i, sizeof(sig->guid));
		memcpy((uint8_t *)pre + sig_offset, sig, sig->c.total_size);
		sig_offset += sig->c.total_size;
		mock_sig_size = sig->c.total_size;
		free(sig);
	}

	sd->workbuf_preamble_size = sig_offset;
	ctx.workbuf_used = sd->workbuf_preamble_offset
		+ sd->workbuf_preamble_size;

	if (t == FOR_EXTEND_HASH || t == FOR_CHECK_HASH)
		vb2api_init_hash2(&ctx, test_guid, NULL);

	if (t == FOR_CHECK_HASH)
		vb2api_extend_hash(&ctx, mock_body, mock_body_size);
};

/* Mocked functions */

int vb2_load_fw_keyblock(struct vb2_context *ctx)
{
	return retval_vb2_load_fw_keyblock;
}

int vb2_load_fw_preamble(struct vb2_context *ctx)
{
	return retval_vb2_load_fw_preamble;
}

int vb2ex_hwcrypto_digest_init(enum vb2_hash_algorithm hash_alg,
			       uint32_t data_size)
{
	vb2_digest_init(&hwcrypto_emulation_dc, hash_alg);

	switch (hwcrypto_state) {
	case HWCRYPTO_DISABLED:
		return VB2_ERROR_EX_HWCRYPTO_UNSUPPORTED;
	case HWCRYPTO_ENABLED:
		if (hash_alg != mock_hash_alg)
			return VB2_ERROR_SHA_INIT_ALGORITHM;
		else
			return retval_hwcrypto;
	case HWCRYPTO_FORBIDDEN:
	default:
		return VB2_ERROR_UNKNOWN;
	}
}

int vb2ex_hwcrypto_digest_extend(const uint8_t *buf,
				 uint32_t size)
{
	vb2_digest_extend(&hwcrypto_emulation_dc, buf, size);

	if (hwcrypto_state != HWCRYPTO_ENABLED)
		return VB2_ERROR_UNKNOWN;

	return retval_hwcrypto;
}

int vb2ex_hwcrypto_digest_finalize(uint8_t *digest,
				   uint32_t digest_size)
{
	vb2_digest_finalize(&hwcrypto_emulation_dc, digest, digest_size);

	if (hwcrypto_state != HWCRYPTO_ENABLED)
		return VB2_ERROR_UNKNOWN;

	return retval_hwcrypto;
}

/* Tests */

static void phase3_tests(void)
{
	reset_common_data(FOR_MISC);
	TEST_SUCC(vb2api_fw_phase3(&ctx), "phase3 good");

	reset_common_data(FOR_MISC);
	retval_vb2_load_fw_keyblock = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_fw_phase3(&ctx), VB2_ERROR_MOCK, "phase3 keyblock");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_RO_INVALID_RW, "  recovery reason");

	reset_common_data(FOR_MISC);
	retval_vb2_load_fw_preamble = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_fw_phase3(&ctx), VB2_ERROR_MOCK, "phase3 keyblock");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_RO_INVALID_RW, "  recovery reason");
}

static void init_hash_tests(void)
{
	struct vb2_fw_preamble *pre;
	struct vb2_signature *sig;
	int wb_used_before;
	uint32_t size;

	reset_common_data(FOR_MISC);
	pre = (struct vb2_fw_preamble *)
		(ctx.workbuf + sd->workbuf_preamble_offset);
	sig = (struct vb2_signature *)((uint8_t *)pre + pre->hash_offset);

	wb_used_before = ctx.workbuf_used;
	TEST_SUCC(vb2api_init_hash2(&ctx, test_guid, &size),
		  "init hash good");
	TEST_EQ(sd->workbuf_hash_offset,
		(wb_used_before + (VB2_WORKBUF_ALIGN - 1)) &
		~(VB2_WORKBUF_ALIGN - 1),
		"hash context offset");
	TEST_EQ(sd->workbuf_hash_size, sizeof(struct vb2_digest_context),
		"hash context size");
	TEST_EQ(ctx.workbuf_used,
		sd->workbuf_hash_offset + sd->workbuf_hash_size,
		"hash uses workbuf");
	TEST_EQ(sd->hash_tag,
		sd->workbuf_preamble_offset + pre->hash_offset,
		"hash signature offset");
	TEST_EQ(sd->hash_remaining_size, mock_body_size, "hash remaining");

	wb_used_before = ctx.workbuf_used;
	TEST_SUCC(vb2api_init_hash2(&ctx, test_guid + 2, NULL),
		  "init hash again");
	TEST_EQ(ctx.workbuf_used, wb_used_before, "init hash reuses context");
	TEST_EQ(sd->hash_tag,
		sd->workbuf_preamble_offset + pre->hash_offset +
		2 * mock_sig_size,
		"hash signature offset 2");

	reset_common_data(FOR_MISC);
	TEST_EQ(vb2api_init_hash2(&ctx, test_guid + 3, &size),
		VB2_ERROR_API_INIT_HASH_GUID, "init hash invalid guid");

	reset_common_data(FOR_MISC);
	sd->workbuf_preamble_size = 0;
	TEST_EQ(vb2api_init_hash2(&ctx, test_guid, &size),
		VB2_ERROR_API_INIT_HASH_PREAMBLE, "init hash preamble");

	reset_common_data(FOR_MISC);
	ctx.workbuf_used =
		ctx.workbuf_size - sizeof(struct vb2_digest_context) + 8;
	TEST_EQ(vb2api_init_hash2(&ctx, test_guid, &size),
		VB2_ERROR_API_INIT_HASH_WORKBUF, "init hash workbuf");

	reset_common_data(FOR_MISC);
	sig->hash_alg = VB2_HASH_INVALID;
	TEST_EQ(vb2api_init_hash2(&ctx, test_guid, &size),
		VB2_ERROR_SHA_INIT_ALGORITHM, "init hash algorithm");

	if (hwcrypto_state == HWCRYPTO_ENABLED) {
		reset_common_data(FOR_MISC);
		retval_hwcrypto = VB2_ERROR_MOCK;
		TEST_EQ(vb2api_init_hash2(&ctx, test_guid, &size),
			VB2_ERROR_MOCK, "init hash use hwcrypto");
	}
}

static void extend_hash_tests(void)
{
	struct vb2_digest_context *dc;

	reset_common_data(FOR_EXTEND_HASH);
	TEST_SUCC(vb2api_extend_hash(&ctx, mock_body, 32),
		"hash extend good");
	TEST_EQ(sd->hash_remaining_size, mock_body_size - 32,
		"hash extend remaining");
	TEST_SUCC(vb2api_extend_hash(&ctx, mock_body, mock_body_size - 32),
		"hash extend again");
	TEST_EQ(sd->hash_remaining_size, 0, "hash extend remaining 2");

	reset_common_data(FOR_EXTEND_HASH);
	sd->workbuf_hash_size = 0;
	TEST_EQ(vb2api_extend_hash(&ctx, mock_body, mock_body_size),
		VB2_ERROR_API_EXTEND_HASH_WORKBUF, "hash extend no workbuf");

	reset_common_data(FOR_EXTEND_HASH);
	TEST_EQ(vb2api_extend_hash(&ctx, mock_body, mock_body_size + 1),
		VB2_ERROR_API_EXTEND_HASH_SIZE, "hash extend too much");

	reset_common_data(FOR_EXTEND_HASH);
	TEST_EQ(vb2api_extend_hash(&ctx, mock_body, 0),
		VB2_ERROR_API_EXTEND_HASH_SIZE, "hash extend empty");

	if (hwcrypto_state == HWCRYPTO_ENABLED) {
		reset_common_data(FOR_EXTEND_HASH);
		retval_hwcrypto = VB2_ERROR_MOCK;
		TEST_EQ(vb2api_extend_hash(&ctx, mock_body, mock_body_size),
			VB2_ERROR_MOCK, "hash extend use hwcrypto");
	} else {
		reset_common_data(FOR_EXTEND_HASH);
		dc = (struct vb2_digest_context *)
			(ctx.workbuf + sd->workbuf_hash_offset);
		dc->hash_alg = VB2_HASH_INVALID;
		TEST_EQ(vb2api_extend_hash(&ctx, mock_body, mock_body_size),
			VB2_ERROR_SHA_EXTEND_ALGORITHM, "hash extend fail");
	}
}

static void check_hash_tests(void)
{
	struct vb2_fw_preamble *pre;
	struct vb2_signature *sig;
	struct vb2_digest_context *dc;

	reset_common_data(FOR_CHECK_HASH);
	pre = (struct vb2_fw_preamble *)
		(ctx.workbuf + sd->workbuf_preamble_offset);
	sig = (struct vb2_signature *)((uint8_t *)pre + pre->hash_offset);
	dc = (struct vb2_digest_context *)
		(ctx.workbuf + sd->workbuf_hash_offset);

	TEST_SUCC(vb2api_check_hash(&ctx), "check hash good");

	reset_common_data(FOR_CHECK_HASH);
	sd->hash_tag = 0;
	TEST_EQ(vb2api_check_hash(&ctx),
		VB2_ERROR_API_CHECK_HASH_TAG, "check hash tag");

	reset_common_data(FOR_CHECK_HASH);
	sd->workbuf_hash_size = 0;
	TEST_EQ(vb2api_check_hash(&ctx),
		VB2_ERROR_API_CHECK_HASH_WORKBUF, "check hash no workbuf");

	reset_common_data(FOR_CHECK_HASH);
	sd->hash_remaining_size = 1;
	TEST_EQ(vb2api_check_hash(&ctx),
		VB2_ERROR_API_CHECK_HASH_SIZE, "check hash size");

	reset_common_data(FOR_CHECK_HASH);
	ctx.workbuf_used = ctx.workbuf_size;
	TEST_EQ(vb2api_check_hash(&ctx),
		VB2_ERROR_API_CHECK_HASH_WORKBUF_DIGEST, "check hash workbuf");

	reset_common_data(FOR_CHECK_HASH);
	*((uint8_t *)sig + sig->sig_offset) ^= 0x55;
	TEST_EQ(vb2api_check_hash(&ctx),
		VB2_ERROR_API_CHECK_HASH_SIG, "check hash sig");

	if (hwcrypto_state == HWCRYPTO_ENABLED) {
		reset_common_data(FOR_CHECK_HASH);
		retval_hwcrypto = VB2_ERROR_MOCK;
		TEST_EQ(vb2api_check_hash(&ctx),
			VB2_ERROR_MOCK, "check hash use hwcrypto");
	} else {
		reset_common_data(FOR_CHECK_HASH);
		dc->hash_alg = VB2_HASH_INVALID;
		*((uint8_t *)sig + sig->sig_offset) ^= 0x55;
		TEST_EQ(vb2api_check_hash(&ctx),
			VB2_ERROR_SHA_FINALIZE_ALGORITHM, "check hash finaliz");
	}
}

int main(int argc, char* argv[])
{
	phase3_tests();

	fprintf(stderr, "Running hash API tests without hwcrypto support...\n");
	hwcrypto_state = HWCRYPTO_DISABLED;
	init_hash_tests();
	extend_hash_tests();
	check_hash_tests();

	fprintf(stderr, "Running hash API tests with hwcrypto support...\n");
	hwcrypto_state = HWCRYPTO_ENABLED;
	init_hash_tests();
	extend_hash_tests();
	check_hash_tests();

	fprintf(stderr, "Running hash API tests with forbidden hwcrypto...\n");
	hwcrypto_state = HWCRYPTO_FORBIDDEN;
	init_hash_tests();
	extend_hash_tests();
	check_hash_tests();

	return gTestSuccess ? 0 : 255;
}
