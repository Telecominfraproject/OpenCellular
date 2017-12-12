/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests detachable menu UI
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
#include "vboot_api.h"
#include "vboot_audio.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"
#include "vboot_struct.h"
#include "vboot_ui_menu_private.h"

/* Mock data */
static VbCommonParams cparams;
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static GoogleBinaryBlockHeader gbb;
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
static uint32_t mock_keypress[32];
static uint32_t mock_keyflags[32];
static uint32_t mock_keypress_count;
static uint32_t mock_switches[8];
static uint32_t mock_switches_count;
static int mock_switches_are_stuck;
static uint32_t screens_displayed[32];
static uint32_t screens_count = 0;
static uint32_t mock_num_disks[8];
static uint32_t mock_num_disks_count;

extern enum VbEcBootMode_t VbGetMode(void);
extern struct RollbackSpaceFwmp *VbApiKernelGetFwmp(void);

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	memset(&cparams, 0, sizeof(cparams));
	cparams.shared_data_size = sizeof(shared_data);
	cparams.shared_data_blob = shared_data;
	cparams.gbb_data = &gbb;
	cparams.gbb = &gbb;

	memset(&gbb, 0, sizeof(gbb));
	gbb.major_version = GBB_MAJOR_VER;
	gbb.minor_version = GBB_MINOR_VER;
	gbb.flags = 0;

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

	current_menu = VB_MENU_DEV_WARNING;
	prev_menu = VB_MENU_DEV_WARNING;
	current_menu_idx = VB_WARN_POWER_OFF;
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

uint32_t VbTryLoadKernel(struct vb2_context *ctx, VbCommonParams *cparams,
			 uint32_t get_info_flags)
{
	return vbtlk_retval + get_info_flags;
}

VbError_t VbDisplayScreen(struct vb2_context *ctx, VbCommonParams *cparams,
			  uint32_t screen, int force)
{
	if (screens_count < ARRAY_SIZE(screens_displayed))
		screens_displayed[screens_count++] = screen;
	printf("VbDisplayScreen: screens_displayed[%d] = 0x%x\n",
	       screens_count, screen);
	return VBERROR_SUCCESS;
}

VbError_t VbDisplayMenu(struct vb2_context *ctx,
			VbCommonParams *cparams, uint32_t screen, int force,
			uint32_t selected_index, uint32_t disabled_idx_mask)
{
	if (screens_count < ARRAY_SIZE(screens_displayed))
		screens_displayed[screens_count++] = screen;
	else
		printf("Ran out of screens_displayed entries!\n");
	printf("VbDisplayMenu: screens_displayed[%d] = 0x%x,"
	       " selected_index = %u, disabled_idx_mask = 0x%x\n",
	       screens_count, screen,
	       selected_index, disabled_idx_mask);

	return VBERROR_SUCCESS;
}

uint32_t SetVirtualDevMode(int val)
{
	virtdev_set = val;
	return virtdev_retval;
}

/* Tests */

static void VbBootTest(void)
{
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_NORMAL);
	TEST_EQ(VbBootNormal(&ctx, &cparams), 1002, "VbBootNormal()");
	TEST_EQ(VbGetMode(), VB_EC_NORMAL, "vboot_mode normal");
}

static void VbBootDevTest(void)
{
	printf("Testing VbBootDeveloperMenu()...\n");

	/* Proceed after timeout */
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_DEVELOPER);
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Timeout");
	TEST_EQ(VbGetMode(), VB_EC_DEVELOPER, "vboot_mode developer");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");

	/* Proceed to legacy after timeout if GBB flag set */
	ResetMocks();
	sd->gbb_flags |= GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY |
		     GBB_FLAG_FORCE_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Timeout");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	/* Proceed to legacy after timeout if boot legacy and default boot
	 * legacy are set */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_LEGACY);
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Timeout");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	/* Proceed to legacy boot mode only if enabled */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_LEGACY);
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Timeout");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Proceed to usb after timeout if boot usb and default boot
	 * usb are set */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_USB);
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 0, "Ctrl+U USB");

	/* Proceed to usb boot mode only if enabled */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_USB);
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Timeout");

	/* If no USB tries fixed disk */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_USB);
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Ctrl+U enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Shutdown requested in loop */
	ResetMocks();
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");

#if 0
	/* Space goes straight to recovery if no virtual dev switch */
	ResetMocks();
	mock_keypress[0] = ' ';
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams),
		VBERROR_LOAD_KERNEL_RECOVERY,
		"Space = recovery");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST),
		VB2_RECOVERY_RW_DEV_SCREEN, "  recovery reason");
