/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_init
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "gbb_header.h"
#include "host_common.h"
#include "rollback_index.h"
#include "test_common.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/* Mock data */
static VbCommonParams cparams;
static VbInitParams iparams;
static VbNvContext vnc;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static uint64_t mock_timer;
static int rollback_s3_retval;
static int nv_write_called;
static GoogleBinaryBlockHeader gbb;
static int mock_virt_dev_sw;
static uint32_t mock_tpm_version;
static uint32_t mock_rfs_retval;
static int rfs_clear_tpm_request;
static int rfs_disable_dev_request;
static uint8_t backup_space[BACKUP_NV_SIZE];
static int backup_write_called;
static int backup_read_called;

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	Memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = &gbb;
	cparams.gbb_size = sizeof(gbb);

	Memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

	Memset(&iparams, 0, sizeof(iparams));

	Memset(&vnc, 0, sizeof(vnc));
	VbNvSetup(&vnc);
	VbNvTeardown(&vnc);                   /* So CRC gets generated */

	Memset(backup_space, 0, sizeof(backup_space));
	backup_write_called = 0;
	backup_read_called = 0;

	Memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	mock_timer = 10;
	rollback_s3_retval = TPM_SUCCESS;
	nv_write_called = 0;

	mock_virt_dev_sw = 0;
	mock_tpm_version = 0x10001;
	mock_rfs_retval = 0;

	rfs_clear_tpm_request = 0;
	rfs_disable_dev_request = 0;
}

/****************************************************************************/
/* Mocked verification functions */

VbError_t VbExNvStorageRead(uint8_t *buf)
{
	Memcpy(buf, vnc.raw, sizeof(vnc.raw));
	return VBERROR_SUCCESS;
}

VbError_t VbExNvStorageWrite(const uint8_t *buf)
{
	nv_write_called++;
	Memcpy(vnc.raw, buf, sizeof(vnc.raw));
	return VBERROR_SUCCESS;
}

uint32_t RollbackBackupRead(uint8_t *raw)
{
	backup_read_called++;
	Memcpy(raw, backup_space, sizeof(backup_space));
	return TPM_SUCCESS;
}

uint32_t RollbackBackupWrite(uint8_t *raw)
{
	backup_write_called++;
	Memcpy(backup_space, raw, sizeof(backup_space));
	return TPM_SUCCESS;
}

uint64_t VbExGetTimer(void)
{
	/*
	 * Exponential-ish rather than linear time, so that subtracting any
	 * two mock values will yield a unique result.
	 */
	uint64_t new_timer = mock_timer * 2 + 1;
	VbAssert(new_timer > mock_timer);  /* Make sure we don't overflow */
	mock_timer = new_timer;
	return mock_timer;
}

uint32_t RollbackS3Resume(void)
{
	return rollback_s3_retval;
}

uint32_t RollbackFirmwareSetup(int is_hw_dev,
                               int disable_dev_request,
                               int clear_tpm_owner_request,
                               /* two outputs on success */
                               int *is_virt_dev, uint32_t *version)
{
	rfs_clear_tpm_request = clear_tpm_owner_request;
	rfs_disable_dev_request = disable_dev_request;

	*is_virt_dev = mock_virt_dev_sw;
	*version = mock_tpm_version;
	return mock_rfs_retval;
}

/****************************************************************************/
/* Test VbInit() and check expected return value and recovery reason */

static void TestVbInit(VbError_t expected_retval,
                       uint8_t expected_recovery, const char *desc)
{
	uint32_t rr = 256;

	TEST_EQ(VbInit(&cparams, &iparams), expected_retval, desc);
	VbNvGet(&vnc, VBNV_RECOVERY_REQUEST, &rr);
	TEST_EQ(rr, expected_recovery, "  (recovery request)");
}

/****************************************************************************/

