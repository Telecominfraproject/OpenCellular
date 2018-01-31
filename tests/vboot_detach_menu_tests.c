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
static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static LoadKernelParams lkp;
static uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
static struct vb2_context ctx;
static struct vb2_shared_data *sd;

static int shutdown_request_calls_left;
static int audio_looping_calls_left;
static VbError_t vbtlk_retval[5];
static VbError_t vbtlk_last_retval;
static int vbtlk_retval_count;
static const VbError_t vbtlk_retval_fixed = 1002;
static int vbexlegacy_called;
static int debug_info_displayed;
static int trust_ec;
static int virtdev_set;
static uint32_t virtdev_retval;
static uint32_t mock_keypress[64];
static uint32_t mock_keyflags[64];
static uint32_t mock_keypress_count;
static uint32_t mock_switches[8];
static uint32_t mock_switches_count;
static int mock_switches_are_stuck;
static uint32_t screens_displayed[64];
static uint32_t screens_count = 0;
static uint32_t beeps_played[64];
static uint32_t beeps_count = 0;

extern enum VbEcBootMode_t VbGetMode(void);
extern struct RollbackSpaceFwmp *VbApiKernelGetFwmp(void);

/* Reset mock data (for use before each test) */
static void ResetMocks(void)
{
	memset(VbApiKernelGetFwmp(), 0, sizeof(struct RollbackSpaceFwmp));

	memset(&shared_data, 0, sizeof(shared_data));
	VbSharedDataInit(shared, sizeof(shared_data));
	shared->flags = VBSD_HONOR_VIRT_DEV_SWITCH |
			VBSD_BOOT_FIRMWARE_VBOOT2;

	memset(&lkp, 0, sizeof(lkp));

	memset(&ctx, 0, sizeof(ctx));
	ctx.workbuf = workbuf;
	ctx.workbuf_size = sizeof(workbuf);
	vb2_init_context(&ctx);
	vb2_nv_init(&ctx);

	sd = vb2_get_sd(&ctx);
	sd->vbsd = shared;

	/* In recovery we have 50 keyscans per disk scan, this must be high. */
	shutdown_request_calls_left = 301;
	audio_looping_calls_left = 60;
	vbexlegacy_called = 0;
	debug_info_displayed = 0;
	trust_ec = 0;
	virtdev_set = 0;
	virtdev_retval = 0;

	vbtlk_last_retval = vbtlk_retval_fixed - VB_DISK_FLAG_FIXED;
	memset(vbtlk_retval, 0, sizeof(vbtlk_retval));
	vbtlk_retval_count = 0;

	memset(screens_displayed, 0, sizeof(screens_displayed));
	screens_count = 0;
	memset(beeps_played, 0, sizeof(beeps_played));
	beeps_count = 0;

	memset(mock_keypress, 0, sizeof(mock_keypress));
	memset(mock_keyflags, 0, sizeof(mock_keyflags));
	mock_keypress_count = 0;

	memset(mock_switches, 0, sizeof(mock_switches));
	mock_switches_count = 0;
	mock_switches_are_stuck = 0;
}

static void ResetMocksForDeveloper(void)
{
	ResetMocks();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	VbExEcEnteringMode(0, VB_EC_DEVELOPER);
	shutdown_request_calls_left = -1;
}

