/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for misc library
 */

#include <stdio.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "2secdata.h"
#include "test_common.h"

/* Common context for tests */
static uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE]
	__attribute__ ((aligned (VB2_WORKBUF_ALIGN)));
static struct vb2_context cc;
static struct vb2_shared_data *sd;

const char mock_body[320] = "Mock body";
const int mock_body_size = sizeof(mock_body);
const int mock_algorithm = VB2_ALG_RSA2048_SHA256;
const int mock_hash_alg = VB2_HASH_SHA256;
static const uint8_t mock_hwid_digest[VB2_GBB_HWID_DIGEST_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

/* Mocked function data */

static int retval_vb2_fw_parse_gbb;
static int retval_vb2_check_dev_switch;
static int retval_vb2_check_tpm_clear;
static int retval_vb2_select_fw_slot;

/* Type of test to reset for */
enum reset_type {
	FOR_MISC,
};

static void reset_common_data(enum reset_type t)
{
	memset(workbuf, 0xaa, sizeof(workbuf));

	memset(&cc, 0, sizeof(cc));
	cc.workbuf = workbuf;
	cc.workbuf_size = sizeof(workbuf);

	vb2_init_context(&cc);
	sd = vb2_get_sd(&cc);

	vb2_nv_init(&cc);

	vb2_secdata_create(&cc);
	vb2_secdata_init(&cc);

	retval_vb2_fw_parse_gbb = VB2_SUCCESS;
	retval_vb2_check_dev_switch = VB2_SUCCESS;
	retval_vb2_check_tpm_clear = VB2_SUCCESS;
	retval_vb2_select_fw_slot = VB2_SUCCESS;

	memcpy(sd->gbb_hwid_digest, mock_hwid_digest,
	       sizeof(sd->gbb_hwid_digest));
};

/* Mocked functions */

int vb2_fw_parse_gbb(struct vb2_context *ctx)
{
	return retval_vb2_fw_parse_gbb;
}

int vb2_check_dev_switch(struct vb2_context *ctx)
{
	return retval_vb2_check_dev_switch;
}

int vb2_check_tpm_clear(struct vb2_context *ctx)
{
	return retval_vb2_check_tpm_clear;
}

int vb2_select_fw_slot(struct vb2_context *ctx)
{
	return retval_vb2_select_fw_slot;
}

/* Tests */

static void misc_tests(void)
{
	/* Test secdata passthru functions */
	reset_common_data(FOR_MISC);
	/* Corrupt secdata so initial check will fail */
	cc.secdata[0] ^= 0x42;
	TEST_EQ(vb2api_secdata_check(&cc), VB2_ERROR_SECDATA_CRC,
		"secdata check");
	TEST_SUCC(vb2api_secdata_create(&cc), "secdata create");
	TEST_SUCC(vb2api_secdata_check(&cc), "secdata check 2");

	/* Test fail passthru */
	reset_common_data(FOR_MISC);
	vb2api_fail(&cc, 12, 34);
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST),
		12, "vb2api_fail request");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_SUBCODE),
		34, "vb2api_fail subcode");
}

static void phase1_tests(void)
{
	reset_common_data(FOR_MISC);
	TEST_SUCC(vb2api_fw_phase1(&cc), "phase1 good");
	TEST_EQ(sd->recovery_reason, 0, "  not recovery");
	TEST_EQ(cc.flags & VB2_CONTEXT_RECOVERY_MODE, 0, "  recovery flag");
	TEST_EQ(cc.flags & VB2_CONTEXT_CLEAR_RAM, 0, "  clear ram flag");

	reset_common_data(FOR_MISC);
	retval_vb2_fw_parse_gbb = VB2_ERROR_GBB_MAGIC;
	TEST_EQ(vb2api_fw_phase1(&cc), VB2_ERROR_API_PHASE1_RECOVERY,
		"phase1 gbb");
	TEST_EQ(sd->recovery_reason, VB2_RECOVERY_GBB_HEADER,
		"  recovery reason");
	TEST_NEQ(cc.flags & VB2_CONTEXT_RECOVERY_MODE, 0, "  recovery flag");
	TEST_NEQ(cc.flags & VB2_CONTEXT_CLEAR_RAM, 0, "  clear ram flag");


	reset_common_data(FOR_MISC);
	retval_vb2_check_dev_switch = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_fw_phase1(&cc), VB2_ERROR_API_PHASE1_RECOVERY,
		"phase1 dev switch");
	TEST_EQ(sd->recovery_reason, VB2_RECOVERY_DEV_SWITCH,
		"  recovery reason");
	TEST_NEQ(cc.flags & VB2_CONTEXT_RECOVERY_MODE, 0, "  recovery flag");
	TEST_NEQ(cc.flags & VB2_CONTEXT_CLEAR_RAM, 0, "  clear ram flag");

	reset_common_data(FOR_MISC);
	cc.secdata[0] ^= 0x42;
	TEST_EQ(vb2api_fw_phase1(&cc), VB2_ERROR_API_PHASE1_RECOVERY,
		"phase1 secdata");
	TEST_EQ(sd->recovery_reason, VB2_RECOVERY_SECDATA_INIT,
		"  recovery reason");
	TEST_NEQ(cc.flags & VB2_CONTEXT_RECOVERY_MODE, 0, "  recovery flag");
	TEST_NEQ(cc.flags & VB2_CONTEXT_CLEAR_RAM, 0, "  clear ram flag");
}