static void VbInitTest(void)
{
	uint32_t u;

	/* Test passing in too small a shared data area */
	ResetMocks();
	cparams.shared_data_size = VB_SHARED_DATA_MIN_SIZE - 1;
	TestVbInit(VBERROR_INIT_SHARED_DATA, 0, "Shared data too small");

	/* Normal call; dev=0 rec=0 */
	ResetMocks();
	TestVbInit(0, 0, "Normal call");
	TEST_EQ(shared->timer_vb_init_enter, 21, "  time enter");
	TEST_EQ(shared->timer_vb_init_exit, 43, "  time exit");
	TEST_EQ(shared->flags, 0, "  shared flags");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(nv_write_called, 0,
		"  NV write not called since nothing changed");

	/* If NV data is trashed, we initialize it */
	ResetMocks();
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
	/*
	 * Note that we're not doing a VbNvTeardown(), so the CRC hasn't been
	 * regenerated yet.  So VbInit() should ignore the corrupted recovery
	 * value and boot normally.
	 */
	TestVbInit(0, 0, "NV data trashed");
	TEST_EQ(nv_write_called, 1, "  NV write called");

	/*
	 * Test boot switch flags which are just passed through to shared
	 * flags, and don't have an effect on VbInit().
	 */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_WP_ENABLED;
	TestVbInit(0, 0, "Flags test WP");
	TEST_EQ(shared->flags, VBSD_BOOT_FIRMWARE_WP_ENABLED,
		"  shared flags");

	ResetMocks();
	iparams.flags = VB_INIT_FLAG_SW_WP_ENABLED;
	TestVbInit(0, 0, "Flags test SW WP");
	TEST_EQ(shared->flags, VBSD_BOOT_FIRMWARE_SW_WP_ENABLED,
		"  shared flags");

	ResetMocks();
	iparams.flags = VB_INIT_FLAG_RO_NORMAL_SUPPORT;
	TestVbInit(0, 0, "  flags test RO normal");
	TEST_EQ(shared->flags, VBSD_BOOT_RO_NORMAL_SUPPORT,
		"  shared flags");

	ResetMocks();
	iparams.flags = VB_INIT_FLAG_EC_SOFTWARE_SYNC;
	TestVbInit(0, 0, "  flags test EC software sync");
	TEST_EQ(shared->flags, VBSD_EC_SOFTWARE_SYNC, "  shared flags");

	ResetMocks();
	iparams.flags = VB_INIT_FLAG_EC_SLOW_UPDATE;
	TestVbInit(0, 0, "  flags test EC slow update");
	TEST_EQ(shared->flags, VBSD_EC_SLOW_UPDATE, "  shared flags");

	/* S3 resume */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_S3_RESUME;
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
	VbNvTeardown(&vnc);
	/* S3 resume doesn't clear the recovery request (or act on it) */
	TestVbInit(0, 123, "S3 resume");
	TEST_EQ(shared->flags, VBSD_BOOT_S3_RESUME, "  shared flags S3");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(shared->recovery_reason, 0,
		"  S3 doesn't look at recovery request");

	/* S3 resume with TPM resume error */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_S3_RESUME;
	rollback_s3_retval = 1;
	/* S3 resume doesn't clear the recovery request (or act on it) */
	TestVbInit(VBERROR_TPM_S3_RESUME, 0, "S3 resume rollback error");

	/*
	 * Normal boot doesn't care about TPM resume error because it doesn't
	 * call RollbackS3Resume().
	 */
	ResetMocks();
	rollback_s3_retval = 1;
	TestVbInit(0, 0, "Normal doesn't S3 resume");

	/* S3 resume with debug reset */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_S3_RESUME;
	VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 1);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "S3 debug reset");
	TEST_EQ(iparams.out_flags, VB_INIT_OUT_S3_DEBUG_BOOT, "  out flags");
	VbNvGet(&vnc, VBNV_DEBUG_RESET_MODE, &u);
	TEST_EQ(u, 0, "  S3 clears nv debug reset mode");

	/* Normal boot clears S3 debug reset mode; doesn't set output flag */
	ResetMocks();
	VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 1);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Normal with debug reset mode");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	VbNvGet(&vnc, VBNV_DEBUG_RESET_MODE, &u);
	TEST_EQ(u, 0, "  normal clears nv debug reset mode");

	/*
	 * S3 resume with debug reset is a normal boot, so doesn't resume the
	 * TPM.
	 */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_S3_RESUME;
	rollback_s3_retval = 1;
	VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 1);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "S3 debug reset rollback error");

	/* Developer mode */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON;
	TestVbInit(0, 0, "Dev mode on");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER |
		VB_INIT_OUT_ENABLE_ALTERNATE_OS, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");

	/* Developer mode forced by GBB flag */
	ResetMocks();
	iparams.flags = 0;
	gbb.flags = GBB_FLAG_FORCE_DEV_SWITCH_ON;
	TestVbInit(0, 0, "Dev mode via GBB");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER |
		VB_INIT_OUT_ENABLE_ALTERNATE_OS, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");

	/* Developer mode when option ROM matters and isn't loaded */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON |
		VB_INIT_FLAG_OPROM_MATTERS;
	TestVbInit(VBERROR_VGA_OPROM_MISMATCH, 0, "Dev mode need oprom");
	VbNvGet(&vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 1, "  oprom requested");

	/* Developer mode when option ROM matters and is already loaded */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON |
		VB_INIT_FLAG_OPROM_MATTERS | VB_INIT_FLAG_OPROM_LOADED;
	TestVbInit(0, 0, "Dev mode has oprom");

	/* Normal mode when option ROM matters and is loaded */
	ResetMocks();
	VbNvSet(&vnc, VBNV_OPROM_NEEDED, 1);
	VbNvTeardown(&vnc);
	iparams.flags = VB_INIT_FLAG_OPROM_MATTERS | VB_INIT_FLAG_OPROM_LOADED;
	TestVbInit(VBERROR_VGA_OPROM_MISMATCH, 0, "Normal mode with oprom");
	VbNvGet(&vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 0, "  oprom not requested");

	/* Option ROMs can be forced by GBB flag */
	ResetMocks();
	gbb.flags = GBB_FLAG_LOAD_OPTION_ROMS;
	TestVbInit(0, 0, "GBB load option ROMs");
	TEST_EQ(iparams.out_flags, VB_INIT_OUT_ENABLE_OPROM, "  out flags");

	/* If requiring signed only, don't enable alternate OS by default */
	ResetMocks();
	VbNvSet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, 1);
	VbNvTeardown(&vnc);
	iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON;
	TestVbInit(0, 0, "Dev signed only");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER, "  out flags");

	/* But that can be overridden by the GBB */
	ResetMocks();
	VbNvSet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, 1);
	VbNvTeardown(&vnc);
	iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON;
	gbb.flags = GBB_FLAG_ENABLE_ALTERNATE_OS;
	TestVbInit(0, 0, "Force option ROMs via GBB");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER |
		VB_INIT_OUT_ENABLE_ALTERNATE_OS, "  out flags");

	/* The GBB override is ignored in normal mode */
	ResetMocks();
	gbb.flags = GBB_FLAG_ENABLE_ALTERNATE_OS;
	TestVbInit(0, 0, "Normal mode ignores forcing option ROMs via GBB");
	TEST_EQ(iparams.out_flags, 0, "  out flags");

	/* Recovery mode from NV storage */
	ResetMocks();
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Recovery mode - from nv");
	TEST_EQ(shared->recovery_reason, 123, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_ENABLE_RECOVERY |
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
	TEST_EQ(shared->flags, 0, "  shared flags");

	/* Recovery mode from recovery button */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_REC_BUTTON_PRESSED;
	TestVbInit(0, 0, "Recovery mode - button");
	TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_MANUAL,
		"  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_ENABLE_RECOVERY |
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_REC_SWITCH_ON, "  shared flags");

	/* Recovery button reason supersedes NV reason */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_REC_BUTTON_PRESSED;
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Recovery mode - button AND nv");
	TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_MANUAL,
		"  recovery reason");

	/* Recovery mode from previous boot fail */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_PREVIOUS_BOOT_FAIL;
	TestVbInit(0, 0, "Recovery mode - previous boot fail");
	TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_FIRMWARE,
		"  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_ENABLE_RECOVERY |
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
	TEST_EQ(shared->flags, 0, "  shared flags");

	/* Recovery mode from NV supersedes previous boot fail */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_PREVIOUS_BOOT_FAIL;
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, 123);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Recovery mode - previous boot fail AND nv");
	TEST_EQ(shared->recovery_reason, 123, "  recovery reason");

	/* Dev + recovery = recovery */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_REC_BUTTON_PRESSED |
		VB_INIT_FLAG_DEV_SWITCH_ON;
	TestVbInit(0, 0, "Recovery mode - button");
	TEST_EQ(shared->recovery_reason, VBNV_RECOVERY_RO_MANUAL,
		"  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_ENABLE_RECOVERY |
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE, "  out flags");
	TEST_EQ(shared->flags,
		VBSD_BOOT_REC_SWITCH_ON | VBSD_BOOT_DEV_SWITCH_ON,
		"  shared flags");
}

