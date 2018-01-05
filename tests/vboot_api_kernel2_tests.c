/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for vboot_api_kernel, part 2
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2misc.h"
#include "2nvstorage.h"
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
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static LoadKernelParams lkp;
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
static struct vb2_context ctx;
static struct vb2_shared_data *sd;

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
extern struct RollbackSpaceFwmp *VbApiKernelGetFwmp(void);

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	memset(VbApiKernelGetFwmp(), 0, sizeof(struct RollbackSpaceFwmp));

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));

	memset(&lkp, 0, sizeof(lkp));

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);
	vb2_init_context(&ctx);
	vb2_nv_init(&ctx);

	sd = vb2_get_sd(&ctx);
	sd->vbsd = shared;

	shutdown_request_calls_left = -1;
	audio_looping_calls_left = 30;
	vbtlk_retval = 1000;
	vbexlegacy_called = 0;
	trust_ec = 0;
	virtdev_set = 0;
	virtdev_retval = 0;

	memset(screens_displayed, 0, sizeof(screens_displayed));
	screens_count = 0;

	memset(mock_keypress, 0, sizeof(mock_keypress));
	memset(mock_keyflags, 0, sizeof(mock_keyflags));
	mock_keypress_count = 0;

	memset(mock_switches, 0, sizeof(mock_switches));
	mock_switches_count = 0;
	mock_switches_are_stuck = 0;

	memset(mock_num_disks, 0, sizeof(mock_num_disks));
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

int vb2_audio_looping(void)
{
	if (audio_looping_calls_left == 0)
		return 0;
	else if (audio_looping_calls_left > 0)
		audio_looping_calls_left--;

	return 1;
}

uint32_t VbTryLoadKernel(struct vb2_context *ctx, uint32_t get_info_flags)
{
	return vbtlk_retval + get_info_flags;
}

VbError_t VbDisplayScreen(struct vb2_context *ctx, uint32_t screen, int force)
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
	TEST_EQ(VbUserConfirms(&ctx, 0), -1, "Shutdown requested");

	ResetMocks();
	mock_keypress[0] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbUserConfirms(&ctx, 0), -1, "Shutdown requested");

	ResetMocks();
	mock_keypress[0] = '\r';
	TEST_EQ(VbUserConfirms(&ctx, 0), 1, "Enter");

	ResetMocks();
	mock_keypress[0] = 0x1b;
	TEST_EQ(VbUserConfirms(&ctx, 0), 0, "Esc");

	ResetMocks();
	mock_keypress[0] = ' ';
	shutdown_request_calls_left = 1;
	TEST_EQ(VbUserConfirms(&ctx, VB_CONFIRM_SPACE_MEANS_NO), 0,
                "Space means no");

	ResetMocks();
	mock_keypress[0] = ' ';
	shutdown_request_calls_left = 1;
	TEST_EQ(VbUserConfirms(&ctx, 0), -1, "Space ignored");

	ResetMocks();
	mock_keypress[0] = '\r';
	mock_keyflags[0] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	TEST_EQ(VbUserConfirms(&ctx, VB_CONFIRM_MUST_TRUST_KEYBOARD),
		1, "Enter with trusted keyboard");

	ResetMocks();
	mock_keypress[0] = '\r';	/* untrusted */
	mock_keypress[1] = ' ';
	TEST_EQ(VbUserConfirms(&ctx,
			       VB_CONFIRM_SPACE_MEANS_NO |
			       VB_CONFIRM_MUST_TRUST_KEYBOARD),
		0, "Untrusted keyboard");

	ResetMocks();
	mock_switches[0] = VB_INIT_FLAG_REC_BUTTON_PRESSED;
	TEST_EQ(VbUserConfirms(&ctx,
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
	TEST_EQ(VbUserConfirms(&ctx,
			       VB_CONFIRM_SPACE_MEANS_NO |
			       VB_CONFIRM_MUST_TRUST_KEYBOARD),
		0, "Recovery button stuck");

	printf("...done.\n");
}

static void VbBootTest(void)
{
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_NORMAL);
	TEST_EQ(VbBootNormal(&ctx), 1002, "VbBootNormal()");
	TEST_EQ(VbGetMode(), VB_EC_NORMAL, "vboot_mode normal");
}

