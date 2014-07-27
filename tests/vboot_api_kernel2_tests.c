/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_kernel, part 2
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
static LoadKernelParams lkp;

static int shutdown_request_calls_left;
static int audio_looping_calls_left;
static uint32_t vbtlk_retval;
static int vbexlegacy_called;
static int trust_ec;
static int virtdev_set;
static uint32_t virtdev_retval;
static uint32_t mock_keypress[8];
static uint32_t mock_keyflags[8];
static uint32_t mock_keypress_count;
static uint32_t mock_switches[8];
static uint32_t mock_switches_count;
static int mock_switches_are_stuck;
static uint32_t screens_displayed[8];
static uint32_t screens_count = 0;
static uint32_t mock_num_disks[8];
static uint32_t mock_num_disks_count;

extern enum VbEcBootMode_t VbGetMode(void);

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	Memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = &gbb;
	cparams.gbb = &gbb;

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

	Memset(&lkp, 0, sizeof(lkp));

	shutdown_request_calls_left = -1;
	audio_looping_calls_left = 30;
	vbtlk_retval = 1000;
	vbexlegacy_called = 0;
	trust_ec = 0;
	virtdev_set = 0;
	virtdev_retval = 0;

	Memset(screens_displayed, 0, sizeof(screens_displayed));
	screens_count = 0;

	Memset(mock_keypress, 0, sizeof(mock_keypress));
	Memset(mock_keyflags, 0, sizeof(mock_keyflags));
	mock_keypress_count = 0;

	Memset(mock_switches, 0, sizeof(mock_switches));
	mock_switches_count = 0;
	mock_switches_are_stuck = 0;

	Memset(mock_num_disks, 0, sizeof(mock_num_disks));
	mock_num_disks_count = 0;
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

uint32_t VbExKeyboardRead(void)
{
	return VbExKeyboardReadWithFlags(NULL);
}

uint32_t VbExKeyboardReadWithFlags(uint32_t *key_flags)
{
	if (mock_keypress_count < ARRAY_SIZE(mock_keypress)) {
		if (key_flags != NULL)
			*key_flags = mock_keyflags[mock_keypress_count];
		return mock_keypress[mock_keypress_count++];
	} else
		return 0;
}

uint32_t VbExGetSwitches(uint32_t request_mask)
{
	if (mock_switches_are_stuck)
		return mock_switches[0] & request_mask;
	if (mock_switches_count < ARRAY_SIZE(mock_switches))
		return mock_switches[mock_switches_count++] & request_mask;
	else
		return 0;
}

int VbExLegacy(void)
{
	vbexlegacy_called++;
	return 0;
}

VbError_t VbExDiskGetInfo(VbDiskInfo **infos_ptr, uint32_t *count,
                          uint32_t disk_flags)
{
	if (mock_num_disks_count < ARRAY_SIZE(mock_num_disks)) {
		if (mock_num_disks[mock_num_disks_count] == -1)
			return VBERROR_SIMULATED;
		else
			*count = mock_num_disks[mock_num_disks_count++];
	} else {
		*count = 0;
	}
	return VBERROR_SUCCESS;
}

VbError_t VbExDiskFreeInfo(VbDiskInfo *infos,
                           VbExDiskHandle_t preserve_handle)
{
	return VBERROR_SUCCESS;
}

int VbExTrustEC(int devidx)
{
	return trust_ec;
}

int VbAudioLooping(VbAudioContext *audio)
{
	if (audio_looping_calls_left == 0)
		return 0;
	else if (audio_looping_calls_left > 0)
		audio_looping_calls_left--;

	return 1;
}

uint32_t VbTryLoadKernel(VbCommonParams *cparams, LoadKernelParams *p,
                         uint32_t get_info_flags)
{
	return vbtlk_retval + get_info_flags;
}

VbError_t VbDisplayScreen(VbCommonParams *cparams, uint32_t screen, int force,
                          VbNvContext *vncptr)
{
	if (screens_count < ARRAY_SIZE(screens_displayed))
		screens_displayed[screens_count++] = screen;

	return VBERROR_SUCCESS;
}

uint32_t SetVirtualDevMode(int val)
{
	virtdev_set = val;
	return virtdev_retval;
}

/* Tests */