static void VbInitTestTPM(void)
{
	uint32_t u;

	/* Rollback setup needs to reboot */
	ResetMocks();
	mock_rfs_retval = TPM_E_MUST_REBOOT;
	TestVbInit(VBERROR_TPM_REBOOT_REQUIRED, 0,
		   "Rollback TPM reboot (rec=0)");
	ResetMocks();
	mock_rfs_retval = TPM_E_MUST_REBOOT;
	iparams.flags = VB_INIT_FLAG_REC_BUTTON_PRESSED;
	TestVbInit(VBERROR_TPM_REBOOT_REQUIRED, VBNV_RECOVERY_RO_TPM_REBOOT,
		   "Rollback TPM reboot, in recovery, first time");
	/* Ignore if we already tried rebooting */
	ResetMocks();
	mock_rfs_retval = TPM_E_MUST_REBOOT;
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, VBNV_RECOVERY_RO_TPM_REBOOT);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Rollback TPM reboot, in recovery, already retried");
	TEST_EQ(shared->fw_version_tpm, 0x10001, "  shared fw_version_tpm");

	/* Other rollback setup errors */
	ResetMocks();
	mock_rfs_retval = TPM_E_IOERROR;
	mock_tpm_version = 0x20002;
	TestVbInit(VBERROR_TPM_FIRMWARE_SETUP, VBNV_RECOVERY_RO_TPM_S_ERROR,
		   "Rollback TPM setup error - not in recovery");
	TEST_EQ(shared->fw_version_tpm, 0, "  shared fw_version_tpm not set");
	ResetMocks();
	mock_rfs_retval = TPM_E_IOERROR;
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, VBNV_RECOVERY_US_TEST);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Rollback TPM setup error ignored in recovery");
	TEST_EQ(shared->fw_version_tpm, 0x10001, "  shared fw_version_tpm");

	/* Virtual developer switch, but not enabled. */
	ResetMocks();
	VbNvSet(&vnc, VBNV_DISABLE_DEV_REQUEST, 1);
	VbNvTeardown(&vnc);
	iparams.flags = VB_INIT_FLAG_VIRTUAL_DEV_SWITCH;
	TestVbInit(0, 0, "TPM Dev mode off");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(shared->flags, VBSD_HONOR_VIRT_DEV_SWITCH, "  shared flags");
	VbNvGet(&vnc, VBNV_DISABLE_DEV_REQUEST, &u);
	TEST_EQ(u, 0, "  disable dev request");

	/* Virtual developer switch, enabled. */
	ResetMocks();
	VbNvSet(&vnc, VBNV_DISABLE_DEV_REQUEST, 1);
	VbNvTeardown(&vnc);
	iparams.flags = VB_INIT_FLAG_VIRTUAL_DEV_SWITCH;
	mock_virt_dev_sw = 1;
	TestVbInit(0, 0, "TPM Dev mode on");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER |
		VB_INIT_OUT_ENABLE_ALTERNATE_OS, "  out flags");
	TEST_EQ(shared->flags,
		VBSD_BOOT_DEV_SWITCH_ON | VBSD_HONOR_VIRT_DEV_SWITCH,
		"  shared flags");
	/* Disable-request doesn't get cleared because dev mode is still on */
	VbNvGet(&vnc, VBNV_DISABLE_DEV_REQUEST, &u);
	TEST_EQ(u, 1, "  disable dev request");
	/* Disable request was passed on to RollbackFirmwareSetup() */
	TEST_EQ(rfs_disable_dev_request, 1, "  rfs disable dev");

	/* Ignore virtual developer switch, even though enabled. */
	ResetMocks();
	mock_virt_dev_sw = 1;
	TestVbInit(0, 0, "TPM Dev mode on but ignored");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(shared->flags, 0, "  shared flags");

	/* HW dev switch on, no virtual developer switch */
	ResetMocks();
	iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON;
	TestVbInit(0, 0, "HW Dev mode on");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER |
		VB_INIT_OUT_ENABLE_ALTERNATE_OS, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");

	/* Check TPM owner clear request */
	ResetMocks();
	VbNvSet(&vnc, VBNV_CLEAR_TPM_OWNER_REQUEST, 1);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "TPM clear owner");
	VbNvGet(&vnc, VBNV_CLEAR_TPM_OWNER_REQUEST, &u);
	TEST_EQ(u, 0, "  tpm clear request");
	VbNvGet(&vnc, VBNV_CLEAR_TPM_OWNER_DONE, &u);
	TEST_EQ(u, 1, "  tpm clear request");
	TEST_EQ(rfs_clear_tpm_request, 1, "rfs tpm clear request");
}

