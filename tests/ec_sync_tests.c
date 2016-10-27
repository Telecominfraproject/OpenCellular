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
static int ec_ro_protected;
static int ec_rw_protected;
static int run_retval;
static int ec_run_image;
static int update_retval;
static int ec_ro_updated;
static int ec_rw_updated;
static int get_expected_retval;
static int shutdown_request_calls_left;

static uint8_t mock_ec_ro_hash[32];
static uint8_t mock_ec_rw_hash[32];
static int mock_ec_ro_hash_size;
static int mock_ec_rw_hash_size;
static uint8_t want_ec_hash[32];
static uint8_t update_hash;
static int want_ec_hash_size;
static VbNvContext mock_vnc;

static uint32_t screens_displayed[8];
static uint32_t screens_count = 0;

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = &gbb;

	memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

	memset(&mock_vnc, 0, sizeof(VbNvContext));
	VbNvSetup(&mock_vnc);
	VbNvTeardown(&mock_vnc); /* So CRC gets generated */

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	trust_ec = 0;
	mock_in_rw = 0;
	ec_ro_protected = 0;
	ec_rw_protected = 0;
	ec_run_image = 0;   /* 0 = RO, 1 = RW */
	ec_ro_updated = 0;
	ec_rw_updated = 0;
	in_rw_retval = VBERROR_SUCCESS;
	protect_retval = VBERROR_SUCCESS;
	update_retval = VBERROR_SUCCESS;
	run_retval = VBERROR_SUCCESS;
	get_expected_retval = VBERROR_SUCCESS;
	shutdown_request_calls_left = -1;

	memset(mock_ec_ro_hash, 0, sizeof(mock_ec_ro_hash));
	mock_ec_ro_hash[0] = 42;
	mock_ec_ro_hash_size = sizeof(mock_ec_ro_hash);

	memset(mock_ec_rw_hash, 0, sizeof(mock_ec_rw_hash));
	mock_ec_rw_hash[0] = 42;
	mock_ec_rw_hash_size = sizeof(mock_ec_rw_hash);

	memset(want_ec_hash, 0, sizeof(want_ec_hash));
	want_ec_hash[0] = 42;
	want_ec_hash_size = sizeof(want_ec_hash);

	update_hash = 42;

	// TODO: ensure these are actually needed

	memset(screens_displayed, 0, sizeof(screens_displayed));
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

VbError_t VbExEcProtect(int devidx, enum VbSelectFirmware_t select)
{
	if (select == VB_SELECT_FIRMWARE_READONLY)
		ec_ro_protected = 1;
	else
		ec_rw_protected = 1;
	return protect_retval;
}

VbError_t VbExEcDisableJump(int devidx)
{
	return run_retval;
}

VbError_t VbExEcJumpToRW(int devidx)
{
	ec_run_image = 1;
	mock_in_rw = 1;
	return run_retval;
}

VbError_t VbExEcHashImage(int devidx, enum VbSelectFirmware_t select,
			  const uint8_t **hash, int *hash_size)
{
	*hash = select == VB_SELECT_FIRMWARE_READONLY ?
		mock_ec_ro_hash : mock_ec_rw_hash;
	*hash_size = select == VB_SELECT_FIRMWARE_READONLY ?
		     mock_ec_ro_hash_size : mock_ec_rw_hash_size;
	return *hash_size ? VBERROR_SUCCESS : VBERROR_SIMULATED;
}

VbError_t VbExEcGetExpectedImage(int devidx, enum VbSelectFirmware_t select,
				 const uint8_t **image, int *image_size)
{
	static uint8_t fake_image[64] = {5, 6, 7, 8};
	*image = fake_image;
	*image_size = sizeof(fake_image);
	return get_expected_retval;
}

VbError_t VbExEcGetExpectedImageHash(int devidx, enum VbSelectFirmware_t select,
				     const uint8_t **hash, int *hash_size)
{
	*hash = want_ec_hash;
	*hash_size = want_ec_hash_size;

	return want_ec_hash_size ? VBERROR_SUCCESS : VBERROR_SIMULATED;
}