static void VbUserConfirmsTest(void)
{
	printf("Testing VbUserConfirms()...\n");

	ResetMocks();
	shutdown_request_calls_left = 1;
	TEST_EQ(VbUserConfirms(&cparams, 0), -1, "Shutdown requested");

	ResetMocks();
	mock_keypress[0] = '\r';
	TEST_EQ(VbUserConfirms(&cparams, 0), 1, "Enter");

	ResetMocks();
	mock_keypress[0] = 0x1b;
	TEST_EQ(VbUserConfirms(&cparams, 0), 0, "Esc");

	ResetMocks();
	mock_keypress[0] = ' ';
	shutdown_request_calls_left = 1;
	TEST_EQ(VbUserConfirms(&cparams, VB_CONFIRM_SPACE_MEANS_NO), 0,
                "Space means no");

	ResetMocks();
	mock_keypress[0] = ' ';
	shutdown_request_calls_left = 1;
	TEST_EQ(VbUserConfirms(&cparams, 0), -1, "Space ignored");

	ResetMocks();
	mock_keypress[0] = '\r';
	mock_keyflags[0] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	TEST_EQ(VbUserConfirms(&cparams, VB_CONFIRM_MUST_TRUST_KEYBOARD),
		1, "Enter with trusted keyboard");

	ResetMocks();
	mock_keypress[0] = '\r';	/* untrusted */
	mock_keypress[1] = ' ';
	TEST_EQ(VbUserConfirms(&cparams,
			       VB_CONFIRM_SPACE_MEANS_NO |
			       VB_CONFIRM_MUST_TRUST_KEYBOARD),
		0, "Untrusted keyboard");

	ResetMocks();
	mock_switches[0] = VB_INIT_FLAG_REC_BUTTON_PRESSED;
	TEST_EQ(VbUserConfirms(&cparams,
			       VB_CONFIRM_SPACE_MEANS_NO |
			       VB_CONFIRM_MUST_TRUST_KEYBOARD),
		1, "Recovery button");

	ResetMocks();
	mock_keypress[0] = '\r';
	mock_keypress[1] = 'y';
	mock_keypress[2] = 'z';
	mock_keypress[3] = ' ';
	mock_switches[0] = VB_INIT_FLAG_REC_BUTTON_PRESSED;
	mock_switches_are_stuck = 1;
	TEST_EQ(VbUserConfirms(&cparams,
			       VB_CONFIRM_SPACE_MEANS_NO |
			       VB_CONFIRM_MUST_TRUST_KEYBOARD),
		0, "Recovery button stuck");

	printf("...done.\n");
}

static void VbBootTest(void)
{
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_NORMAL);
	TEST_EQ(VbBootNormal(&cparams, &lkp), 1002, "VbBootNormal()");
	TEST_EQ(VbGetMode(), VB_EC_NORMAL, "vboot_mode normal");
}