#endif
	/*
	 * Pushing power should shut down the DUT because default
	 * selection is power off
	 */
	ResetMocks();
	mock_keypress[0] = 0x90; // power button
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"dev warning menu: default to power off");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");

	/* Disable developer mode */
	ResetMocks();
	shared->flags = VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = 0x62; // volume up
	mock_keypress[1] = 0x90; // power button
	mock_keypress[2] = 0x90; // power button
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), VBERROR_REBOOT_REQUIRED,
		"disable developer mode");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[4], VB_SCREEN_TO_NORM_CONFIRMED,
		"  confirm screen");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 1,
		"  disable dev request");

#if 0
	/* Space-space doesn't disable it */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = ' ';
	mock_keypress[1] = ' ';
	mock_keypress[2] = 0x1b;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Space-space");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");

	/* Enter doesn't by default */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = '\r';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Enter ignored");

	/* Enter does if GBB flag set */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	sd->gbb_flags |= GBB_FLAG_ENTER_TRIGGERS_TONORM;
	mock_keypress[0] = '\r';
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), VBERROR_REBOOT_REQUIRED,
		"Enter = tonorm");
#endif

	/* Tonorm ignored if GBB forces dev switch on */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	sd->gbb_flags |= GBB_FLAG_FORCE_DEV_SWITCH_ON;
	mock_keypress[0] = 0x62; // volume up
	mock_keypress[1] = 0x90; // power
	mock_keypress[2] = 0x90; // power
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002,
		"Can't tonorm gbb-dev");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  warning screen");

	/* Shutdown requested at tonorm screen */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = 0x62; // volume up
	mock_keypress[1] = 0x90; // power
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested at tonorm");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		" developer warning screen: power off");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  developer warning screen: enable root verification");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  developer warning screen: power off");

	/* Ctrl+D dismisses warning */
	ResetMocks();
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Ctrl+D");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Ctrl+D doesn't boot legacy even if GBB flag is set */
	ResetMocks();
	mock_keypress[0] = 0x04;
	sd->gbb_flags |= GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Ctrl+D");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Ctrl+L tries legacy boot mode only if enabled */
	ResetMocks();
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Ctrl+L normal");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

#if 0
	ResetMocks();
	sd->gbb_flags |= GBB_FLAG_FORCE_DEV_BOOT_LEGACY;
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002,
		"Ctrl+L force legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002,
		"Ctrl+L nv legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");

	ResetMocks();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_ENABLE_LEGACY;
	mock_keypress[0] = 0x0c;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002,
		"Ctrl+L fwmp legacy");
	TEST_EQ(vbexlegacy_called, 1, "  fwmp legacy");
#endif

	/* Ctrl+U boots USB only if enabled */
	ResetMocks();
	mock_keypress[0] = 0x15;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Ctrl+U normal");

	/* Ctrl+U enabled, with good USB boot */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 0, "Ctrl+U USB");

	/* Ctrl+U enabled via GBB */
	ResetMocks();
	sd->gbb_flags |= GBB_FLAG_FORCE_DEV_BOOT_USB;
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 0, "Ctrl+U force USB");

	/* Ctrl+U enabled via FWMP */
	ResetMocks();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_ENABLE_USB;
	mock_keypress[0] = 0x15;
	vbtlk_retval = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 0, "Ctrl+U force USB");

	/* Now go to USB boot through menus */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT,
		   VB2_DEV_DEFAULT_BOOT_USB);
	mock_keypress[1] = 0x62; // volume up: Enable Root Verification
	mock_keypress[2] = 0x62; // volume up: Show Debug Info
	mock_keypress[3] = 0x62; // volume up: Developer Options
	mock_keypress[4] = 0x90; // power button
	mock_keypress[5] = 0x90; // power button: default to USB boot
	// NOTE: need to check the return value of this....
	VbBootDeveloperMenu(&ctx, &cparams);
	/* TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), */
	/* 	VBERROR_SHUTDOWN_REQUESTED, */
	/* 	"go through menu to USB boot"); */
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[3], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  power button");
	TEST_EQ(screens_displayed[4], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[5], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");

	/* If no USB, eventually times out and tries fixed disk */
	ResetMocks();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 0x15;
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), 1002, "Ctrl+U enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");

	/* If dev mode is disabled, goes to TONORM screen repeatedly */
	ResetMocks();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_DISABLE_BOOT;
	mock_keypress[0] = '\x1b';  /* Just causes TONORM again */
	mock_keypress[1] = '\r';
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), VBERROR_REBOOT_REQUIRED,
		"FWMP dev disabled");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
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
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested when dev disabled");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");

	printf("...done.\n");
}