VbError_t VbExEcUpdateImage(int devidx, enum VbSelectFirmware_t select,
			    const uint8_t *image, int image_size)
{
	if (select == VB_SELECT_FIRMWARE_READONLY) {
		ec_ro_updated = 1;
		mock_ec_ro_hash[0] = update_hash;
	 } else {
		ec_rw_updated = 1;
		mock_ec_rw_hash[0] = update_hash;
	}
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

	TEST_EQ(VbEcSoftwareSync(0, &cparams, &mock_vnc), retval, desc);
	VbNvGet(&mock_vnc, VBNV_RECOVERY_REQUEST, &u);
	TEST_EQ(u, recovery_reason, "  recovery reason");
}

/* Tests */

static void VbSoftwareSyncTest(void)
{
	/* Recovery cases */
	ResetMocks();
	shared->recovery_reason = 123;
	test_ssync(0, 0, "In recovery, EC-RO");
	TEST_EQ(ec_rw_protected, 0, "  ec rw protected");

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
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
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
	mock_ec_rw_hash_size = 0;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_HASH_FAILED, "Bad EC hash");

	ResetMocks();
	mock_ec_rw_hash_size = 16;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_HASH_SIZE, "Bad EC hash size");

	ResetMocks();
	want_ec_hash_size = 0;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_EXPECTED_HASH, "Bad precalculated hash");

	ResetMocks();
	want_ec_hash_size = 16;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_HASH_SIZE,
		   "Hash size mismatch");

	ResetMocks();
	want_ec_hash_size = 4;
	mock_ec_rw_hash_size = 4;
	test_ssync(0, 0, "Custom hash size");

	/* Updates required */
	ResetMocks();
	mock_in_rw = 1;
	mock_ec_rw_hash[0]++;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   0, "Pending update needs reboot");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	VbNvSet(&mock_vnc, VBNV_TRY_RO_SYNC, 1);
	test_ssync(0, 0, "Update rw without reboot");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 0, "  ec ro updated");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	mock_ec_ro_hash[0]++;
	VbNvSet(&mock_vnc, VBNV_TRY_RO_SYNC, 1);
	test_ssync(0, 0, "Update rw and ro images without reboot");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 1, "  ec ro updated");

	ResetMocks();
	shared->flags |= VBSD_BOOT_FIRMWARE_WP_ENABLED;
	VbNvSet(&mock_vnc, VBNV_TRY_RO_SYNC, 1);
	mock_ec_rw_hash[0]++;
	mock_ec_ro_hash[0]++;
	test_ssync(0, 0, "WP enabled");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 0, "  ec ro updated");

	ResetMocks();
	VbNvSet(&mock_vnc, VBNV_TRY_RO_SYNC, 1);
	mock_ec_ro_hash[0]++;
	test_ssync(0, 0, "rw update not needed");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 0, "  ec rw not updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 1, "  ec ro updated");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	mock_ec_ro_hash[0]++;
	test_ssync(0, 0, "ro update not requested");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 0, "  ec ro updated");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	update_hash++;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_UPDATE, "updated hash mismatch");
	TEST_EQ(ec_rw_protected, 0, "  ec rw protected");
	TEST_EQ(ec_run_image, 0, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 0, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 0, "  ec ro updated");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	update_retval = VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   0, "Reboot after rw update");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_updated, 0, "  ec rw updated");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	update_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VBNV_RECOVERY_EC_UPDATE, "Update failed");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	shared->flags |= VBSD_EC_SLOW_UPDATE;
	test_ssync(0, 0, "Slow update");
	TEST_EQ(screens_displayed[0], VB_SCREEN_WAIT, "  wait screen");

	/* RW cases, no update */
	ResetMocks();
	mock_in_rw = 1;
	test_ssync(0, 0, "AP-RW, EC-RW");

	ResetMocks();
	test_ssync(0, 0, "AP-RW, EC-RO -> EC-RW");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 0, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 0, "  ec ro updated");

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

	return gTestSuccess ? 0 : 255;
}