static void ResetMocksForManualRecovery(void)
{
	ResetMocks();
	shared->flags |= VBSD_BOOT_REC_SWITCH_ON;
	trust_ec = 1;
	VbExEcEnteringMode(0, VB_EC_RECOVERY);
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

VbError_t VbTryLoadKernel(struct vb2_context *ctx, uint32_t get_info_flags)
{
	if (vbtlk_retval_count < ARRAY_SIZE(vbtlk_retval) &&
	    vbtlk_retval[vbtlk_retval_count] != 0)
		vbtlk_last_retval = vbtlk_retval[vbtlk_retval_count++];
	return vbtlk_last_retval + get_info_flags;
}

VbError_t VbDisplayScreen(struct vb2_context *ctx, uint32_t screen, int force)
{
	if (screens_count < ARRAY_SIZE(screens_displayed))
		screens_displayed[screens_count++] = screen;
	printf("VbDisplayScreen: screens_displayed[%d] = 0x%x\n",
	       screens_count - 1, screen);
	return VBERROR_SUCCESS;
}

VbError_t VbDisplayMenu(struct vb2_context *ctx, uint32_t screen, int force,
			uint32_t selected_index, uint32_t disabled_idx_mask)
{
	if (screens_count < ARRAY_SIZE(screens_displayed))
		screens_displayed[screens_count++] = screen;
	else
		printf("Ran out of screens_displayed entries!\n");
	printf("VbDisplayMenu: screens_displayed[%d] = 0x%x,"
	       " selected_index = %u, disabled_idx_mask = 0x%x\n",
	       screens_count - 1, screen,
	       selected_index, disabled_idx_mask);

	return VBERROR_SUCCESS;
}

VbError_t VbDisplayDebugInfo(struct vb2_context *ctx)
{
	debug_info_displayed = 1;
	return VBERROR_SUCCESS;
}

VbError_t VbExBeep(uint32_t msec, uint32_t frequency)
{
	if (beeps_count < ARRAY_SIZE(beeps_played))
		beeps_played[beeps_count++] = frequency;
	printf("VbExBeep: beeps_played[%d] = %dHz for %dms\n",
	       beeps_count - 1, frequency, msec);
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
	TEST_EQ(VbBootNormal(&ctx), vbtlk_retval_fixed, "VbBootNormal()");
	TEST_EQ(VbGetMode(), VB_EC_NORMAL, "vboot_mode normal");
}

static void VbBootDevTest(void)
{
	int i;

	printf("Testing VbBootDeveloperMenu()...\n");

	/* Proceed after timeout */
	ResetMocksForDeveloper();
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed, "Timeout");
	TEST_EQ(VbGetMode(), VB_EC_DEVELOPER, "vboot_mode developer");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 0, "  no beeps on normal boot");

	/* Proceed to legacy after timeout if GBB flag set */
	ResetMocksForDeveloper();
	sd->gbb_flags |= GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY |
		     GBB_FLAG_FORCE_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"default legacy GBB");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 1, "  error beep: legacy BIOS not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Proceed to legacy after timeout if boot legacy and default boot
	 * legacy are set */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT, VB2_DEV_DEFAULT_BOOT_LEGACY);
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"default legacy NV");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 1, "  error beep: legacy BIOS not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Proceed to legacy boot mode only if enabled */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT, VB2_DEV_DEFAULT_BOOT_LEGACY);
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"default legacy not enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 2, "  error beeps: legacy boot not enabled");
	TEST_EQ(beeps_played[0], 400, "    first error beep");
	TEST_EQ(beeps_played[1], 400, "    second error beep");

	/* Proceed to usb after timeout if boot usb and default boot
	 * usb are set */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT, VB2_DEV_DEFAULT_BOOT_USB);
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), 0, "Ctrl+U USB");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* Proceed to usb boot mode only if enabled */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT, VB2_DEV_DEFAULT_BOOT_USB);
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"default USB not enabled");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 2, "  error beeps: USB boot not enabled");
	TEST_EQ(beeps_played[0], 400, "    first error beep");
	TEST_EQ(beeps_played[1], 400, "    second error beep");

	/* If no USB tries fixed disk */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT, VB2_DEV_DEFAULT_BOOT_USB);
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"default USB with no disk");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 1, "  error beep: USB not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Shutdown requested in loop */
	ResetMocksForDeveloper();
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/*
	 * Pushing power should shut down the DUT because default
	 * selection is power off
	 */
	ResetMocksForDeveloper();
	mock_keypress[0] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"dev warning menu: default to power off");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* Selecting Power Off in developer options. */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Power Off
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Power Off in DEVELOPER");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* Pressing ENTER is equivalent to power button. */
	ResetMocksForDeveloper();
	mock_keypress[0] = '\r';
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"dev warning menu: ENTER is power button");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* ENTER functionality is unaffected by GBB flag in detachable UI. */
	ResetMocksForDeveloper();
	sd->gbb_flags |= GBB_FLAG_ENTER_TRIGGERS_TONORM;
	mock_keypress[0] = '\r';
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"dev warning menu: ENTER unaffected by GBB");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* Pressing SPACE or VolUp+Down combo has no effect in dev mode */
	ResetMocksForDeveloper();
	mock_keypress[0] = ' ';
	mock_keypress[1] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS;
	mock_keypress[2] = VB_BUTTON_POWER_SHORT_PRESS;	// select Power Off
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"SPACE or VolUp+Down have no effect");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* Disable developer mode */
	ResetMocksForDeveloper();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = VB_BUTTON_VOL_UP_SHORT_PRESS;
	mock_keypress[1] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[2] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_REBOOT_REQUIRED,
		"disable developer mode");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_TO_NORM_CONFIRMED,
		"  confirm screen");
	TEST_EQ(screens_displayed[4], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 5, "  no extra screens");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 1,
		"  disable dev request");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(beeps_count, 0, "  no beep on reboot");

	/* Tonorm ignored if GBB forces dev switch on */
	ResetMocksForDeveloper();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	sd->gbb_flags |= GBB_FLAG_FORCE_DEV_SWITCH_ON;
	mock_keypress[0] = VB_BUTTON_VOL_UP_SHORT_PRESS;
	mock_keypress[1] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[2] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Can't tonorm gbb-dev");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: power off");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  wanring screen: enable verification");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen: confirm");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK,
		"  blank (error flash)");
	TEST_EQ(screens_displayed[4], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen: confirm");
	TEST_EQ(screens_displayed[5], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 6, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timeout");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Shutdown requested at tonorm screen */
	ResetMocksForDeveloper();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	mock_keypress[0] = VB_BUTTON_VOL_UP_SHORT_PRESS;
	mock_keypress[1] = VB_BUTTON_POWER_SHORT_PRESS;
	shutdown_request_calls_left = 2;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested at tonorm");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		" developer warning screen: power off");
	TEST_EQ(screens_displayed[1], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  developer warning screen: enable root verification");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  developer warning screen: power off");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on shutdown");

	/* Ctrl+D dismisses warning */
	ResetMocksForDeveloper();
	mock_keypress[0] = 'D' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed, "Ctrl+D");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on normal boot");

	/* Ctrl+D doesn't boot legacy even if GBB flag is set */
	ResetMocksForDeveloper();
	mock_keypress[0] = 'D' & 0x1f;
	sd->gbb_flags |= GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed, "Ctrl+D");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");

	/* Volume-down long press shortcut acts like Ctrl+D */
	ResetMocksForDeveloper();
	mock_keypress[0] = VB_BUTTON_VOL_DOWN_LONG_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"VolDown long press");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0,
		"  recovery reason");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on normal boot");

	/* Ctrl+L tries legacy boot mode only if enabled */
	ResetMocksForDeveloper();
	mock_keypress[0] = 'L' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed, "Ctrl+L normal");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timed out");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Ctrl+L boots legacy if enabled by GBB flag */
	ResetMocksForDeveloper();
	sd->gbb_flags |= GBB_FLAG_FORCE_DEV_BOOT_LEGACY;
	mock_keypress[0] = 'L' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Ctrl+L force legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 1, "  error beep: legacy BIOS not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Ctrl+L boots legacy if enabled by NVRAM */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	mock_keypress[0] = 'L' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Ctrl+L nv legacy");
	TEST_EQ(vbexlegacy_called, 1, "  try legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 1, "  error beep: legacy BIOS not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Ctrl+L boots legacy if enabled by FWMP */
	ResetMocksForDeveloper();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_ENABLE_LEGACY;
	mock_keypress[0] = 'L' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Ctrl+L fwmp legacy");
	TEST_EQ(vbexlegacy_called, 1, "  fwmp legacy");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 1, "  error beep: legacy BIOS not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Ctrl+U boots USB only if enabled */
	ResetMocksForDeveloper();
	mock_keypress[0] = 'U' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Ctrl+U not enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timed out");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Ctrl+U enabled, with good USB boot */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 'U' & 0x1f;
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS, "Ctrl+U USB");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* Ctrl+U enabled, without valid USB */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 'U' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Ctrl+U without valid USB");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 1, "  error beep: USB not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Ctrl+U enabled via GBB */
	ResetMocksForDeveloper();
	sd->gbb_flags |= GBB_FLAG_FORCE_DEV_BOOT_USB;
	mock_keypress[0] = 0x15;
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS, "Ctrl+U force USB");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* Ctrl+U enabled via FWMP */
	ResetMocksForDeveloper();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_ENABLE_USB;
	mock_keypress[0] = 0x15;
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS, "Ctrl+U force USB");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* If no valid USB, eventually times out and tries fixed disk */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = 'U' & 0x1f;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Ctrl+U failed - no USB");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 1, "  error beep: USB not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Now go to USB boot through menus */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot From USB
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS,
		"Menu selected USB boot");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* If default USB, the option is preselected */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT, VB2_DEV_DEFAULT_BOOT_USB);
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS,
		"Menu selected USB default boot");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* If invalid USB, we still timeout after selecting it in menu */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot From USB
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Menu selected invalid USB boot");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 1, "  error beep: USB not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* If USB is disabled, we error flash/beep from the menu option */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot From USB
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Menu selected disabled USB boot");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Boot Legacy via menu and default */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	vb2_nv_set(&ctx, VB2_NV_DEV_DEFAULT_BOOT, VB2_DEV_DEFAULT_BOOT_LEGACY);
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Menu selected legacy boot");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(vbexlegacy_called, 2, "  tried legacy boot twice");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timeout");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: legacy boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: legacy boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: legacy boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  two error beeps: legacy BIOS not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");
	TEST_EQ(beeps_played[1], 200, "    low-frequency error beep");

	/* Refuse to boot legacy via menu if not enabled */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot From USB
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot Legacy BIOS
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Menu selected legacy boot when not enabled");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(vbexlegacy_called, 0, "  did not attempt legacy boot");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timeout");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: legacy boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: legacy boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Use volume-up long press shortcut to boot USB */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = VB_BUTTON_VOL_UP_LONG_PRESS;
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS, "VolUp USB");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* Can boot a valid USB image after failing to boot invalid image */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = VB_BUTTON_VOL_UP_LONG_PRESS;
	mock_keypress[1] = VB_BUTTON_VOL_UP_LONG_PRESS;
	vbtlk_retval[0] = VBERROR_SIMULATED - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS,
		"VolUp USB valid after invalid");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 1, "  error beep: first USB invalid");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Volume-up long press only works if USB is enabled */
	ResetMocksForDeveloper();
	mock_keypress[0] = VB_BUTTON_VOL_UP_LONG_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"VolUp not enabled");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timed out");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Volume-up long press without valid USB will still time out */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	mock_keypress[0] = VB_BUTTON_VOL_UP_LONG_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"VolUp without valid USB");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  blank (error flash)");
	TEST_EQ(screens_displayed[2], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(audio_looping_calls_left, 0, "  used up audio");
	TEST_EQ(beeps_count, 1, "  error beep: USB not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");

	/* Volume-up long press works from other menus, like LANGUAGE */
	ResetMocksForDeveloper();
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_USB, 1);
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Power Off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_LONG_PRESS;
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SUCCESS,
		"VolUp USB from LANGUAGE");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  language menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on USB boot");

	/* Can disable developer mode through TONORM screen */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enable os verification
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // confirm is the default
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_REBOOT_REQUIRED,
		"TONORM via menu");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 1,
		"  disable dev request");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning: enable os verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm: confirm");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_TO_NORM_CONFIRMED,
		"  confirm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps on reboot");

	/* If dev mode is disabled, goes to TONORM screen repeatedly */
	ResetMocksForDeveloper();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_DISABLE_BOOT;
	audio_looping_calls_left = 1;	/* Confirm audio doesn't tick down. */
	i = 0;
	mock_keypress[i++] = 'D' & 0x1f;  /* Just stays on TONORM and flashes */
	mock_keypress[i++] = 'U' & 0x1f;  /* same */
	mock_keypress[i++] = 'L' & 0x1f;  /* same */
	mock_keypress[i++] = VB_BUTTON_VOL_UP_LONG_PRESS;	/* same */
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_LONG_PRESS;	/* same */
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS;	/* noop */
	mock_keypress[i++] = '\r';
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_REBOOT_REQUIRED,
		"FWMP dev disabled");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 1,
		"  disable dev request");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_TO_NORM_CONFIRMED,
		"  confirm screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 10, "  5 * 2 error beeps: booting not allowed");
	TEST_EQ(beeps_played[0], 400, "    first error beep");
	TEST_EQ(beeps_played[1], 400, "    second error beep");
	TEST_EQ(beeps_played[2], 400, "    first error beep");
	TEST_EQ(beeps_played[3], 400, "    second error beep");
	TEST_EQ(beeps_played[4], 400, "    first error beep");
	TEST_EQ(beeps_played[5], 400, "    second error beep");
	TEST_EQ(beeps_played[6], 400, "    first error beep");
	TEST_EQ(beeps_played[7], 400, "    second error beep");
	TEST_EQ(beeps_played[8], 400, "    first error beep");
	TEST_EQ(beeps_played[9], 400, "    second error beep");

	/* Shutdown requested when dev disabled */
	ResetMocksForDeveloper();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_DISABLE_BOOT;
	shutdown_request_calls_left = 1;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested when dev disabled");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 0,
		"  did not exit dev mode");
	TEST_EQ(screens_displayed[0], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* Explicit Power Off when dev disabled */
	ResetMocksForDeveloper();
	VbApiKernelGetFwmp()->flags |= FWMP_DEV_DISABLE_BOOT;
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS;	// Power Off
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Power Off when dev disabled");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_DISABLE_DEV_REQUEST), 0,
		"  did not exit dev mode");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(audio_looping_calls_left, 0, "  aborts audio");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen: confirm enabling OS verification");
	/* Cancel option is removed with dev_disable_boot! */
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  tonorm screen: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps for power off");

	/* Show Debug Info displays debug info, then times out to boot */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS Verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"Show Debug Info");
	TEST_EQ(debug_info_displayed, 1, "  debug info displayed");
	TEST_EQ(vbexlegacy_called, 0, "  not legacy");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timed out");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps for debug info");

	printf("...done.\n");
}