static void VbBootRecTest(void)
{
	printf("Testing VbBootRecoveryMenu()...\n");

	/* Shutdown requested in loop */
	ResetMocks();
	shutdown_request_calls_left = 10;
	VbExEcEnteringMode(0, VB_EC_RECOVERY);
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested");
	TEST_EQ(VbGetMode(), VB_EC_RECOVERY, "vboot_mode recovery");

	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");

	/* Remove disks */
	ResetMocks();
	shutdown_request_calls_left = 100;
	mock_num_disks[0] = 1;
	mock_num_disks[1] = 1;
	mock_num_disks[2] = 1;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
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
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
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
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
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
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
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
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Bad disk count");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");

#if 0
	/* Ctrl+D ignored for many reasons.  Should always be ignored
	   in recovery for detachables. */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	trust_ec = 0;
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if EC not trusted");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON |
		VBSD_BOOT_DEV_SWITCH_ON;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if already in dev mode");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if recovery not manually triggered");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		 "  todev screen");

	ResetMocks();
	shared->flags = VBSD_BOOT_REC_SWITCH_ON;
	trust_ec = 1;
	shutdown_request_calls_left = 100;
	mock_keypress[0] = 0x04;
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if no virtual dev switch");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV_MENU,
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
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if phys rec button is still pressed");
	TEST_NEQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		 "  todev screen");
