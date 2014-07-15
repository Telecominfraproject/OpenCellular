/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_kernel, part 3 - software sync
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "gbb_header.h"
#include "host_common.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_audio.h"
#include "vboot_common.h"
#include "vboot_kernel.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Mock data */
static VbCommonParams cparams;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static GoogleBinaryBlockHeader gbb;

static int trust_ec;
static int mock_in_rw;
static VbError_t in_rw_retval;
static int protect_retval;
static int ec_protected;
static int run_retval;
static int ec_run_image;
static int update_retval;
static int ec_updated;
static int get_expected_retval;
static int shutdown_request_calls_left;

static uint8_t mock_ec_hash[32];
static int mock_ec_hash_size;
static uint8_t want_ec_hash[32];
static int want_ec_hash_size;
static uint8_t mock_sha[32];

static uint32_t screens_displayed[8];
static uint32_t screens_count = 0;

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	Memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = &gbb;

	Memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

	/*
	 * Only the outermost vboot_api_kernel call sets vboot_api_kernel's
	 * vnc.  So clear it here too.
	 */
	Memset(VbApiKernelGetVnc(), 0, sizeof(VbNvContext));
	VbNvSetup(VbApiKernelGetVnc());
	VbNvTeardown(VbApiKernelGetVnc()); /* So CRC gets generated */

	Memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	trust_ec = 0;
	mock_in_rw = 0;
	ec_protected = 0;
	ec_run_image = 0;   /* 0 = RO, 1 = RW */
	ec_updated = 0;
	in_rw_retval = VBERROR_SUCCESS;
	protect_retval = VBERROR_SUCCESS;
	update_retval = VBERROR_SUCCESS;
	run_retval = VBERROR_SUCCESS;
	get_expected_retval = VBERROR_SUCCESS;
	shutdown_request_calls_left = -1;

	Memset(mock_ec_hash, 0, sizeof(mock_ec_hash));
	mock_ec_hash[0] = 42;
	mock_ec_hash_size = sizeof(mock_ec_hash);

	Memset(want_ec_hash, 0, sizeof(want_ec_hash));
	want_ec_hash[0] = 42;
	want_ec_hash_size = sizeof(want_ec_hash);

	Memset(mock_sha, 0, sizeof(want_ec_hash));
	mock_sha[0] = 42;

	// TODO: ensure these are actually needed

	Memset(screens_displayed, 0, sizeof(screens_displayed));
	screens_count = 0;
}

/* Mock functions */

uint32_t VbExIsShutdownRequested(void)
{
	if (shutdown_request_calls_left == 0)
		return 1;
	else if (shutdown_request_calls_left > 0)
		shutdown_request_calls_left--;

	return 0;
}

int VbExTrustEC(int devidx)
{
	return trust_ec;
}

VbError_t VbExEcRunningRW(int devidx, int *in_rw)
{
	*in_rw = mock_in_rw;
	return in_rw_retval;
}

VbError_t VbExEcProtectRW(int devidx)
{
	ec_protected = 1;
	return protect_retval;
}

VbError_t VbExEcDisableJump(int devidx)
{
	return run_retval;
}

VbError_t VbExEcJumpToRW(int devidx)
{
	ec_run_image = 1;
	return run_retval;
}

VbError_t VbExEcHashRW(int devidx, const uint8_t **hash, int *hash_size)
{
	*hash = mock_ec_hash;
	*hash_size = mock_ec_hash_size;
	return mock_ec_hash_size ? VBERROR_SUCCESS : VBERROR_SIMULATED;
}

VbError_t VbExEcGetExpectedRW(int devidx, enum VbSelectFirmware_t select,
                              const uint8_t **image, int *image_size)
{
	static uint8_t fake_image[64] = {5, 6, 7, 8};
	*image = fake_image;
	*image_size = sizeof(fake_image);
	return get_expected_retval;
}

VbError_t VbExEcGetExpectedRWHash(int devidx, enum VbSelectFirmware_t select,
				  const uint8_t **hash, int *hash_size)
{
	*hash = want_ec_hash;
	*hash_size = want_ec_hash_size;

	if (want_ec_hash_size == -1)
		return VBERROR_EC_GET_EXPECTED_HASH_FROM_IMAGE;
	else
		return want_ec_hash_size ? VBERROR_SUCCESS : VBERROR_SIMULATED;
}

uint8_t *internal_SHA256(const uint8_t *data, uint64_t len, uint8_t *digest)
{
	Memcpy(digest, mock_sha, sizeof(mock_sha));
	return digest;
}

VbError_t VbExEcUpdateRW(int devidx, const uint8_t *image, int image_size)
{
	ec_updated = 1;
	return update_retval;
}

VbError_t VbDisplayScreen(VbCommonParams *cparams, uint32_t screen, int force,
                          VbNvContext *vncptr)
{
	if (screens_count < ARRAY_SIZE(screens_displayed))
		screens_displayed[screens_count++] = screen;

	return VBERROR_SUCCESS;
}

