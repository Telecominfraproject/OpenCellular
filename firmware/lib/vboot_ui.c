/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - user interface for RW firmware
 */

#include "sysincludes.h"

#include "2sysincludes.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "ec_sync.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "region.h"
#include "rollback_index.h"
#include "utility.h"
#include "vb2_common.h"
#include "vboot_api.h"
#include "vboot_audio.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"
#include "vboot_nvstorage.h"

static void VbAllowUsbBoot(struct vb2_context *ctx)
{
	VB2_DEBUG(".");
	vb2_nv_set(ctx, VB2_NV_DEV_BOOT_USB, 1);
}

/**
 * Checks GBB flags against VbExIsShutdownRequested() shutdown request to
 * determine if a shutdown is required.
 *
 * Returns true if a shutdown is required and false if no shutdown is required.
 */
static int VbWantShutdown(uint32_t gbb_flags, uint32_t key)
{
	uint32_t shutdown_request = VbExIsShutdownRequested();

	if (key == VB_BUTTON_POWER_SHORT_PRESS)
		shutdown_request |= VB_SHUTDOWN_REQUEST_POWER_BUTTON;

	/* If desired, ignore shutdown request due to lid closure. */
	if (gbb_flags & GBB_FLAG_DISABLE_LID_SHUTDOWN)
		shutdown_request &= ~VB_SHUTDOWN_REQUEST_LID_CLOSED;

	return !!shutdown_request;
}

static void VbTryLegacy(int allowed)
{
	if (!allowed)
		VB2_DEBUG("VbBootDeveloper() - Legacy boot is disabled\n");
	else if (0 != RollbackKernelLock(0))
		VB2_DEBUG("Error locking kernel versions on legacy boot.\n");
	else
		VbExLegacy();	/* will not return if successful */

	/* If legacy boot fails, beep and return to calling UI loop. */
	VbExBeep(120, 400);
	VbExSleepMs(120);
	VbExBeep(120, 400);
}

uint32_t VbTryUsb(struct vb2_context *ctx, VbCommonParams *cparams)
{
	uint32_t retval = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_REMOVABLE);
	if (VBERROR_SUCCESS == retval) {
		VB2_DEBUG("VbBootDeveloper() - booting USB\n");
	} else {
		VB2_DEBUG("VbBootDeveloper() - no kernel found on USB\n");
		VbExBeep(250, 200);
		VbExSleepMs(120);
		/*
		 * Clear recovery requests from failed
		 * kernel loading, so that powering off
		 * at this point doesn't put us into
		 * recovery mode.
		 */
		vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST,
			   VBNV_RECOVERY_NOT_REQUESTED);
	}
	return retval;
}

#define CONFIRM_KEY_DELAY 20  /* Check confirm screen keys every 20ms */

int VbUserConfirms(struct vb2_context *ctx, VbCommonParams *cparams,
		   uint32_t confirm_flags)
{
	VbSharedDataHeader *shared =
			(VbSharedDataHeader *)cparams->shared_data_blob;
	uint32_t key;
	uint32_t key_flags;
	uint32_t btn;
	int rec_button_was_pressed = 0;

	VB2_DEBUG("Entering(%x)\n", confirm_flags);

	/* Await further instructions */
	while (1) {
		key = VbExKeyboardReadWithFlags(&key_flags);
		if (VbWantShutdown(cparams->gbb->flags, key))
			return -1;
		switch (key) {
		case '\r':
			/* If we require a trusted keyboard for confirmation,
			 * but the keyboard may be faked (for instance, a USB
			 * device), beep and keep waiting.
			 */
			if (confirm_flags & VB_CONFIRM_MUST_TRUST_KEYBOARD &&
			    !(key_flags & VB_KEY_FLAG_TRUSTED_KEYBOARD)) {
				VbExBeep(120, 400);
				break;
			}
			VB2_DEBUG("Yes (1)\n");
			return 1;
			break;
		case ' ':
			VB2_DEBUG("Space (%d)\n",
				  confirm_flags & VB_CONFIRM_SPACE_MEANS_NO);
			if (confirm_flags & VB_CONFIRM_SPACE_MEANS_NO)
				return 0;
			break;
		case 0x1b:
			VB2_DEBUG("No (0)\n");
			return 0;
			break;
		default:
			/* If the recovery button is physical, and is pressed,
			 * this is also a YES, but must wait for release.
			 */
			btn = VbExGetSwitches(VB_INIT_FLAG_REC_BUTTON_PRESSED);
			if (!(shared->flags & VBSD_BOOT_REC_SWITCH_VIRTUAL)) {
				if (btn) {
					VB2_DEBUG("Rec button pressed\n");
					rec_button_was_pressed = 1;
				} else if (rec_button_was_pressed) {
					VB2_DEBUG("Rec button (1)\n");
					return 1;
				}
			}
			VbCheckDisplayKey(ctx, cparams, key);
		}
		VbExSleepMs(CONFIRM_KEY_DELAY);
	}

	/* Not reached, but compiler will complain without it */
	return -1;
}