#endif

	/* Navigate to confirm dev mode selection and then cancel */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	trust_ec = 1;
	mock_keypress[0] = 0x93; // volume up/down to exit insert
	mock_keyflags[0] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[1] = 0x62; // volume up
	mock_keyflags[1] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[2] = 0x62; // volume up
	mock_keyflags[2] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[3] = 0x62; // volume up
	mock_keyflags[3] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[4] = 0x90; // power button
	mock_keypress[5] = 0x90; // power button
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_SHUTDOWN_REQUESTED,
		"go to TO_DEV screen and cancel");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: language");
	TEST_EQ(screens_displayed[2], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: power off");
	TEST_EQ(screens_displayed[3], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: show debug info");
	TEST_EQ(screens_displayed[4], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: disable os verification");
	TEST_EQ(screens_displayed[5], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: cancel");
	TEST_EQ(screens_displayed[6], VB_SCREEN_RECOVERY_MENU,
		"  back to recovery menu");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");

	/* Navigate to confirm dev mode selection and then confirm */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	trust_ec = 1;
	mock_keypress[0] = 0x93; // volume up/down to exit insert
	mock_keyflags[0] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[1] = 0x62; // volume up
	mock_keyflags[1] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[2] = 0x62; // volume up
	mock_keyflags[2] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[3] = 0x62; // volume up
	mock_keyflags[3] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[4] = 0x90; // power button
	mock_keypress[5] = 0x62; // volume up
	mock_keyflags[5] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[6] = 0x90; // power button
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams), VBERROR_REBOOT_REQUIRED,
		"go to TO_DEV screen and confirm");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: language");
	TEST_EQ(screens_displayed[2], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: power off");
	TEST_EQ(screens_displayed[3], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: show debug info");
	TEST_EQ(screens_displayed[4], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: disable os verification");
	TEST_EQ(screens_displayed[5], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: cancel");
	TEST_EQ(screens_displayed[6], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: confirm disabling os verification");
	TEST_EQ(virtdev_set, 1, "  virtual dev mode on");

#if 0
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
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams),
		VBERROR_TPM_SET_BOOT_MODE_STATE,
		"Ctrl+D todev failure");
#endif

	printf("...done.\n");
}

static void VbTestLanguageMenu(void)
{
	printf("Testing VbTestLanguageMenu()...\n");

	/* Navigate to all language menus from recovery */
	ResetMocks();
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH | VBSD_BOOT_REC_SWITCH_ON;
	shutdown_request_calls_left = 100;
	vbtlk_retval = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	trust_ec = 1;
	mock_keypress[0] = 0x93; // volume up/down to exit insert
	mock_keypress[1] = 0x63; // volume down
	mock_keypress[2] = 0x90; // power button
	mock_keypress[3] = 0x90; // power button: select current language
	mock_keypress[4] = 0x62; // volume up: show debug info
	mock_keypress[5] = 0x62; // volume up: disable os verification
	mock_keypress[6] = 0x90; // power button: select disable os verification
	mock_keypress[7] = 0x63; // volume down: power off
	mock_keyflags[7] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[8] = 0x63; // volume down: language
	mock_keyflags[8] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[9] = 0x90; // power button: select language
	mock_keypress[10] = 0x90; // power button: select current language to go back
	mock_keypress[11] = 0x63; // volume down: power off
	mock_keyflags[11] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[12] = 0x90; // power button: select power off
	TEST_EQ(VbBootRecoveryMenu(&ctx, &cparams), VBERROR_SHUTDOWN_REQUESTED,
		"go to language menu");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: language");
	TEST_EQ(screens_displayed[2], VB_SCREEN_RECOVERY_MENU,
		"  select language option");
	TEST_EQ(screens_displayed[3], VB_SCREEN_LANGUAGES_MENU,
		"  language menu");
	TEST_EQ(screens_displayed[4], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: power off");
	TEST_EQ(screens_displayed[5], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: show debug info");
	TEST_EQ(screens_displayed[6], VB_SCREEN_RECOVERY_MENU,
		"  recovery menu: disable os verification");
	TEST_EQ(screens_displayed[7], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery menu: select disable os verification");
	TEST_EQ(screens_displayed[8], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to dev menu: cancel");
	TEST_EQ(screens_displayed[9], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to dev menu: power off");
	TEST_EQ(screens_displayed[10], VB_SCREEN_LANGUAGES_MENU,
		"  recovery to dev menu: language");
	TEST_EQ(screens_displayed[11], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to dev menu: cancel");
	TEST_EQ(screens_displayed[12], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to dev menu: power off");

	/* Navigate to all language menus from developer menu */
	ResetMocks();
	shared->flags = VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = 0x63; // volume down: language
	mock_keypress[1] = 0x90; // power button: select language
	mock_keypress[2] = 0x90; // power button: select current language
	mock_keypress[3] = 0x62; // volume up: enable root verification
	mock_keypress[4] = 0x90; // power button: select enable root verification
	mock_keypress[5] = 0x63; // volume down: cancel
	mock_keypress[6] = 0x63; // volume down: power off
	mock_keypress[7] = 0x63; // volume down: language
	mock_keypress[8] = 0x90; // power button: select language
	mock_keypress[9] = 0x90; // power button: select current language
	mock_keypress[10] = 0x63; // volume down: cancel
	mock_keypress[11] = 0x90; // power button: return to dev warning screen
	mock_keypress[12] = 0x62; // volume up: enable root verification
	mock_keypress[13] = 0x62; // volume up: show debug info
	mock_keypress[14] = 0x62; // volume up: developer options
	mock_keypress[15] = 0x90; // power button: select developer options
	mock_keypress[16] = 0x63; // volume down: cancel
	mock_keypress[17] = 0x63; // volume down: power off
	mock_keypress[18] = 0x63; // volume down: language
	mock_keypress[19] = 0x90; // power button: select language
	mock_keypress[20] = 0x90; // power button: select current language
	mock_keypress[21] = 0x90; // power button: select power off
	TEST_EQ(VbBootDeveloperMenu(&ctx, &cparams), VBERROR_SHUTDOWN_REQUESTED,
		" scroll through all language menus in developer options");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: power off");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: language");
	TEST_EQ(screens_displayed[2], VB_SCREEN_LANGUAGES_MENU,
		"  language menu: select current language");
	TEST_EQ(screens_displayed[3], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: cancel ");
	TEST_EQ(screens_displayed[4], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: enable root verification");
	TEST_EQ(screens_displayed[5], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: cancel");
	TEST_EQ(screens_displayed[6], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: power off");
	TEST_EQ(screens_displayed[7], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: language");
	TEST_EQ(screens_displayed[8], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: language");
	TEST_EQ(screens_displayed[9], VB_SCREEN_LANGUAGES_MENU,
		"  language menu: select current language");
	TEST_EQ(screens_displayed[10], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: confirm enabling os verification");
	TEST_EQ(screens_displayed[11], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: cancel");
	TEST_EQ(screens_displayed[12], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: enable root verification");
	TEST_EQ(screens_displayed[13], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: show debug info");
	TEST_EQ(screens_displayed[14], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: developer options");
	TEST_EQ(screens_displayed[15], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  select developer options");
	TEST_EQ(screens_displayed[16], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: boot developer image");
	TEST_EQ(screens_displayed[17], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: cancel");
	TEST_EQ(screens_displayed[18], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: power off");
	TEST_EQ(screens_displayed[19], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: language");
	TEST_EQ(screens_displayed[20], VB_SCREEN_LANGUAGES_MENU,
		"  language menu");
	TEST_EQ(screens_displayed[21], VB_SCREEN_DEVELOPER_MENU,
		"  select current language");
	TEST_EQ(screens_displayed[22], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: power off");

  	printf("...done.\n");
}

int main(void)
{
	VbTestLanguageMenu();
	VbBootTest();
	VbBootDevTest();
	VbBootRecTest();

	return gTestSuccess ? 0 : 255;
}