static void VbInitTestBackup(void)
{
	VbNvContext tmp_vnc;
	uint32_t u, nv_w, bu_r;

	ResetMocks();
	/* Normal mode call */
	TestVbInit(0, 0, "normal mode, no backup");
	TEST_EQ(shared->flags, 0, "  shared flags");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(nv_write_called, 0,
		"  NV write not called since nothing changed");

	ResetMocks();
	/* Now set some params that should be backed up. */
	VbNvSet(&vnc, VBNV_KERNEL_FIELD, 0xaabbccdd);
	VbNvSet(&vnc, VBNV_LOCALIZATION_INDEX, 0xa5);
	VbNvSet(&vnc, VBNV_DEV_BOOT_USB, 1);
	VbNvSet(&vnc, VBNV_DEV_BOOT_LEGACY, 1);
	VbNvSet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, 1);
	/* and some that don't */
	VbNvSet(&vnc, VBNV_OPROM_NEEDED, 1);
	VbNvSet(&vnc, VBNV_TRY_B_COUNT, 3);
	/* Make sure they're clean */
	VbNvTeardown(&vnc);
	/* Normal mode call */
	TestVbInit(0, 0, "normal mode, some backup");
	TEST_EQ(shared->flags, 0, "  shared flags");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(nv_write_called, 1,
		"  Write NV because things have changed");
	/* Some fields should be unchanged */
	VbNvGet(&vnc, VBNV_KERNEL_FIELD, &u);
	TEST_EQ(u, 0xaabbccdd, "  NV kernel field");
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 0xa5, "  NV localization index");
	VbNvGet(&vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 1, "  NV oprom_needed");
	VbNvGet(&vnc, VBNV_TRY_B_COUNT, &u);
	TEST_EQ(u, 3, "  NV try_b_count");
	/* But normal mode should have cleared the DEV_BOOT flags */
	VbNvGet(&vnc, VBNV_DEV_BOOT_USB, &u);
	TEST_EQ(u, 0, "  NV dev_boot_usb");
	VbNvGet(&vnc, VBNV_DEV_BOOT_LEGACY, &u);
	TEST_EQ(u, 0, "  NV dev_boot_legacy");
	VbNvGet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, &u);
	TEST_EQ(u, 0, "  NV dev_boot_signed_only");
	/* So we should have written the backup */
	TEST_EQ(backup_write_called, 1, "  Backup written once");
	/* And the backup should reflect the persisent flags. */
	Memset(&tmp_vnc, 0, sizeof(tmp_vnc));
	TEST_EQ(0, RestoreNvFromBackup(&tmp_vnc), "read from backup");
	VbNvGet(&tmp_vnc, VBNV_KERNEL_FIELD, &u);
	TEST_EQ(u, 0xaabbccdd, "  BU kernel field");
	VbNvGet(&tmp_vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 0xa5, "  BU localization index");
	VbNvGet(&tmp_vnc, VBNV_DEV_BOOT_USB, &u);
	TEST_EQ(u, 0, "  BU dev_boot_usb");
	VbNvGet(&tmp_vnc, VBNV_DEV_BOOT_LEGACY, &u);
	TEST_EQ(u, 0, "  BU dev_boot_legacy");
	VbNvGet(&tmp_vnc, VBNV_DEV_BOOT_SIGNED_ONLY, &u);
	TEST_EQ(u, 0, "  BU dev_boot_signed_only");
	/* but not the others */
	VbNvGet(&tmp_vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 0, "  BU oprom_needed");
	VbNvGet(&tmp_vnc, VBNV_TRY_B_COUNT, &u);
	TEST_EQ(u, 0, "  BU try_b_count");

	/*
	 * If we change one of the non-backed-up NVRAM params and try
	 * again, we shouldn't need to backup again.
	 */
	VbNvSet(&vnc, VBNV_OPROM_NEEDED, 0);
	VbNvSet(&vnc, VBNV_TRY_B_COUNT, 2);
	/* Make sure they're clean */
	VbNvTeardown(&vnc);
	/* Normal mode call */
	TestVbInit(0, 0, "normal mode, expect no backup");
	TEST_EQ(shared->flags, 0, "  shared flags");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(backup_write_called, 1, "  Backup still only written once");

	/* Now switch to dev-mode. */
	iparams.flags = VB_INIT_FLAG_DEV_SWITCH_ON;
	TestVbInit(0, 0, "Dev mode on");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER |
		VB_INIT_OUT_ENABLE_ALTERNATE_OS, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");
	TEST_EQ(backup_write_called, 1, "  Still only one backup");

	/* Now change some params that should be backed up. */
	VbNvSet(&vnc, VBNV_KERNEL_FIELD, 0xdeadbeef);
	VbNvSet(&vnc, VBNV_LOCALIZATION_INDEX, 0x5a);
	VbNvSet(&vnc, VBNV_DEV_BOOT_USB, 1);
	VbNvSet(&vnc, VBNV_DEV_BOOT_LEGACY, 1);
	VbNvSet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, 1);
	/* and some that don't */
	VbNvSet(&vnc, VBNV_OPROM_NEEDED, 1);
	VbNvSet(&vnc, VBNV_TRY_B_COUNT, 4);
	/* Make sure they're clean */
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Dev mode on");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");
	TEST_EQ(backup_write_called, 1, "  Once more, one backup");

	/* But if we explictly request a backup, they'll get saved. */
	VbNvSet(&vnc, VBNV_BACKUP_NVRAM_REQUEST, 1);
	VbNvTeardown(&vnc);
	TestVbInit(0, 0, "Dev mode on");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");
	TEST_EQ(backup_write_called, 2, "  Two backups now");
	VbNvGet(&vnc, VBNV_BACKUP_NVRAM_REQUEST, &u);
	TEST_EQ(u, 0, "  backup_request cleared");
	/* Quick check that the non-backed-up stuff is still valid */
	VbNvGet(&vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 1, "  NV oprom_needed");
	VbNvGet(&vnc, VBNV_TRY_B_COUNT, &u);
	TEST_EQ(u, 4, "  NV try_b_count");
	/* But only the stuff we care about was backed up */
	Memset(&tmp_vnc, 0, sizeof(tmp_vnc));
	TEST_EQ(0, RestoreNvFromBackup(&tmp_vnc), "read from backup");
	VbNvGet(&tmp_vnc, VBNV_KERNEL_FIELD, &u);
	TEST_EQ(u, 0xdeadbeef, "  BU kernel field");
	VbNvGet(&tmp_vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 0x5a, "  BU localization index");
	VbNvGet(&tmp_vnc, VBNV_DEV_BOOT_USB, &u);
	TEST_EQ(u, 1, "  BU dev_boot_usb");
	VbNvGet(&tmp_vnc, VBNV_DEV_BOOT_LEGACY, &u);
	TEST_EQ(u, 1, "  BU dev_boot_legacy");
	VbNvGet(&tmp_vnc, VBNV_DEV_BOOT_SIGNED_ONLY, &u);
	TEST_EQ(u, 1, "  BU dev_boot_signed_only");
	/* but not the others */
	VbNvGet(&tmp_vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 0, "  BU oprom_needed");
	VbNvGet(&tmp_vnc, VBNV_TRY_B_COUNT, &u);
	TEST_EQ(u, 0, "  BU try_b_count");

	/* If we lose the NV storage, the backup bits will be restored */
	vnc.raw[0] = 0;
	bu_r = backup_read_called;
	nv_w = nv_write_called;
	TestVbInit(0, 0, "Dev mode on");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags,
		VB_INIT_OUT_CLEAR_RAM |
		VB_INIT_OUT_ENABLE_DISPLAY |
		VB_INIT_OUT_ENABLE_USB_STORAGE |
		VB_INIT_OUT_ENABLE_DEVELOPER, "  out flags");
	TEST_EQ(shared->flags, VBSD_BOOT_DEV_SWITCH_ON, "  shared flags");
	TEST_EQ(backup_write_called, 2, "  Still just two backups now");
	TEST_EQ(backup_read_called, bu_r + 1, "  One more backup read");
	TEST_EQ(nv_write_called, nv_w + 1, "  One more NV write");
	/* The non-backed-up stuff is reset to defaults */
	VbNvGet(&vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 0, "  NV oprom_needed");
	VbNvGet(&vnc, VBNV_TRY_B_COUNT, &u);
	TEST_EQ(u, 0, "  NV try_b_count");
	/* And the backed up stuff is restored */
	VbNvGet(&vnc, VBNV_KERNEL_FIELD, &u);
	TEST_EQ(u, 0xdeadbeef, "  BU kernel field");
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 0x5a, "  BU localization index");
	VbNvGet(&vnc, VBNV_DEV_BOOT_USB, &u);
	TEST_EQ(u, 1, "  BU dev_boot_usb");
	VbNvGet(&vnc, VBNV_DEV_BOOT_LEGACY, &u);
	TEST_EQ(u, 1, "  BU dev_boot_legacy");
	VbNvGet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, &u);
	TEST_EQ(u, 1, "  BU dev_boot_signed_only");

	/*
	 * But if we lose the NV storage and go back to normal mode at the same
	 * time, then the DEV_BOOT_* flags will be cleared.
	 */
	vnc.raw[0] = 0;
	bu_r = backup_read_called;
	nv_w = nv_write_called;
	iparams.flags = 0;
	TestVbInit(0, 0, "Back to normal mode");
	TEST_EQ(shared->recovery_reason, 0, "  recovery reason");
	TEST_EQ(iparams.out_flags, 0, "  out flags");
	TEST_EQ(shared->flags, 0, "  shared flags");
	/* We read twice: once to restore, once for read-prior-to-write */
	TEST_EQ(backup_read_called, bu_r + 2, "  Two more backup reads");
	TEST_EQ(backup_write_called, 3, "  Backup write due clearing DEV_*");
	TEST_EQ(nv_write_called, nv_w + 1, "  One more NV write");
	/* The non-backed-up stuff is reset to defaults */
	VbNvGet(&vnc, VBNV_OPROM_NEEDED, &u);
	TEST_EQ(u, 0, "  NV oprom_needed");
	VbNvGet(&vnc, VBNV_TRY_B_COUNT, &u);
	TEST_EQ(u, 0, "  NV try_b_count");
	/* And the backed up stuff is restored */
	VbNvGet(&vnc, VBNV_KERNEL_FIELD, &u);
	TEST_EQ(u, 0xdeadbeef, "  BU kernel field");
	VbNvGet(&vnc, VBNV_LOCALIZATION_INDEX, &u);
	TEST_EQ(u, 0x5a, "  BU localization index");
	/* But not the DEV_BOOT_* flags */
	VbNvGet(&vnc, VBNV_DEV_BOOT_USB, &u);
	TEST_EQ(u, 0, "  BU dev_boot_usb");
	VbNvGet(&vnc, VBNV_DEV_BOOT_LEGACY, &u);
	TEST_EQ(u, 0, "  BU dev_boot_legacy");
	VbNvGet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, &u);
	TEST_EQ(u, 0, "  BU dev_boot_signed_only");
}


int main(int argc, char *argv[])
{
	VbInitTest();
	VbInitTestTPM();
	VbInitTestBackup();

	return gTestSuccess ? 0 : 255;
}