static void VbBootRecTest(void)
{
	int i;

	printf("Testing VbBootRecoveryMenu()...\n");

	/* Shutdown requested in BROKEN */
	ResetMocks();
	VbExEcEnteringMode(0, VB_EC_RECOVERY);
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested in BROKEN");
	TEST_EQ(VbGetMode(), VB_EC_RECOVERY, "vboot_mode recovery");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* BROKEN screen with disks inserted */
	ResetMocks();
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[2] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[3] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested in BROKEN with disks");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* BROKEN screen with disks on second attempt */
	ResetMocks();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested in BROKEN with later disk");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* go to INSERT if dev switch is on */
	ResetMocks();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested in INSERT with dev switch");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* go to INSERT if recovery button physically pressed and EC trusted */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shutdown requested in INSERT with manual rec");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* Stay at BROKEN if recovery button not physically pressed */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	shared->flags &= ~VBSD_BOOT_REC_SWITCH_ON;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Go to BROKEN if recovery not manually requested");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* Stay at BROKEN if EC is untrusted */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	trust_ec = 0;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Go to BROKEN if EC is not trusted");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* INSERT boots without screens if we have a valid image on first try */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_SIMULATED - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SUCCESS,
		"INSERT boots without screens if valid on first try");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_EQ(screens_displayed[0], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 1, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* INSERT boots eventually if we get a valid image later */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[2] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[3] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[4] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SUCCESS,
		"INSERT boots after valid image appears");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* invalid image, then remove, then valid image */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_SIMULATED - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[2] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[3] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[4] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SUCCESS,
		"INSERT boots after valid image appears");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[2], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 3, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep on shutdown");

	/* Shortcuts that are always ignored in BROKEN for detachables. */
	ResetMocks();
	i = 0;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = 'D' & 0x1f;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = 'U' & 0x1f;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = 'L' & 0x1f;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_LONG_PRESS;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_LONG_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shortcuts ignored in BROKEN");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep from invalid keys");

	/* Shortcuts that are always ignored in INSERT for detachables. */
	ResetMocksForManualRecovery();
	i = 0;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = 'D' & 0x1f;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = 'U' & 0x1f;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = 'L' & 0x1f;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_LONG_PRESS;
	mock_keypress[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_LONG_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Shortcuts ignored in INSERT");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 2, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep from invalid keys");

	/* Power Off BROKEN through OPTIONS menu */
	ResetMocks();
	mock_keypress[0] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter options
	mock_keypress[1] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[2] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Power Off BROKEN through OPTIONS");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	TEST_EQ(screens_displayed[0], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_OPTIONS_MENU,
		"  options: cancel");
	TEST_EQ(screens_displayed[2], VB_SCREEN_OPTIONS_MENU,
		"  options: power off");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep from power off");

	/* Power Off NOGOOD through OPTIONS menu */
	ResetMocksForManualRecovery();
	mock_keypress[0] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter options
	mock_keypress[1] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[2] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Power Off NOGOOD through OPTIONS");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_OPTIONS_MENU,
		"  options: cancel");
	TEST_EQ(screens_displayed[2], VB_SCREEN_OPTIONS_MENU,
		"  options: power off");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep from power off");

	/* Power Off INSERT through TO_DEV menu */
	ResetMocksForManualRecovery();
	mock_keyflags[0] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[0] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter to_dev
	mock_keyflags[1] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[1] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[2] = VB_BUTTON_POWER_SHORT_PRESS;
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Power Off INSERT through TO_DEV");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to_dev: cancel");
	TEST_EQ(screens_displayed[2], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to_dev: power off");
	TEST_EQ(screens_displayed[3], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 4, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beep from power off");

	/* Navigate to confirm dev mode selection and then cancel */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	i = 0;
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter TO_DEV
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // confirm disabling
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // power off
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"go to TO_DEV screen and cancel");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: confirm disabling");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_INSERT,
		"  back to insert screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Navigate to confirm dev mode selection and then confirm */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	i = 0;
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter to_dev
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_REBOOT_REQUIRED,
		"go to TO_DEV screen and confirm");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(virtdev_set, 1, "  virtual dev mode on");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: confirm disabling os verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Untrusted keyboard cannot enter TO_DEV (must be malicious anyway) */
	ResetMocksForManualRecovery();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // try to_dev
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // try confirm
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Untrusted keyboard cannot enter TO_DEV");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(shutdown_request_calls_left, 0, "  timed out");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Untrusted keyboard cannot navigate in TO_DEV menu if already there */
	ResetMocksForManualRecovery();
	i = 0;
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter to_dev
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // try to confirm...
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Untrusted keyboard cannot navigate in TO_DEV");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(shutdown_request_calls_left, 0, "  timed out");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Handle TPM error in enabling dev mode */
	ResetMocksForManualRecovery();
	i = 0;
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter to_dev
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // confirm enabling
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	virtdev_retval = VBERROR_SIMULATED;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_TPM_SET_BOOT_MODE_STATE,
		"todev TPM failure");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(shutdown_request_calls_left, 0, "  aborted explicitly");
	TEST_EQ(virtdev_set, 1, "  virtual dev mode on");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  recovery to_dev menu: confirm disabling os verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Cannot enable dev mode if already enabled. */
	ResetMocksForManualRecovery();
	shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	i = 0;
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter to_dev
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Ctrl+D ignored if already in dev mode");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(shutdown_request_calls_left, 0, "  timed out");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode wasn't enabled again");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 2, "  played error beeps");
	TEST_EQ(beeps_played[0], 400, "    first beep");
	TEST_EQ(beeps_played[1], 400, "    second beep");

	/* Removing invalid USB drops back to INSERT from TO_DEV menu. */
	ResetMocksForManualRecovery();
	mock_keyflags[0] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[0] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter TO_DEV
	/* asynchronous transition to INSERT before keypress[50] */
	mock_keypress[55] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter OPTIONS
	mock_keypress[56] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[57] = VB_BUTTON_POWER_SHORT_PRESS;
	vbtlk_retval[0] = VBERROR_SIMULATED - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Drop back to INSERT from TO_DEV when removing invalid USB");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev menu");
	TEST_EQ(screens_displayed[2], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[3], VB_SCREEN_OPTIONS_MENU,
		"  options menu: cancel");
	TEST_EQ(screens_displayed[4], VB_SCREEN_OPTIONS_MENU,
		"  options menu: power off");
	TEST_EQ(screens_displayed[5], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 6, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Plugging in invalid USB drops back to NOGOOD from LANGUAGE. */
	ResetMocksForManualRecovery();
	mock_keypress[0] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter OPTIONS
	mock_keypress[1] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[2] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[3] = VB_BUTTON_POWER_SHORT_PRESS;
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_SIMULATED - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"Drop back to NOGOOD from LANGUAGE when inserting invalid USB");
	TEST_EQ(shutdown_request_calls_left, 0, "  timed out");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_OPTIONS_MENU,
		"  options menu: cancel");
	TEST_EQ(screens_displayed[2], VB_SCREEN_OPTIONS_MENU,
		"  options menu: power off");
	TEST_EQ(screens_displayed[3], VB_SCREEN_OPTIONS_MENU,
		"  options menu: language");
	TEST_EQ(screens_displayed[4], VB_SCREEN_LANGUAGES_MENU,
		"  languages menu");
	TEST_EQ(screens_displayed[5], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[6], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 7, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Plugging in valid USB boots straight from OPTIONS menu. */
	ResetMocksForManualRecovery();
	mock_keypress[0] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter OPTIONS
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	vbtlk_retval[1] = VBERROR_SUCCESS - VB_DISK_FLAG_REMOVABLE;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SUCCESS,
		"Boot by plugging in USB straight from OPTIONS menu");
	TEST_NEQ(shutdown_request_calls_left, 0, "  booted explicitly");
	TEST_EQ(virtdev_set, 0, "  virtual dev mode off");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_EQ(screens_displayed[0], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[1], VB_SCREEN_OPTIONS_MENU,
		"  options menu: cancel");
	TEST_EQ(screens_displayed[2], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, 3, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	printf("...done.\n");
}

