/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_kernel, part 3 - software sync
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "ec_sync.h"
#include "gbb_header.h"
#include "host_common.h"
#include "load_kernel_fw.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_audio.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"
#include "vboot_struct.h"

/* Mock data */
static VbCommonParams cparams;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static GoogleBinaryBlockHeader gbb;

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
static struct vb2_context ctx;
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
static struct vb2_shared_data *sd;

static uint32_t screens_displayed[8];
static uint32_t screens_count = 0;

static int ec_aux_fw_update_req;
static VbAuxFwUpdateSeverity_t ec_aux_fw_mock_severity;
static VbAuxFwUpdateSeverity_t ec_aux_fw_update_severity;
static int ec_aux_fw_protected;

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
	cparams.gbb = &gbb;

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);
	ctx.flags = VB2_CONTEXT_EC_SYNC_SUPPORTED;
	vb2_init_context(&ctx);
	vb2_nv_init(&ctx);

	sd = vb2_get_sd(&ctx);
	sd->vbsd = shared;

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

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

	ec_aux_fw_mock_severity = VB_AUX_FW_NO_UPDATE;
	ec_aux_fw_update_severity = VB_AUX_FW_NO_UPDATE;
	ec_aux_fw_update_req = 0;
	ec_aux_fw_protected = 0;
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
	return !mock_in_rw;
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

VbError_t VbDisplayScreen(struct vb2_context *ctx, uint32_t screen, int force)
{
	if (screens_count < ARRAY_SIZE(screens_displayed))
		screens_displayed[screens_count++] = screen;

	return VBERROR_SUCCESS;
}

VbError_t VbExCheckAuxFw(VbAuxFwUpdateSeverity_t *severity)
{
	*severity = ec_aux_fw_mock_severity;
	ec_aux_fw_update_severity = ec_aux_fw_mock_severity;
	return VBERROR_SUCCESS;
}

VbError_t VbExUpdateAuxFw()
{
	ec_aux_fw_update_req = ec_aux_fw_update_severity != VB_AUX_FW_NO_UPDATE;
	ec_aux_fw_protected = 1;
	return VBERROR_SUCCESS;
}

static void test_ssync(VbError_t retval, int recovery_reason, const char *desc)
{
	TEST_EQ(ec_sync_all(&ctx), retval, desc);
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		recovery_reason, "  recovery reason");
}

/* Tests */