static void VbBootDevTest(void)
{
	uint32_t u;

	printf("Testing VbBootDeveloper()...\n");

	/* Proceed after timeout */
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_DEVELOPER);
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Timeout");
	TEST_EQ(VbGetMode(), VB_EC_DEVELOPER, "vboot_mode developer");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");
	VbNvGet(VbApiKernelGetVnc(), VBNV_RECOVERY_REQUEST, &u);
	TEST_EQ(u, 0, "  recovery reason");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");

	/* Proceed to legacy after timeout if GBB flag set */
	ResetMocks();
	gbb.flags |= GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Timeout");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	/* Up arrow is uninteresting / passed to VbCheckDisplayKey() */
	ResetMocks();
	mock_keypress[0] = VB_KEY_UP;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Up arrow");

	/* Shutdown requested in loop */
	ResetMocks();
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");

	/* Space goes straight to recovery if no virtual dev switch */
	ResetMocks();
	mock_keypress[0] = ' ';
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), VBERROR_LOAD_KERNEL_RECOVERY,
		"Space = recovery");
	VbNvGet(VbApiKernelGetVnc(), VBNV_RECOVERY_REQUEST, &u);
	TEST_EQ(u, VBNV_RECOVERY_RW_DEV_SCREEN, "  recovery reason");

	/* Space asks to disable virtual dev switch */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), VBERROR_TPM_REBOOT_REQUIRED,
		"Space = tonorm");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_TO_NORM_CONFIRMED,
		"  confirm screen");
	VbNvGet(VbApiKernelGetVnc(), VBNV_DISABLE_DEV_REQUEST, &u);
	TEST_EQ(u, 1, "  disable dev request");

	/* Space-space doesn't disable it */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	mock_keypress[1] = ' ';
	mock_keypress[2] = 0x1b;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Space-space");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");

	/* Enter doesn't by default */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = '\r';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Enter ignored");

	/* Enter does if GBB flag set */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	gbb.flags |= GBB_FLAG_ENTER_TRIGGERS_TONORM;
	mock_keypress[0] = '\r';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), VBERROR_TPM_REBOOT_REQUIRED,
		"Enter = tonorm");

	/* Tonorm ignored if GBB forces dev switch on */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	gbb.flags |= GBB_FLAG_FORCE_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Can't tonorm gbb-dev");

	/* Shutdown requested at tonorm screen */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested at tonorm");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");

	/* Ctrl+D dismisses warning */
	ResetMocks();
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Ctrl+D");
	VbNvGet(VbApiKernelGetVnc(), VBNV_RECOVERY_REQUEST, &u);
	TEST_EQ(u, 0, "  recovery reason");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Ctrl+D doesn't boot legacy even if GBB flag is set */
	ResetMocks();
	mock_keypress[0] = 0x04;
	gbb.flags |= GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Ctrl+D");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Ctrl+L tries legacy boot mode only if enabled */
	ResetMocks();
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Ctrl+L normal");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	ResetMocks();

	gbb.flags |= GBB_FLAG_FORCE_DEV_BOOT_LEGACY;
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Ctrl+L force legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	ResetMocks();
	VbNvSet(VbApiKernelGetVnc(), VBNV_DEV_BOOT_LEGACY, 1);
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Ctrl+L nv legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	/* Ctrl+U boots USB only if enabled */
	ResetMocks();
	mock_keypress[0] = 0x15;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Ctrl+U normal");

	/* Ctrl+U enabled, with good USB boot */
	ResetMocks();
	VbNvSet(VbApiKernelGetVnc(), VBNV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 0, "Ctrl+U USB");

	/* Ctrl+U enabled via GBB */
	ResetMocks();
	gbb.flags |= GBB_FLAG_FORCE_DEV_BOOT_USB;
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 0, "Ctrl+U force USB");

	/* If no USB, eventually times out and tries fixed disk */
	ResetMocks();
	VbNvSet(VbApiKernelGetVnc(), VBNV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 0x15;
	TEST_EQ(VbBootDeveloper(&cparams, &lkp), 1002, "Ctrl+U enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	VbNvGet(VbApiKernelGetVnc(), VBNV_RECOVERY_REQUEST, &u);
	TEST_EQ(u, 0, "  recovery reason");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");

	printf("...done.\n");
}

static void VbBootRecTest(void)
{
	uint32_t u;

	printf("Testing VbBootRecovery()...\n");

	/* Shutdown requested in loop */
	ResetMocks();
	shutdown_request_calls_left = 10;
	VbExEcEnteringMode(0, VB_EC_RECOVERY);
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested");
	TEST_EQ(VbGetMode(), VB_EC_RECOVERY, "vboot_mode recovery");

	VbNvGet(VbApiKernelGetVnc(), VBNV_RECOVERY_REQUEST, &u);
	TEST_EQ(u, 0, "  recovery reason");
	TEST_EQ(screens_displayed[0], VB_SCREEN_BLANK,
		"  blank screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_NO_GOOD,
		"  no good screen");

	/* Disk inserted after start */
	ResetMocks();
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), 0, "Good");

	/* No disk inserted */
	ResetMocks();
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	shutdown_request_calls_left = 10;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Bad disk");
	TEST_EQ(screens_displayed[0], VB_SCREEN_BLANK,
		"  blank screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");

	/* Remove disks */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 1;
	mock_num_disks[1] = 1;
	mock_num_disks[2] = 1;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Remove");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_REMOVE,
		"  remove screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_REMOVE,
		"  remove screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_BLANK,
		"  blank screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");

	/* No removal if dev switch is on */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 1;
	mock_num_disks[1] = 1;
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"No remove in dev");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");

	/* No removal if recovery button physically pressed */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 1;
	mock_num_disks[1] = 1;
	shared->flags |= VBSD_BOOT_REC_SWITCH_ON;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"No remove in rec");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");

	/* Removal if no disk initially found, but found on second attempt */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 0;
	mock_num_disks[1] = 1;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Remove");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_REMOVE,
		"  remove screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK,
		"  blank screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");

	/* Bad disk count doesn't require removal */
	ResetMocks();
	mock_num_disks[0] = -1;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	shutdown_request_calls_left = 10;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Bad disk count");
	TEST_EQ(screens_displayed[0], VB_SCREEN_BLANK,
		"  blank screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");

	/* Ctrl+D ignored for many reasons... */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	trust_ec = 0;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if EC not trusted");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON |
		VBSD_BOOT_DEV_SWITCH_ON;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if already in dev mode");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if recovery not manually triggered");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_BOOT_REC_SWITCH_ON;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if no virtual dev switch");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		 "  todev screen");

	/* Ctrl+D ignored because the physical recovery switch is still pressed
	 * and we don't like that.
	 */
	ResetMocks();
	shared->flags = VBSD_BOOT_REC_SWITCH_ON;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	mock_switches[0] = VB_INIT_FLAG_REC_BUTTON_PRESSED;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if phys rec button is still pressed");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		 "  todev screen");

	/* Ctrl+D then space means don't enable */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	trust_ec = 1;
	mock_keypress[0] = 0x04;
	mock_keypress[1] = ' ';
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D todev abort");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		"  todev screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");

	/* Ctrl+D then enter means enable */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	trust_ec = 1;
	mock_keypress[0] = 0x04;
	mock_keypress[1] = '\r';
	mock_keyflags[1] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_TPM_REBOOT_REQUIRED,
		"Ctrl+D todev confirm");
	TEST_EQ(virtdev_set, 1, "  virtual dev mode on");

	/* Handle TPM error in enabling dev mode */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	trust_ec = 1;
	mock_keypress[0] = 0x04;
	mock_keypress[1] = '\r';
	mock_keyflags[1] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	virtdev_retval = VBERROR_SIMULATED;
	TEST_EQ(VbBootRecovery(&cparams, &lkp), VBERROR_TPM_SET_BOOT_MODE_STATE,
		"Ctrl+D todev failure");

	printf("...done.\n");
}


int main(void)
{
	VbUserConfirmsTest();
	VbBootTest();
	VbBootDevTest();
	VbBootRecTest();

	if (vboot_api_stub_check_memory())
		return 255;

	return gTestSuccess ? 0 : 255;
}