static void VbTestLanguageMenu(void)
{
	int i;

	printf("Testing VbTestLanguageMenu()...\n");

	/* Navigate to language menu from BROKEN */
	ResetMocks();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter OPTIONS
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // languages
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // enter languages
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // select current lang
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // cancel -> BROKEN
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // power off
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"go to language menu from BROKEN");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  language menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OS_BROKEN,
		"  broken screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Navigate to all language menus from recovery */
	ResetMocksForManualRecovery();
	vbtlk_retval[0] = VBERROR_NO_DISK_FOUND - VB_DISK_FLAG_REMOVABLE;
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter OPTIONS
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // languages
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // select current lang
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter TO_DEV
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // confirm disabling
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // select current lang
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"go to language menus from INSERT");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_INSERT,
		"  insert screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  language menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to dev menu: confirm disabling os verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to dev menu: power_off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to dev menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  language menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  to dev menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* Navigate to all language menus from developer menu */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // select current lang
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enable OS verif
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // select current lang
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // return to dev_warn
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enable OS verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // show debug info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // developer options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // select current lang
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootDeveloperMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		" scroll through all language menus in developer options");
	TEST_EQ(debug_info_displayed, 0, "  no debug info");
	TEST_NEQ(shutdown_request_calls_left, 0, "  powered down explicitly");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  language menu: select current language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: cancel ");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  language menu: select current language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: confirm enabling os verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to norm screen: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  warning screen: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  select developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: boot developer image");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  language menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: boot from disk");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  developer menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

  	printf("...done.\n");
}