static void VbSoftwareSyncTest(void)
{
	/* AP-RO cases */
	ResetMocks();
	in_rw_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VB2_RECOVERY_EC_UNKNOWN_IMAGE, "Unknown EC image");

	/* Calculate hashes */
	ResetMocks();
	mock_ec_rw_hash_size = 0;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VB2_RECOVERY_EC_HASH_FAILED, "Bad EC hash");

	ResetMocks();
	mock_ec_rw_hash_size = 16;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VB2_RECOVERY_EC_HASH_SIZE, "Bad EC hash size");

	ResetMocks();
	want_ec_hash_size = 0;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VB2_RECOVERY_EC_EXPECTED_HASH, "Bad precalculated hash");

	ResetMocks();
	want_ec_hash_size = 16;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   VB2_RECOVERY_EC_HASH_SIZE,
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
	vb2_nv_set(&ctx, VB2_NV_TRY_RO_SYNC, 1);
	test_ssync(0, 0, "Update rw without reboot");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 0, "  ec ro updated");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	mock_ec_ro_hash[0]++;
	vb2_nv_set(&ctx, VB2_NV_TRY_RO_SYNC, 1);
	test_ssync(0, 0, "Update rw and ro images without reboot");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 1, "  ec ro updated");

	ResetMocks();
	ctx.flags |= VB2_CONTEXT_SW_WP_ENABLED;
	vb2_nv_set(&ctx, VB2_NV_TRY_RO_SYNC, 1);
	mock_ec_rw_hash[0]++;
	mock_ec_ro_hash[0]++;
	test_ssync(0, 0, "WP enabled");
	TEST_EQ(ec_rw_protected, 1, "  ec rw protected");
	TEST_EQ(ec_run_image, 1, "  ec run image");
	TEST_EQ(ec_rw_updated, 1, "  ec rw updated");
	TEST_EQ(ec_ro_protected, 1, "  ec ro protected");
	TEST_EQ(ec_ro_updated, 0, "  ec ro updated");

	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_TRY_RO_SYNC, 1);
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
		   VB2_RECOVERY_EC_UPDATE, "updated hash mismatch");
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
		   VB2_RECOVERY_EC_UPDATE, "Update failed");

	ResetMocks();
	mock_ec_rw_hash[0]++;
	ctx.flags |= VB2_CONTEXT_EC_SYNC_SLOW;
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
		   VB2_RECOVERY_EC_JUMP_RW, "Jump to RW fail");

	ResetMocks();
	run_retval = VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	test_ssync(VBERROR_EC_REBOOT_TO_RO_REQUIRED,
		   0, "Jump to RW fail because locked");

	ResetMocks();
	protect_retval = VBERROR_SIMULATED;
	test_ssync(VBERROR_SIMULATED,
		   VB2_RECOVERY_EC_PROTECT, "Protect error");

	/* No longer check for shutdown requested */
	ResetMocks();
	shutdown_request_calls_left = 0;
	test_ssync(0, 0,
		   "AP-RW, EC-RO -> EC-RW shutdown requested");

	ResetMocks();
	mock_in_rw = 1;
	shutdown_request_calls_left = 0;
	test_ssync(0, 0, "AP-RW shutdown requested");

	ResetMocks();
	sd->gbb_flags |= VB2_GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC;
	ec_aux_fw_mock_severity = VB_AUX_FW_FAST_UPDATE;
	test_ssync(VBERROR_SUCCESS, 0,
		   "VB2_GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC"
		   " disables auxiliary FW update request");
	TEST_EQ(ec_aux_fw_update_req, 0, "  aux fw update disabled");
	TEST_EQ(ec_aux_fw_protected, 1, "  aux fw protected");

	ResetMocks();
	sd->gbb_flags |= VB2_GBB_FLAG_DISABLE_PD_SOFTWARE_SYNC;
	ec_aux_fw_mock_severity = VB_AUX_FW_FAST_UPDATE;
	test_ssync(VBERROR_SUCCESS, 0,
		   "VB2_GBB_FLAG_DISABLE_PD_SOFTWARE_SYNC"
		   " disables auxiliary FW update request");
	TEST_EQ(ec_aux_fw_update_req, 0, "  aux fw update disabled");
	TEST_EQ(ec_aux_fw_protected, 1, "  aux fw protected");

	ResetMocks();
	ec_aux_fw_mock_severity = VB_AUX_FW_NO_UPDATE;
	test_ssync(VBERROR_SUCCESS, 0,
		   "No auxiliary FW update needed");
	TEST_EQ(screens_count, 0,
		"  wait screen skipped");
	TEST_EQ(ec_aux_fw_update_req, 0, "  no aux fw update requested");
	TEST_EQ(ec_aux_fw_protected, 1, "  aux fw protected");

	ResetMocks();
	ec_aux_fw_mock_severity = VB_AUX_FW_FAST_UPDATE;
	test_ssync(VBERROR_SUCCESS, 0,
		   "Fast auxiliary FW update needed");
	TEST_EQ(screens_count, 0,
		"  wait screen skipped");
	TEST_EQ(ec_aux_fw_update_req, 1, "  aux fw update requested");
	TEST_EQ(ec_aux_fw_protected, 1, "  aux fw protected");

	ResetMocks();
	ec_aux_fw_mock_severity = VB_AUX_FW_SLOW_UPDATE;
	test_ssync(VBERROR_SUCCESS, 0,
		   "Slow auxiliary FW update needed");
	TEST_EQ(ec_aux_fw_update_req, 1, "  aux fw update requested");
	TEST_EQ(ec_aux_fw_protected, 1, "  aux fw protected");
	TEST_EQ(screens_displayed[0], VB_SCREEN_WAIT,
		"  wait screen forced");
}

int main(void)
{
	VbSoftwareSyncTest();

	return gTestSuccess ? 0 : 255;
}