static const char dev_disable_msg[] =
	"Developer mode is disabled on this device by system policy.\n"
	"For more information, see http://dev.chromium.org/chromium-os/fwmp\n"
	"\n";

VbError_t vb2_developer_ui(struct vb2_context *ctx, VbCommonParams *cparams)
{
	GoogleBinaryBlockHeader *gbb = cparams->gbb;
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	uint32_t disable_dev_boot = 0;
	uint32_t use_usb = 0;
	uint32_t use_legacy = 0;
	uint32_t ctrl_d_pressed = 0;

	VbAudioContext *audio = 0;

	VB2_DEBUG("Entering\n");

	/* Check if USB booting is allowed */
	uint32_t allow_usb = vb2_nv_get(ctx, VB2_NV_DEV_BOOT_USB);
	uint32_t allow_legacy = vb2_nv_get(ctx, VB2_NV_DEV_BOOT_LEGACY);

	/* Check if the default is to boot using disk, usb, or legacy */
	uint32_t default_boot = vb2_nv_get(ctx, VB2_NV_DEV_DEFAULT_BOOT);

	if(default_boot == VBNV_DEV_DEFAULT_BOOT_USB)
		use_usb = 1;
	if(default_boot == VBNV_DEV_DEFAULT_BOOT_LEGACY)
		use_legacy = 1;

	/* Handle GBB flag override */
	if (gbb->flags & GBB_FLAG_FORCE_DEV_BOOT_USB)
		allow_usb = 1;
	if (gbb->flags & GBB_FLAG_FORCE_DEV_BOOT_LEGACY)
		allow_legacy = 1;
	if (gbb->flags & GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY) {
		use_legacy = 1;
		use_usb = 0;
	}

	/* Handle FWMP override */
	uint32_t fwmp_flags = vb2_get_fwmp_flags();
	if (fwmp_flags & FWMP_DEV_ENABLE_USB)
		allow_usb = 1;
	if (fwmp_flags & FWMP_DEV_ENABLE_LEGACY)
		allow_legacy = 1;
	if (fwmp_flags & FWMP_DEV_DISABLE_BOOT) {
		if (gbb->flags & GBB_FLAG_FORCE_DEV_SWITCH_ON) {
			VB2_DEBUG("FWMP_DEV_DISABLE_BOOT rejected by "
				  "FORCE_DEV_SWITCH_ON\n");
		} else {
			disable_dev_boot = 1;
		}
	}

	/* If dev mode is disabled, only allow TONORM */
	while (disable_dev_boot) {
		VB2_DEBUG("dev_disable_boot is set\n");
		VbDisplayScreen(ctx, cparams, VB_SCREEN_DEVELOPER_TO_NORM, 0);
		VbExDisplayDebugInfo(dev_disable_msg);

		/* Ignore space in VbUserConfirms()... */
		switch (VbUserConfirms(ctx, cparams, 0)) {
		case 1:
			VB2_DEBUG("leaving dev-mode\n");
			vb2_nv_set(ctx, VB2_NV_DISABLE_DEV_REQUEST, 1);
			VbDisplayScreen(ctx, cparams,
					VB_SCREEN_TO_NORM_CONFIRMED,
					0);
			VbExSleepMs(5000);
			return VBERROR_REBOOT_REQUIRED;
		case -1:
			VB2_DEBUG("shutdown requested\n");
			return VBERROR_SHUTDOWN_REQUESTED;
		default:
			/* Ignore user attempt to cancel */
			VB2_DEBUG("ignore cancel TONORM\n");
		}
	}

	/* Show the dev mode warning screen */
	VbDisplayScreen(ctx, cparams, VB_SCREEN_DEVELOPER_WARNING, 0);

	/* Get audio/delay context */
	audio = VbAudioOpen(cparams);

	/* We'll loop until we finish the delay or are interrupted */
	do {
		uint32_t key = VbExKeyboardRead();
		if (VbWantShutdown(gbb->flags, key)) {
			VB2_DEBUG("VbBootDeveloper() - shutdown requested!\n");
			VbAudioClose(audio);
			return VBERROR_SHUTDOWN_REQUESTED;
		}

		switch (key) {
		case 0:
			/* nothing pressed */
			break;
		case '\r':
			/* Only disable virtual dev switch if allowed by GBB */
			if (!(gbb->flags & GBB_FLAG_ENTER_TRIGGERS_TONORM))
				break;
		case ' ':
			/* See if we should disable virtual dev-mode switch. */
			VB2_DEBUG("shared->flags=0x%x\n", shared->flags);
			if (shared->flags & VBSD_HONOR_VIRT_DEV_SWITCH &&
			    shared->flags & VBSD_BOOT_DEV_SWITCH_ON) {
				/* Stop the countdown while we go ask... */
				VbAudioClose(audio);
				if (gbb->flags & GBB_FLAG_FORCE_DEV_SWITCH_ON) {
					/*
					 * TONORM won't work (only for
					 * non-shipping devices).
					 */
					VB2_DEBUG("TONORM rejected by "
						  "FORCE_DEV_SWITCH_ON\n");
					VbExDisplayDebugInfo(
						"WARNING: TONORM prohibited by "
						"GBB FORCE_DEV_SWITCH_ON.\n\n");
					VbExBeep(120, 400);
					break;
				}
				VbDisplayScreen(ctx, cparams,
						VB_SCREEN_DEVELOPER_TO_NORM,
						0);
				/* Ignore space in VbUserConfirms()... */
				switch (VbUserConfirms(ctx, cparams, 0)) {
				case 1:
					VB2_DEBUG("leaving dev-mode\n");
					vb2_nv_set(ctx, VB2_NV_DISABLE_DEV_REQUEST,
						1);
					VbDisplayScreen(ctx,
						cparams,
						VB_SCREEN_TO_NORM_CONFIRMED,
						0);
					VbExSleepMs(5000);
					return VBERROR_REBOOT_REQUIRED;
				case -1:
					VB2_DEBUG("shutdown requested\n");
					return VBERROR_SHUTDOWN_REQUESTED;
				default:
					/* Stay in dev-mode */
					VB2_DEBUG("stay in dev-mode\n");
					VbDisplayScreen(ctx,
						cparams,
						VB_SCREEN_DEVELOPER_WARNING,
						0);
					/* Start new countdown */
					audio = VbAudioOpen(cparams);
				}
			} else {
				/*
				 * No virtual dev-mode switch, so go directly
				 * to recovery mode.
				 */
				VB2_DEBUG("going to recovery\n");
				vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST,
					   VBNV_RECOVERY_RW_DEV_SCREEN);
				VbAudioClose(audio);
				return VBERROR_LOAD_KERNEL_RECOVERY;
			}
			break;
		case 0x04:
			/* Ctrl+D = dismiss warning; advance to timeout */
			VB2_DEBUG("VbBootDeveloper() - "
				  "user pressed Ctrl+D; skip delay\n");
			ctrl_d_pressed = 1;
			goto fallout;
			break;
		case 0x0c:
			VB2_DEBUG("VbBootDeveloper() - "
				  "user pressed Ctrl+L; Try legacy boot\n");
			VbTryLegacy(allow_legacy);
			break;

		case VB_KEY_CTRL_ENTER:
			/*
			 * The Ctrl-Enter is special for Lumpy test purpose;
			 * fall through to Ctrl+U handler.
			 */
		case 0x15:
			/* Ctrl+U = try USB boot, or beep if failure */
			VB2_DEBUG("VbBootDeveloper() - "
				  "user pressed Ctrl+U; try USB\n");
			if (!allow_usb) {
				VB2_DEBUG("VbBootDeveloper() - "
					  "USB booting is disabled\n");
				VbExDisplayDebugInfo(
					"WARNING: Booting from external media "
					"(USB/SD) has not been enabled. Refer "
					"to the developer-mode documentation "
					"for details.\n");
				VbExBeep(120, 400);
				VbExSleepMs(120);
				VbExBeep(120, 400);
			} else {
				/*
				 * Clear the screen to show we get the Ctrl+U
				 * key press.
				 */
				VbDisplayScreen(ctx, cparams, VB_SCREEN_BLANK,
						0);
				if (VBERROR_SUCCESS ==
				    VbTryUsb(ctx, cparams)) {
					VbAudioClose(audio);
					return VBERROR_SUCCESS;
				} else {
					/* Show dev mode warning screen again */
					VbDisplayScreen(ctx,
						cparams,
						VB_SCREEN_DEVELOPER_WARNING,
						0);
				}
			}
			break;
		default:
			VB2_DEBUG("VbBootDeveloper() - pressed key %d\n", key);
			VbCheckDisplayKey(ctx, cparams, key);
			break;
		}
	} while(VbAudioLooping(audio));

 fallout:

	/* If defaulting to legacy boot, try that unless Ctrl+D was pressed */
	if (use_legacy && !ctrl_d_pressed) {
		VB2_DEBUG("VbBootDeveloper() - defaulting to legacy\n");
		VbTryLegacy(allow_legacy);
	}

	if ((use_usb && !ctrl_d_pressed) && allow_usb) {
		if (VBERROR_SUCCESS == VbTryUsb(ctx, cparams)) {
			VbAudioClose(audio);
			return VBERROR_SUCCESS;
		}
	}

	/* Timeout or Ctrl+D; attempt loading from fixed disk */
	VB2_DEBUG("VbBootDeveloper() - trying fixed disk\n");
	VbAudioClose(audio);
	return VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_FIXED);
}