static void phase2_tests(void)
{
	reset_common_data(FOR_MISC);
	TEST_SUCC(vb2api_fw_phase2(&cc), "phase2 good");
	TEST_EQ(cc.flags & VB2_CONTEXT_CLEAR_RAM, 0, "  clear ram flag");

	reset_common_data(FOR_MISC);
	cc.flags |= VB2_CONTEXT_DEVELOPER_MODE;
	TEST_SUCC(vb2api_fw_phase2(&cc), "phase1 dev");
	TEST_NEQ(cc.flags & VB2_CONTEXT_CLEAR_RAM, 0, "  clear ram flag");

	reset_common_data(FOR_MISC);
	retval_vb2_check_tpm_clear = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_fw_phase2(&cc), VB2_ERROR_MOCK, "phase2 tpm clear");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_TPM_CLEAR_OWNER, "  recovery reason");

	reset_common_data(FOR_MISC);
	retval_vb2_select_fw_slot = VB2_ERROR_MOCK;
	TEST_EQ(vb2api_fw_phase2(&cc), VB2_ERROR_MOCK, "phase2 slot");
	TEST_EQ(vb2_nv_get(&cc, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_FW_SLOT, "  recovery reason");
}

static void get_pcr_digest_tests(void)
{
	uint8_t digest[VB2_PCR_DIGEST_RECOMMENDED_SIZE];
	uint8_t digest_org[VB2_PCR_DIGEST_RECOMMENDED_SIZE];
	uint32_t digest_size;

	reset_common_data(FOR_MISC);
	memset(digest_org, 0, sizeof(digest_org));

	digest_size = sizeof(digest);
	memset(digest, 0, sizeof(digest));
	TEST_SUCC(vb2api_get_pcr_digest(
			&cc, BOOT_MODE_PCR, digest, &digest_size),
		  "BOOT_MODE_PCR");
	TEST_EQ(digest_size, VB2_SHA1_DIGEST_SIZE, "BOOT_MODE_PCR digest size");
	TEST_TRUE(memcmp(digest, digest_org, digest_size),
		  "BOOT_MODE_PCR digest");

	digest_size = sizeof(digest);
	memset(digest, 0, sizeof(digest));
	TEST_SUCC(vb2api_get_pcr_digest(
			&cc, HWID_DIGEST_PCR, digest, &digest_size),
		  "HWID_DIGEST_PCR");
	TEST_EQ(digest_size, VB2_GBB_HWID_DIGEST_SIZE,
		"HWID_DIGEST_PCR digest size");
	TEST_FALSE(memcmp(digest, mock_hwid_digest, digest_size),
		   "HWID_DIGEST_PCR digest");

	digest_size = 1;
	TEST_EQ(vb2api_get_pcr_digest(&cc, BOOT_MODE_PCR, digest, &digest_size),
		VB2_ERROR_API_PCR_DIGEST_BUF,
		"BOOT_MODE_PCR buffer too small");

	TEST_EQ(vb2api_get_pcr_digest(
			&cc, HWID_DIGEST_PCR + 1, digest, &digest_size),
		VB2_ERROR_API_PCR_DIGEST,
		"invalid enum vb2_pcr_digest");
}

int main(int argc, char* argv[])
{
	misc_tests();
	phase1_tests();
	phase2_tests();

	get_pcr_digest_tests();

	return gTestSuccess ? 0 : 255;
}