static void test_ssync(VbError_t retval, int recovery_reason, const char *desc)
{
	uint32_t u;

	TEST_EQ(VbEcSoftwareSync(0, &cparams), retval, desc);
	VbNvGet(VbApiKernelGetVnc(), VBNV_RECOVERY_REQUEST, &u);
	TEST_EQ(u, recovery_reason, "  recovery reason");
}

/* Tests */

static void VbSoftwareSyncTest(void)
{
	/* Recovery cases */
	ResetMocks();
	shared->recovery_reason = 123;
	test_ssync(0, 0, "In recovery, EC-RO");
	TEST_EQ(ec_protected, 0, "  ec protected");

	ResetMocks();
	shared->recovery_reason = 123;
	mock_in_rw = 1;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   123, "Recovery needs EC-RO");

	/* AP-RO cases */
	ResetMocks();
	in_rw_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_UNKNOWN_IMAGE, "Unknown EC image");

	ResetMocks();
	shared->flags |= VBSD_LF_USE_RO_NORMAL;
	mock_in_rw = 1;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   0, "AP-RO needs EC-RO");

	ResetMocks();
	shared->flags |= VBSD_LF_USE_RO_NORMAL;
	test_ssync(0, 0, "AP-RO, EC-RO");
	TEST_EQ(ec_protected, 1, "  ec protected");
	TEST_EQ(ec_run_image, 0, "  ec run image");

	ResetMocks();
	shared->flags |= VBSD_LF_USE_RO_NORMAL;
	run_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_SOFTWARE_SYNC, "Stay in RO fail");

	ResetMocks();
	shared->flags |= VBSD_LF_USE_RO_NORMAL;
	protect_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_SIMULATED,
		   VBNV_RECOVERY_EC_PROTECT, "Protect error");

	/* No longer check for shutdown requested */
	ResetMocks();
	shared->flags |= VBSD_LF_USE_RO_NORMAL;
	shutdown_request_calls_left = 0;
	test_ssync(0, 0, "AP-RO shutdown requested");

	/* Calculate hashes */
	ResetMocks();
	mock_ec_hash_size = 0;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_HASH_FAILED, "Bad EC hash");

	ResetMocks();
	mock_ec_hash_size = 16;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_HASH_SIZE, "Bad EC hash size");

	ResetMocks();
	want_ec_hash_size = 0;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_EXPECTED_HASH, "Bad precalculated hash");

	ResetMocks();
	want_ec_hash_size = 16;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_EXPECTED_HASH,
		   "Bad precalculated hash size");

	ResetMocks();
	mock_in_rw = 1;
	want_ec_hash_size = -1;
	test_ssync(0, 0, "No precomputed hash");

	ResetMocks();
	want_ec_hash_size = -1;
	get_expected_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_EXPECTED_IMAGE, "Can't fetch image");

	/* Updates required */
	ResetMocks();
	mock_in_rw = 1;
	want_ec_hash[0]++;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_HASH_MISMATCH,
		   "Precalculated hash mismatch");

	ResetMocks();
	mock_in_rw = 1;
	mock_ec_hash[0]++;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   0, "Pending update needs reboot");

	ResetMocks();
	mock_ec_hash[0]++;
	test_ssync(0, 0, "Update without reboot");
	TEST_EQ(ec_protected, 1, "  ec protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_updated, 1, "  ec updated");

	ResetMocks();
	mock_ec_hash[0]++;
	update_retval = VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   0, "Reboot after update");
	TEST_EQ(ec_updated, 1, "  ec updated");

	ResetMocks();
	mock_ec_hash[0]++;
	update_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_UPDATE, "Update failed");

	ResetMocks();
	mock_ec_hash[0]++;
	shared->flags |= VBSD_EC_SLOW_UPDATE;
	test_ssync(0, 0, "Slow update");
	TEST_EQ(screens_displayed[0], VB_SCREEN_WAIT, "  wait screen");

	/* RW cases, no update */
	ResetMocks();
	mock_in_rw = 1;
	test_ssync(0, 0, "AP-RW, EC-RW");

	ResetMocks();
	test_ssync(0, 0, "AP-RW, EC-RO -> EC-RW");
	TEST_EQ(ec_protected, 1, "  ec protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_updated, 0, "  ec updated");

	ResetMocks();
	run_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_JUMP_RW, "Jump to RW fail");

	ResetMocks();
	run_retval = VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   0, "Jump to RW fail because locked");

	ResetMocks();
	protect_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_SIMULATED,
		   VBNV_RECOVERY_EC_PROTECT, "Protect error");

	/* No longer check for shutdown requested */
	ResetMocks();
	shutdown_request_calls_left = 0;
	test_ssync(0, 0,
		   "AP-RW, EC-RO -> EC-RW shutdown requested");

	ResetMocks();
	mock_in_rw = 1;
	shutdown_request_calls_left = 0;
	test_ssync(0, 0, "AP-RW shutdown requested");
}

int main(void)
{
	VbSoftwareSyncTest();

	if (vboot_api_stub_check_memory())
		return 255;

	return gTestSuccess ? 0 : 255;
}