VbError_t VbBootDeveloper(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbError_t retval = vb2_developer_ui(ctx, cparams);
	VbDisplayScreen(ctx, cparams, VB_SCREEN_BLANK, 0);
	return retval;
}

/* Delay in recovery mode */
#define REC_DISK_DELAY       1000     /* Check disks every 1s */
#define REC_KEY_DELAY        20       /* Check keys every 20ms */
#define REC_MEDIA_INIT_DELAY 500      /* Check removable media every 500ms */

static VbError_t recovery_ui(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	uint32_t retval;
	uint32_t key;
	int i;

	VB2_DEBUG("VbBootRecovery() start\n");

	if (!vb2_allow_recovery(shared->flags)) {
		/*
		 * We have to save the reason here so that it will survive
		 * coming up three-finger-salute. We're saving it in
		 * VBNV_RECOVERY_SUBCODE to avoid a recovery loop.
		 * If we save the reason in VBNV_RECOVERY_REQUEST, we will come
		 * back here, thus, we won't be able to give a user a chance to
		 * reboot to workaround a boot hiccup.
		 */
		VB2_DEBUG("VbBootRecovery() saving recovery reason (%#x)\n",
			 shared->recovery_reason);
		vb2_nv_set(ctx, VB2_NV_RECOVERY_SUBCODE,
			   shared->recovery_reason);
		/*
		 * Commit NV now, because it won't get saved if the user forces
		 * manual recovery via the three-finger salute.
		 */
		vb2_nv_commit(ctx);

		VbDisplayScreen(ctx, cparams, VB_SCREEN_OS_BROKEN, 0);
		VB2_DEBUG("VbBootRecovery() waiting for manual recovery\n");
		while (1) {
			key = VbExKeyboardRead();
			VbCheckDisplayKey(ctx, cparams, key);
			if (VbWantShutdown(cparams->gbb->flags, key))
				return VBERROR_SHUTDOWN_REQUESTED;
			VbExSleepMs(REC_KEY_DELAY);
		}
	}

	/* Loop and wait for a recovery image */
	VB2_DEBUG("VbBootRecovery() waiting for a recovery image\n");
	while (1) {
		VB2_DEBUG("VbBootRecovery() attempting to load kernel2\n");
		retval = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_REMOVABLE);

		/*
		 * Clear recovery requests from failed kernel loading, since
		 * we're already in recovery mode.  Do this now, so that
		 * powering off after inserting an invalid disk doesn't leave
		 * us stuck in recovery mode.
		 */
		vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST,
			   VBNV_RECOVERY_NOT_REQUESTED);

		if (VBERROR_SUCCESS == retval)
			break; /* Found a recovery kernel */

		VbDisplayScreen(ctx, cparams, VBERROR_NO_DISK_FOUND == retval ?
				VB_SCREEN_RECOVERY_INSERT :
				VB_SCREEN_RECOVERY_NO_GOOD,
				0);

		/*
		 * Scan keyboard more frequently than media, since x86
		 * platforms don't like to scan USB too rapidly.
		 */
		for (i = 0; i < REC_DISK_DELAY; i += REC_KEY_DELAY) {
			key = VbExKeyboardRead();
			/*
			 * We might want to enter dev-mode from the Insert
			 * screen if all of the following are true:
			 *   - user pressed Ctrl-D
			 *   - we can honor the virtual dev switch
			 *   - not already in dev mode
			 *   - user forced recovery mode
			 */
			if (key == 0x04 &&
			    shared->flags & VBSD_HONOR_VIRT_DEV_SWITCH &&
			    !(shared->flags & VBSD_BOOT_DEV_SWITCH_ON) &&
			    (shared->flags & VBSD_BOOT_REC_SWITCH_ON)) {
                                if (!(shared->flags &
				      VBSD_BOOT_REC_SWITCH_VIRTUAL) &&
				    VbExGetSwitches(
					     VB_INIT_FLAG_REC_BUTTON_PRESSED)) {
					/*
					 * Is the recovery button stuck?  In
					 * any case we don't like this.  Beep
					 * and ignore.
					 */
					VB2_DEBUG("^D but rec switch "
						  "is pressed\n");
					VbExBeep(120, 400);
					continue;
				}

				/* Ask the user to confirm entering dev-mode */
				VbDisplayScreen(ctx, cparams,
						VB_SCREEN_RECOVERY_TO_DEV,
						0);
				/* SPACE means no... */
				uint32_t vbc_flags =
					VB_CONFIRM_SPACE_MEANS_NO |
					VB_CONFIRM_MUST_TRUST_KEYBOARD;
				switch (VbUserConfirms(ctx, cparams,
						       vbc_flags)) {
				case 1:
					VB2_DEBUG("Enabling dev-mode...\n");
					if (TPM_SUCCESS != SetVirtualDevMode(1))
						return VBERROR_TPM_SET_BOOT_MODE_STATE;
					VB2_DEBUG("Reboot so it will take "
						  "effect\n");
					if (VbExGetSwitches
					    (VB_INIT_FLAG_ALLOW_USB_BOOT))
						VbAllowUsbBoot(ctx);
					return VBERROR_REBOOT_REQUIRED;
				case -1:
					VB2_DEBUG("Shutdown requested\n");
					return VBERROR_SHUTDOWN_REQUESTED;
				default: /* zero, actually */
					VB2_DEBUG("Not enabling dev-mode\n");
					/*
					 * Jump out of the outer loop to
					 * refresh the display quickly.
					 */
					i = 4;
					break;
				}
			} else {
				VbCheckDisplayKey(ctx, cparams, key);
			}
			if (VbWantShutdown(cparams->gbb->flags, key))
				return VBERROR_SHUTDOWN_REQUESTED;
			VbExSleepMs(REC_KEY_DELAY);
		}
	}

	return VBERROR_SUCCESS;
}

VbError_t VbBootRecovery(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbError_t retval = recovery_ui(ctx, cparams);
	VbDisplayScreen(ctx, cparams, VB_SCREEN_BLANK, 0);
	return retval;
}