static void VbNavigationTest(void)
{
	int i;

	printf("Testing long navigation sequences...");

	/*
	 * NOGOOD, OPTIONS, LANGUAGE, TODEV, LANGUAGE, TODEV,
	 * LANGUAGE, select, Cancel, OPTIONS, LANGUAGE
	 */
	ResetMocksForManualRecovery();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter OPTIONS
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter TO_DEV
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // confirm enabling
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // (end of menu)
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS; // enter TO_DEV
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS; // select current lang
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // (end of menu)
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // (end of menu)
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // power off
	mock_keyflags[i] = VB_KEY_FLAG_TRUSTED_KEYBOARD;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // cancel
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // enter OPTIONS
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // show debug info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // power off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // language
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	TEST_EQ(VbBootRecoveryMenu(&ctx), VBERROR_SHUTDOWN_REQUESTED,
		"recovery mode long navigation");
	TEST_EQ(debug_info_displayed, 1, "  showed debug info");
	TEST_EQ(shutdown_request_calls_left, 0, "  timed out");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  languages menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: confirm enabling");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: confirm enabling");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  languages menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  languages menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_TO_DEV_MENU,
		"  todev: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_RECOVERY_NO_GOOD,
		"  nogood screen");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_OPTIONS_MENU,
		"  options: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_LANGUAGES_MENU,
		"  languages menu");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 0, "  no beeps");

	/* DEVELOPER, Cancel, Show Debug, TO_NORM, Cancel, Boot Legacy */
	ResetMocksForDeveloper();
	i = 0;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot From USB
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot Legacy
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Boot From USB
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Boot From Disk
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Cancel
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Enable OS verif
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Power Off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Language
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Power Off
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Cancel
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Enable OS verif
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Show Debug Info
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Developer Options
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Cancel
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Power Off
	mock_keypress[i++] = VB_BUTTON_VOL_DOWN_SHORT_PRESS; // Language
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Power Off
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Cancel
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot From Disk
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot From USB
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // Boot Legacy
	mock_keypress[i++] = VB_BUTTON_VOL_UP_SHORT_PRESS; // (end of menu)
	mock_keypress[i++] = VB_BUTTON_POWER_SHORT_PRESS;
	vb2_nv_set(&ctx, VB2_NV_DEV_BOOT_LEGACY, 1);
	TEST_EQ(VbBootDeveloperMenu(&ctx), vbtlk_retval_fixed,
		"developer mode long navigation");
	TEST_EQ(debug_info_displayed, 1, "  showed debug info");
	TEST_EQ(vbexlegacy_called, 1, "  tried legacy");
	TEST_EQ(audio_looping_calls_left, 0, "  audio timeout");
	TEST_EQ(vb2_nv_get(&ctx, VB2_NV_RECOVERY_REQUEST), 0, "  no recovery");
	i = 0;
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: Legacy boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: Legacy boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: USB boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: disk boot");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: confirm enabling");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_TO_NORM_MENU,
		"  to dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: enable root verification");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: show debug info");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_WARNING_MENU,
		"  dev warning menu: developer options");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: boot from disk");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: language");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: power off");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: cancel");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: boot from disk");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: boot from USB");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: boot legacy");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: boot legacy");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK, "  blank (flash)");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_DEVELOPER_MENU,
		"  dev menu: boot legacy");
	TEST_EQ(screens_displayed[i++], VB_SCREEN_BLANK,"  final blank screen");
	TEST_EQ(screens_count, i, "  no extra screens");
	TEST_EQ(beeps_count, 1, "  error beep: legacy BIOS not found");
	TEST_EQ(beeps_played[0], 200, "    low-frequency error beep");
}

int main(void)
{
	VbBootTest();
	VbBootDevTest();
	VbBootRecTest();
	VbTestLanguageMenu();
	VbNavigationTest();

	return gTestSuccess ? 0 : 255;
}