static void VbBootDevTest(void)
{
	printf("Testing VbBootDeveloper()...\n");

	/* Proceed after timeout */
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_DEVELOPER);
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Timeout");
	TEST_EQ(VbGetMode(), VB_EC_DEVELOPER, "vboot_mode developer");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");

	/* Proceed to legacy after timeout if GBB flag set */
	ResetMocks();
	sd->gbb_flags |= VB2_GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY |
			VB2_GBB_FLAG_FORCE_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Timeout");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	/* Proceed to legacy after timeout if boot legacy and default boot
	 * legacy are set */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_LEGACY);
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Timeout");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	/* Proceed to legacy boot mode only if enabled */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_LEGACY);
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Timeout");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Proceed to usb after timeout if boot usb and default boot
	 * usb are set */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_USB);
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloper(&ctx), 0, "Ctrl+U USB");

	/* Proceed to usb boot mode only if enabled */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_USB);
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Timeout");

	/* If no USB tries fixed disk */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_USB);
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Ctrl+U enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Up arrow is uninteresting / passed to VbCheckDisplayKey() */
	ResetMocks();
	mock_keypress[0] = VB_KEY_UP;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Up arrow");

	/* Shutdown requested in loop */
	ResetMocks();
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloper(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");

	/* Shutdown requested by keyboard in loop */
	ResetMocks();
	mock_keypress[0] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloper(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested by keyboard");

	/* Space goes straight to recovery if no virtual dev switch */
	ResetMocks();
	mock_keypress[0] = ' ';
	TEST_EQ(VbBootDeveloper(&ctx),
		VBERROR_LOAD_KERNEL_RECOVERY,
		"Space = recovery");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_RW_DEV_SCREEN, "  recovery reason");

	/* Space asks to disable virtual dev switch */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&ctx), VBERROR_REBOOT_REQUIRED,
		"Space = tonorm");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_TO_NORM_CONFIRMED,
		"  confirm screen");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 1,
		"  disable dev request");

	/* Space-space doesn't disable it */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	mock_keypress[1] = ' ';
	mock_keypress[2] = 0x1b;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Space-space");
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
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Enter ignored");

	/* Enter does if GBB flag set */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	sd->gbb_flags |= VB2_GBB_FLAG_ENTER_TRIGGERS_TONORM;
	mock_keypress[0] = '\r';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&ctx), VBERROR_REBOOT_REQUIRED,
		"Enter = tonorm");

	/* Tonorm ignored if GBB forces dev switch on */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	sd->gbb_flags |= VB2_GBB_FLAG_FORCE_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&ctx), 1002,
		"Can't tonorm gbb-dev");

	/* Shutdown requested at tonorm screen */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloper(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested at tonorm");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");

	/* Shutdown requested by keyboard at tonorm screen */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloper(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested by keyboard at nonorm");

	/* Ctrl+D dismisses warning */
	ResetMocks();
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Ctrl+D");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Ctrl+D doesn't boot legacy even if GBB flag is set */
	ResetMocks();
	mock_keypress[0] = 0x04;
	sd->gbb_flags |= VB2_GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Ctrl+D");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Ctrl+L tries legacy boot mode only if enabled */
	ResetMocks();
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Ctrl+L normal");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	ResetMocks();
	sd->gbb_flags |= VB2_GBB_FLAG_FORCE_DEV_BOOT_LEGACY;
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloper(&ctx), 1002,
		"Ctrl+L force legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloper(&ctx), 1002,
		"Ctrl+L nv legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	ResetMocks();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_ENABLE_LEGACY;
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloper(&ctx), 1002,
		"Ctrl+L fwmp legacy");
	TEST_EQ(vbexlegacy_called, 1, "  fwmp legacy");

	/* Ctrl+U boots USB only if enabled */
	ResetMocks();
	mock_keypress[0] = 0x15;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Ctrl+U normal");

	/* Ctrl+U enabled, with good USB boot */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloper(&ctx), 0, "Ctrl+U USB");

	/* Ctrl+U enabled via GBB */
	ResetMocks();
	sd->gbb_flags |= VB2_GBB_FLAG_FORCE_DEV_BOOT_USB;
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloper(&ctx), 0, "Ctrl+U force USB");

	/* Ctrl+U enabled via FWMP */
	ResetMocks();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_ENABLE_USB;
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloper(&ctx), 0, "Ctrl+U force USB");

	/* If no USB, eventually times out and tries fixed disk */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 0x15;
	TEST_EQ(VbBootDeveloper(&ctx), 1002, "Ctrl+U enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");

	/* If dev mode is disabled, goes to TONORM screen repeatedly */
	ResetMocks();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_DISABLE_BOOT;
	mock_keypress[0] = '\x1b';  /* Just causes TONORM again */
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloper(&ctx), VBERROR_REBOOT_REQUIRED,
		"FWMP dev disabled");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_TO_NORM_CONFIRMED,
		"  confirm screen");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 1,
		"  disable dev request");

	/* Shutdown requested when dev disabled */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_DISABLE_BOOT;
	shutdown_request_calls_left = 1;
	TEST_EQ(VbBootDeveloper(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested when dev disabled");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_TO_NORM,
		"  tonorm screen");

	/* Shutdown requested by keyboard when dev disabled */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_DISABLE_BOOT;
	mock_keypress[0] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloper(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested by keyboard when dev disabled");

	printf("...done.\n");
}

static void VbBootRecTest(void)
{
	printf("Testing VbBootRecovery()...\n");

	/* Shutdown requested in loop */
	ResetMocks();
	shutdown_request_calls_left = 10;
	VbExEcEnteringMode(0, VB_EC_RECOVERY);
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested");
	TEST_EQ(VbGetMode(), VB_EC_RECOVERY, "vboot_mode recovery");

	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");

	/* Shutdown requested by keyboard */
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_RECOVERY);
	mock_keypress[0] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested by keyboard");

	/* Remove disks */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 1;
	mock_num_disks[1] = 1;
	mock_num_disks[2] = 1;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Remove");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");

	/* No removal if dev switch is on */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 1;
	mock_num_disks[1] = 1;
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
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
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"No remove in rec");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");

	/* Removal if no disk initially found, but found on second attempt */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 0;
	mock_num_disks[1] = 1;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Remove");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");

	/* Bad disk count doesn't require removal */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = -1;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	shutdown_request_calls_left = 10;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Bad disk count");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");

	/* Ctrl+D ignored for many reasons... */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	trust_ec = 0;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
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
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if already in dev mode");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if recovery not manually triggered");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_BOOT_REC_SWITCH_ON;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
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
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
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
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_SHUTDOWN_REQUESTED,
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
	TEST_EQ(VbBootRecovery(&ctx), VBERROR_REBOOT_REQUIRED,
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
	TEST_EQ(VbBootRecovery(&ctx),
		VBERROR_TPM_SET_BOOT_MODE_STATE,
		"Ctrl+D todev failure");

	printf("...done.\n");
}


int main(void)
{
	VbUserConfirmsTest();
	VbBootTest();
	VbBootDevTest();
	VbBootRecTest();

	return gTestSuccess ? 0 : 255;
}
