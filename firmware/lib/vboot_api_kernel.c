/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for kernel selection
 */

#include "sysincludes.h"

#include "gbb_access.h"
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "region.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_audio.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"
#include "vboot_nvstorage.h"

/* Global variables */
static VbNvContext vnc;

#ifdef CHROMEOS_ENVIRONMENT
/* Global variable accessor for unit tests */

VbNvContext *VbApiKernelGetVnc(void)
{
	return &vnc;
}
#endif

/**
 * Set recovery request (called from vboot_api_kernel.c functions only)
 */
static void VbSetRecoveryRequest(uint32_t recovery_request)
{
	VBDEBUG(("VbSetRecoveryRequest(%d)\n", (int)recovery_request));
	VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, recovery_request);
}

/**
 * Checks GBB flags against VbExIsShutdownRequested() shutdown request to
 * determine if a shutdown is required.
 *
 * Returns true if a shutdown is required and false if no shutdown is required.
 */
static int VbWantShutdown(uint32_t gbb_flags)
{
	uint32_t shutdown_request = VbExIsShutdownRequested();

	/* If desired, ignore shutdown request due to lid closure. */
	if (gbb_flags & GBB_FLAG_DISABLE_LID_SHUTDOWN)
		shutdown_request &= ~VB_SHUTDOWN_REQUEST_LID_CLOSED;

	return !!shutdown_request;
}

/**
 * Attempt loading a kernel from the specified type(s) of disks.
 *
 * If successful, sets p->disk_handle to the disk for the kernel and returns
 * VBERROR_SUCCESS.
 *
 * Returns VBERROR_NO_DISK_FOUND if no disks of the specified type were found.
 *
 * May return other VBERROR_ codes for other failures.
 */
uint32_t VbTryLoadKernel(VbCommonParams *cparams, LoadKernelParams *p,
                         uint32_t get_info_flags)
{
	VbError_t retval = VBERROR_UNKNOWN;
	VbDiskInfo* disk_info = NULL;
	uint32_t disk_count = 0;
	uint32_t i;

	VBDEBUG(("VbTryLoadKernel() start, get_info_flags=0x%x\n",
		 (unsigned)get_info_flags));

	p->disk_handle = NULL;

	/* Find disks */
	if (VBERROR_SUCCESS != VbExDiskGetInfo(&disk_info, &disk_count,
					       get_info_flags))
		disk_count = 0;

	VBDEBUG(("VbTryLoadKernel() found %d disks\n", (int)disk_count));
	if (0 == disk_count) {
		VbSetRecoveryRequest(VBNV_RECOVERY_RW_NO_DISK);
		return VBERROR_NO_DISK_FOUND;
	}

	/* Loop over disks */
	for (i = 0; i < disk_count; i++) {
		VBDEBUG(("VbTryLoadKernel() trying disk %d\n", (int)i));
		/*
		 * Sanity-check what we can. FWIW, VbTryLoadKernel() is always
		 * called with only a single bit set in get_info_flags.
		 *
		 * Ensure 512-byte sectors and non-trivially sized disk (for
		 * cgptlib) and that we got a partition with only the flags we
		 * asked for.
		 */
		if (512 != disk_info[i].bytes_per_lba ||
		    16 > disk_info[i].lba_count ||
		    get_info_flags != (disk_info[i].flags & ~VB_DISK_FLAG_EXTERNAL_GPT)) {
			VBDEBUG(("  skipping: bytes_per_lba=%" PRIu64
				 " lba_count=%" PRIu64 " flags=0x%x\n",
				 disk_info[i].bytes_per_lba,
				 disk_info[i].lba_count,
				 disk_info[i].flags));
			continue;
		}
		p->disk_handle = disk_info[i].handle;
		p->bytes_per_lba = disk_info[i].bytes_per_lba;
		p->gpt_lba_count = disk_info[i].lba_count;
		p->streaming_lba_count = disk_info[i].streaming_lba_count
						?: p->gpt_lba_count;
		p->boot_flags |= disk_info[i].flags & VB_DISK_FLAG_EXTERNAL_GPT
				? BOOT_FLAG_EXTERNAL_GPT : 0;
		retval = LoadKernel(p, cparams);
		VBDEBUG(("VbTryLoadKernel() LoadKernel() = %d\n", retval));

		/*
		 * Stop now if we found a kernel.
		 *
		 * TODO: If recovery requested, should track the farthest we
		 * get, instead of just returning the value from the last disk
		 * attempted.
		 */
		if (VBERROR_SUCCESS == retval)
			break;
	}

	/* If we didn't find any good kernels, don't return a disk handle. */
	if (VBERROR_SUCCESS != retval) {
		VbSetRecoveryRequest(VBNV_RECOVERY_RW_NO_KERNEL);
		p->disk_handle = NULL;
	}

	VbExDiskFreeInfo(disk_info, p->disk_handle);

	/*
	 * Pass through return code.  Recovery reason (if any) has already been
	 * set by LoadKernel().
	 */
	return retval;
}

#define CONFIRM_KEY_DELAY 20  /* Check confirm screen keys every 20ms */

int VbUserConfirms(VbCommonParams *cparams, uint32_t confirm_flags)
{
	VbSharedDataHeader *shared =
           (VbSharedDataHeader *)cparams->shared_data_blob;
	uint32_t key;
	uint32_t key_flags;
        uint32_t button;
	int rec_button_was_pressed = 0;

	VBDEBUG(("Entering %s(0x%x)\n", __func__, confirm_flags));

	/* Await further instructions */
	while (1) {
		if (VbWantShutdown(cparams->gbb->flags))
			return -1;
		key = VbExKeyboardReadWithFlags(&key_flags);
                button = VbExGetSwitches(VB_INIT_FLAG_REC_BUTTON_PRESSED);
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

			VBDEBUG(("%s() - Yes (1)\n", __func__));
			return 1;
			break;
		case ' ':
			VBDEBUG(("%s() - Space (%d)\n", __func__,
				 confirm_flags & VB_CONFIRM_SPACE_MEANS_NO));
			if (confirm_flags & VB_CONFIRM_SPACE_MEANS_NO)
				return 0;
			break;
		case 0x1b:
			VBDEBUG(("%s() - No (0)\n", __func__));
			return 0;
			break;
		default:
			/* If the recovery button is physical, and is pressed,
			 * this is also a YES, but must wait for release.
			 */
			if (!(shared->flags & VBSD_BOOT_REC_SWITCH_VIRTUAL)) {
				if (button) {
					VBDEBUG(("%s() - Rec button pressed\n",
						 __func__));
	                                rec_button_was_pressed = 1;
				} else if (rec_button_was_pressed) {
					VBDEBUG(("%s() - Rec button (1)\n",
					 __func__));
					return 1;
				}
			}
			VbCheckDisplayKey(cparams, key, &vnc);
		}
		VbExSleepMs(CONFIRM_KEY_DELAY);
	}

	/* Not reached, but compiler will complain without it */
	return -1;
}

VbError_t VbBootNormal(VbCommonParams *cparams, LoadKernelParams *p)
{
	/* Boot from fixed disk only */
	VBDEBUG(("Entering %s()\n", __func__));
	return VbTryLoadKernel(cparams, p, VB_DISK_FLAG_FIXED);
}

VbError_t VbBootDeveloper(VbCommonParams *cparams, LoadKernelParams *p)
{
	GoogleBinaryBlockHeader *gbb = cparams->gbb;
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	uint32_t allow_usb = 0, allow_legacy = 0, ctrl_d_pressed = 0;
	VbAudioContext *audio = 0;

	VBDEBUG(("Entering %s()\n", __func__));

	/* Check if USB booting is allowed */
	VbNvGet(&vnc, VBNV_DEV_BOOT_USB, &allow_usb);
	VbNvGet(&vnc, VBNV_DEV_BOOT_LEGACY, &allow_legacy);

	/* Handle GBB flag override */
	if (gbb->flags & GBB_FLAG_FORCE_DEV_BOOT_USB)
		allow_usb = 1;
	if (gbb->flags & GBB_FLAG_FORCE_DEV_BOOT_LEGACY)
		allow_legacy = 1;

	/* Show the dev mode warning screen */
	VbDisplayScreen(cparams, VB_SCREEN_DEVELOPER_WARNING, 0, &vnc);

	/* Get audio/delay context */
	audio = VbAudioOpen(cparams);

	/* We'll loop until we finish the delay or are interrupted */
	do {
		uint32_t key;

		if (VbWantShutdown(gbb->flags)) {
			VBDEBUG(("VbBootDeveloper() - shutdown requested!\n"));
			VbAudioClose(audio);
			return VBERROR_SHUTDOWN_REQUESTED;
		}

		key = VbExKeyboardRead();
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
			VBDEBUG(("%s shared->flags=0x%x\n",
				 __func__, shared->flags));
			if (shared->flags & VBSD_HONOR_VIRT_DEV_SWITCH &&
			    shared->flags & VBSD_BOOT_DEV_SWITCH_ON) {
				/* Stop the countdown while we go ask... */
				VbAudioClose(audio);
				if (gbb->flags & GBB_FLAG_FORCE_DEV_SWITCH_ON) {
					/*
					 * TONORM won't work (only for
					 * non-shipping devices).
					 */
					VBDEBUG(("%s() - TONORM rejected by "
						 "FORCE_DEV_SWITCH_ON\n",
						 __func__));
					VbExDisplayDebugInfo(
						"WARNING: TONORM prohibited by "
						"GBB FORCE_DEV_SWITCH_ON.\n\n");
					VbExBeep(120, 400);
					break;
				}
				VbDisplayScreen(cparams,
						VB_SCREEN_DEVELOPER_TO_NORM,
						0, &vnc);
				/* Ignore space in VbUserConfirms()... */
				switch (VbUserConfirms(cparams, 0)) {
				case 1:
					VBDEBUG(("%s() - leaving dev-mode.\n",
						 __func__));
					VbNvSet(&vnc, VBNV_DISABLE_DEV_REQUEST,
						1);
					VbDisplayScreen(
						cparams,
						VB_SCREEN_TO_NORM_CONFIRMED,
						0, &vnc);
					VbExSleepMs(5000);
					return VBERROR_TPM_REBOOT_REQUIRED;
				case -1:
					VBDEBUG(("%s() - shutdown requested\n",
						 __func__));
					return VBERROR_SHUTDOWN_REQUESTED;
				default:
					/* Stay in dev-mode */
					VBDEBUG(("%s() - stay in dev-mode\n",
						 __func__));
					VbDisplayScreen(
						cparams,
						VB_SCREEN_DEVELOPER_WARNING,
						0, &vnc);
					/* Start new countdown */
					audio = VbAudioOpen(cparams);
				}
			} else {
				/*
				 * No virtual dev-mode switch, so go directly
				 * to recovery mode.
				 */
				VBDEBUG(("%s() - going to recovery\n",
					 __func__));
				VbSetRecoveryRequest(
					VBNV_RECOVERY_RW_DEV_SCREEN);
				VbAudioClose(audio);
				return VBERROR_LOAD_KERNEL_RECOVERY;
			}
			break;
		case 0x04:
			/* Ctrl+D = dismiss warning; advance to timeout */
			VBDEBUG(("VbBootDeveloper() - "
				 "user pressed Ctrl+D; skip delay\n"));
			ctrl_d_pressed = 1;
			goto fallout;
			break;
		case 0x0c:
			VBDEBUG(("VbBootDeveloper() - "
				 "user pressed Ctrl+L; Try legacy boot\n"));
			/*
			 * If VbExLegacy() succeeds, it will never return.  If
			 * it returns, beep.
			 */
			if (allow_legacy)
				VbExLegacy();
			else
				VBDEBUG(("VbBootDeveloper() - "
					 "Legacy boot is disabled\n"));

			VbExBeep(120, 400);
			VbExSleepMs(120);
			VbExBeep(120, 400);
			break;

		case VB_KEY_CTRL_ENTER:
			/*
			 * The Ctrl-Enter is special for Lumpy test purpose;
			 * fall through to Ctrl+U handler.
			 */
		case 0x15:
			/* Ctrl+U = try USB boot, or beep if failure */
			VBDEBUG(("VbBootDeveloper() - "
				 "user pressed Ctrl+U; try USB\n"));
			if (!allow_usb) {
				VBDEBUG(("VbBootDeveloper() - "
					 "USB booting is disabled\n"));
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
				VbDisplayScreen(cparams, VB_SCREEN_BLANK, 0,
						&vnc);
				if (VBERROR_SUCCESS ==
				    VbTryLoadKernel(cparams, p,
						    VB_DISK_FLAG_REMOVABLE)) {
					VBDEBUG(("VbBootDeveloper() - "
						 "booting USB\n"));
					VbAudioClose(audio);
					return VBERROR_SUCCESS;
				} else {
					VBDEBUG(("VbBootDeveloper() - "
						 "no kernel found on USB\n"));
					VbExBeep(250, 200);
					VbExSleepMs(120);
					/*
					 * Clear recovery requests from failed
					 * kernel loading, so that powering off
					 * at this point doesn't put us into
					 * recovery mode.
					 */
					VbSetRecoveryRequest(
						VBNV_RECOVERY_NOT_REQUESTED);
					/* Show dev mode warning screen again */
					VbDisplayScreen(
						cparams,
						VB_SCREEN_DEVELOPER_WARNING,
						0, &vnc);
				}
			}
			break;
		default:
			VBDEBUG(("VbBootDeveloper() - pressed key %d\n", key));
			VbCheckDisplayKey(cparams, key, &vnc);
			break;
		}
	} while(VbAudioLooping(audio));

 fallout:

	/* If defaulting to legacy boot, try that unless Ctrl+D was pressed */
	if ((gbb->flags & GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY) &&
	    !ctrl_d_pressed) {
		VBDEBUG(("VbBootDeveloper() - defaulting to legacy\n"));
		VbExLegacy();

		/* If that fails, beep and fall through to fixed disk */
		VbExBeep(120, 400);
		VbExSleepMs(120);
		VbExBeep(120, 400);
	}

	/* Timeout or Ctrl+D; attempt loading from fixed disk */
	VBDEBUG(("VbBootDeveloper() - trying fixed disk\n"));
	VbAudioClose(audio);
	return VbTryLoadKernel(cparams, p, VB_DISK_FLAG_FIXED);
}

/* Delay in recovery mode */
#define REC_DISK_DELAY       1000     /* Check disks every 1s */
#define REC_KEY_DELAY        20       /* Check keys every 20ms */
#define REC_MEDIA_INIT_DELAY 500      /* Check removable media every 500ms */

VbError_t VbBootRecovery(VbCommonParams *cparams, LoadKernelParams *p)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	uint32_t retval;
	uint32_t key;
	int i;

	VBDEBUG(("VbBootRecovery() start\n"));

	/*
	 * If the dev-mode switch is off and the user didn't press the recovery
	 * button, require removal of all external media.
	 */
	if (!(shared->flags & VBSD_BOOT_DEV_SWITCH_ON) &&
	    !(shared->flags & VBSD_BOOT_REC_SWITCH_ON)) {
		VbDiskInfo *disk_info = NULL;
		uint32_t disk_count = 0;

		VBDEBUG(("VbBootRecovery() forcing device removal\n"));

		/* If no media is detected initially, delay and make one extra
		 * attempt, in case devices appear later than expected. */
		if (VBERROR_SUCCESS != VbExDiskGetInfo(&disk_info, &disk_count,
						       VB_DISK_FLAG_REMOVABLE))
			disk_count = 0;

		VbExDiskFreeInfo(disk_info, NULL);
		if (0 == disk_count)
			VbExSleepMs(REC_MEDIA_INIT_DELAY);

		while (1) {
			disk_info = NULL;
			disk_count = 0;
			if (VBERROR_SUCCESS !=
			    VbExDiskGetInfo(&disk_info, &disk_count,
					    VB_DISK_FLAG_REMOVABLE))
				disk_count = 0;

			VbExDiskFreeInfo(disk_info, NULL);

			if (0 == disk_count) {
				VbDisplayScreen(cparams, VB_SCREEN_BLANK,
						0, &vnc);
				break;
			}

			VBDEBUG(("VbBootRecovery() "
				 "waiting for %d disks to be removed\n",
				 (int)disk_count));

			VbDisplayScreen(cparams, VB_SCREEN_RECOVERY_REMOVE,
					0, &vnc);

			/*
			 * Scan keyboard more frequently than media, since x86
			 * platforms don't like to scan USB too rapidly.
			 */
			for (i = 0; i < REC_DISK_DELAY; i += REC_KEY_DELAY) {
				VbCheckDisplayKey(cparams, VbExKeyboardRead(),
						  &vnc);
				if (VbWantShutdown(cparams->gbb->flags))
					return VBERROR_SHUTDOWN_REQUESTED;
				VbExSleepMs(REC_KEY_DELAY);
			}
		}
	}

	/* Loop and wait for a recovery image */
	while (1) {
		VBDEBUG(("VbBootRecovery() attempting to load kernel2\n"));
		retval = VbTryLoadKernel(cparams, p, VB_DISK_FLAG_REMOVABLE);

		/*
		 * Clear recovery requests from failed kernel loading, since
		 * we're already in recovery mode.  Do this now, so that
		 * powering off after inserting an invalid disk doesn't leave
		 * us stuck in recovery mode.
		 */
		VbSetRecoveryRequest(VBNV_RECOVERY_NOT_REQUESTED);

		if (VBERROR_SUCCESS == retval)
			break; /* Found a recovery kernel */

		VbDisplayScreen(cparams, VBERROR_NO_DISK_FOUND == retval ?
				VB_SCREEN_RECOVERY_INSERT :
				VB_SCREEN_RECOVERY_NO_GOOD,
				0, &vnc);

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
			 *   - EC isn't pwned
			 */
			if (key == 0x04 &&
			    shared->flags & VBSD_HONOR_VIRT_DEV_SWITCH &&
			    !(shared->flags & VBSD_BOOT_DEV_SWITCH_ON) &&
			    (shared->flags & VBSD_BOOT_REC_SWITCH_ON) &&
			    VbExTrustEC(0)) {
                                if (!(shared->flags &
				      VBSD_BOOT_REC_SWITCH_VIRTUAL) &&
				    VbExGetSwitches(
					     VB_INIT_FLAG_REC_BUTTON_PRESSED)) {
					/*
					 * Is the recovery button stuck?  In
					 * any case we don't like this.  Beep
					 * and ignore.
					 */
					VBDEBUG(("%s() - ^D but rec switch "
						 "is pressed\n", __func__));
					VbExBeep(120, 400);
					continue;
				}

				/* Ask the user to confirm entering dev-mode */
				VbDisplayScreen(cparams,
						VB_SCREEN_RECOVERY_TO_DEV,
						0, &vnc);
				/* SPACE means no... */
				uint32_t vbc_flags =
					VB_CONFIRM_SPACE_MEANS_NO |
					VB_CONFIRM_MUST_TRUST_KEYBOARD;
				switch (VbUserConfirms(cparams, vbc_flags)) {
				case 1:
					VBDEBUG(("%s() Enabling dev-mode...\n",
						 __func__));
					if (TPM_SUCCESS != SetVirtualDevMode(1))
						return VBERROR_TPM_SET_BOOT_MODE_STATE;
					VBDEBUG(("%s() Reboot so it will take "
						 "effect\n", __func__));
					return VBERROR_TPM_REBOOT_REQUIRED;
				case -1:
					VBDEBUG(("%s() - Shutdown requested\n",
						 __func__));
					return VBERROR_SHUTDOWN_REQUESTED;
				default: /* zero, actually */
					VBDEBUG(("%s() - Not enabling "
						 "dev-mode\n", __func__));
					/*
					 * Jump out of the outer loop to
					 * refresh the display quickly.
					 */
					i = 4;
					break;
				}
			} else {
				VbCheckDisplayKey(cparams, key, &vnc);
			}
			if (VbWantShutdown(cparams->gbb->flags))
				return VBERROR_SHUTDOWN_REQUESTED;
			VbExSleepMs(REC_KEY_DELAY);
		}
	}

	return VBERROR_SUCCESS;
}

/**
 * Wrapper around VbExEcProtectRW() which sets recovery reason on error.
 */
static VbError_t EcProtectRW(int devidx)
{
	int rv = VbExEcProtectRW(devidx);

	if (rv == VBERROR_EC_REBOOT_TO_RO_REQUIRED) {
		VBDEBUG(("VbExEcProtectRW() needs reboot\n"));
	} else if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbExEcProtectRW() returned %d\n", rv));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_PROTECT);
	}
	return rv;
}

VbError_t VbEcSoftwareSync(int devidx, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	int in_rw = 0;
	int rv;
	const uint8_t *ec_hash = NULL;
	int ec_hash_size;
	const uint8_t *rw_hash = NULL;
	int rw_hash_size;
	const uint8_t *expected = NULL;
	int expected_size;
	uint8_t expected_hash[SHA256_DIGEST_SIZE];
	int need_update = 0;
	int i;

	VBDEBUG(("VbEcSoftwareSync(devidx=%d)\n", devidx));

	/* Determine whether the EC is in RO or RW */
	rv = VbExEcRunningRW(devidx, &in_rw);

	if (shared->recovery_reason) {
		/* Recovery mode; just verify the EC is in RO code */
		if (rv == VBERROR_SUCCESS && in_rw == 1) {
			/*
			 * EC is definitely in RW firmware.  We want it in
			 * read-only code, so preserve the current recovery
			 * reason and reboot.
			 *
			 * We don't reboot on error or unknown EC code, because
			 * we could end up in an endless reboot loop.  If we
			 * had some way to track that we'd already rebooted for
			 * this reason, we could retry only once.
			 */
			VBDEBUG(("VbEcSoftwareSync() - "
				 "want recovery but got EC-RW\n"));
			VbSetRecoveryRequest(shared->recovery_reason);
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		VBDEBUG(("VbEcSoftwareSync() in recovery; EC-RO\n"));
		return VBERROR_SUCCESS;
	}

	/*
	 * Not in recovery.  If we couldn't determine where the EC was,
	 * reboot to recovery.
	 */
	if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcRunningRW() returned %d\n", rv));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_UNKNOWN_IMAGE);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	/* If AP is read-only normal, EC should be in its RO code also. */
	if (shared->flags & VBSD_LF_USE_RO_NORMAL) {
		/* If EC is in RW code, request reboot back to RO */
		if (in_rw == 1) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "want RO-normal but got EC-RW\n"));
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		/* Protect the RW flash and stay in EC-RO */
		rv = EcProtectRW(devidx);
		if (rv != VBERROR_SUCCESS)
			return rv;

		rv = VbExEcDisableJump(devidx);
		if (rv != VBERROR_SUCCESS) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "VbExEcDisableJump() returned %d\n", rv));
			VbSetRecoveryRequest(VBNV_RECOVERY_EC_SOFTWARE_SYNC);
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		VBDEBUG(("VbEcSoftwareSync() in RO-Normal; EC-RO\n"));
		return VBERROR_SUCCESS;
	}

	/* Get hash of EC-RW */
	rv = VbExEcHashRW(devidx, &ec_hash, &ec_hash_size);
	if (rv) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcHashRW() returned %d\n", rv));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_HASH_FAILED);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}
	if (ec_hash_size != SHA256_DIGEST_SIZE) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcHashRW() says size %d, not %d\n",
			 ec_hash_size, SHA256_DIGEST_SIZE));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_HASH_SIZE);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	VBDEBUG(("EC hash:"));
	for (i = 0; i < SHA256_DIGEST_SIZE; i++)
		VBDEBUG(("%02x", ec_hash[i]));
	VBDEBUG(("\n"));

	/*
	 * Get expected EC-RW hash. Note that we've already checked for
	 * RO_NORMAL, so we know that the BIOS must be RW-A or RW-B, and
	 * therefore the EC must match.
	 */
	rv = VbExEcGetExpectedRWHash(devidx, shared->firmware_index ?
				 VB_SELECT_FIRMWARE_B : VB_SELECT_FIRMWARE_A,
				 &rw_hash, &rw_hash_size);

	if (rv == VBERROR_EC_GET_EXPECTED_HASH_FROM_IMAGE) {
		/*
		 * BIOS has verified EC image but doesn't have a precomputed
		 * hash for it, so we must compute the hash ourselves.
		 */
		rw_hash = NULL;
	} else if (rv) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcGetExpectedRWHash() returned %d\n", rv));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_EXPECTED_HASH);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	} else if (rw_hash_size != SHA256_DIGEST_SIZE) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcGetExpectedRWHash() says size %d, not %d\n",
			 rw_hash_size, SHA256_DIGEST_SIZE));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_EXPECTED_HASH);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	} else {
		VBDEBUG(("Expected hash:"));
		for (i = 0; i < SHA256_DIGEST_SIZE; i++)
			VBDEBUG(("%02x", rw_hash[i]));
		VBDEBUG(("\n"));

		need_update = SafeMemcmp(ec_hash, rw_hash, SHA256_DIGEST_SIZE);
	}

	/*
	 * Get expected EC-RW image if we're sure we need to update (because the
	 * expected hash didn't match the EC) or we still don't know (because
	 * there was no expected hash and we need the image to compute one
	 * ourselves).
	 */
	if (need_update || !rw_hash) {
		/* Get expected EC-RW image */
		rv = VbExEcGetExpectedRW(devidx, shared->firmware_index ?
					 VB_SELECT_FIRMWARE_B :
					 VB_SELECT_FIRMWARE_A,
					 &expected, &expected_size);
		if (rv) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "VbExEcGetExpectedRW() returned %d\n", rv));
			VbSetRecoveryRequest(VBNV_RECOVERY_EC_EXPECTED_IMAGE);
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}
		VBDEBUG(("VbEcSoftwareSync() - expected len = %d\n",
			 expected_size));

		/* Hash expected image */
		internal_SHA256(expected, expected_size, expected_hash);
		VBDEBUG(("Computed hash of expected image:"));
		for (i = 0; i < SHA256_DIGEST_SIZE; i++)
			VBDEBUG(("%02x", expected_hash[i]));
		VBDEBUG(("\n"));
	}

	if (!rw_hash) {
		/*
		 * BIOS didn't have expected EC hash, so check if we need
		 * update by comparing EC hash to the one we just computed.
		 */
		need_update = SafeMemcmp(ec_hash, expected_hash,
					 SHA256_DIGEST_SIZE);
	} else if (need_update &&
		   SafeMemcmp(rw_hash, expected_hash, SHA256_DIGEST_SIZE)) {
		/*
		 * We need to update, but the expected EC image doesn't match
		 * the expected EC hash we were given.
		 */
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcGetExpectedRW() returned %d\n", rv));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_HASH_MISMATCH);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	/*
	 * TODO: GBB flag to override whether we need update; needed for EC
	 * development.
	 */

	if (in_rw) {
		if (need_update) {
			/*
			 * Check if BIOS should also load VGA Option ROM when
			 * rebooting to save another reboot if possible.
			 */
			if ((shared->flags & VBSD_EC_SLOW_UPDATE) &&
			    (shared->flags & VBSD_OPROM_MATTERS) &&
			    !(shared->flags & VBSD_OPROM_LOADED)) {
				VBDEBUG(("VbEcSoftwareSync() - Reboot to "
					 "load VGA Option ROM\n"));
				VbNvSet(&vnc, VBNV_OPROM_NEEDED, 1);
			}

			/*
			 * EC is running the wrong RW image.  Reboot the EC to
			 * RO so we can update it on the next boot.
			 */
			VBDEBUG(("VbEcSoftwareSync() - "
				 "in RW, need to update RW, so reboot\n"));
			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		VBDEBUG(("VbEcSoftwareSync() in EC-RW and it matches\n"));
		return VBERROR_SUCCESS;
	}

	/* Update EC if necessary */
	if (need_update) {
		VBDEBUG(("VbEcSoftwareSync() updating EC-RW...\n"));

		if (shared->flags & VBSD_EC_SLOW_UPDATE) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "EC is slow. Show WAIT screen.\n"));

			/* Ensure the VGA Option ROM is loaded */
			if ((shared->flags & VBSD_OPROM_MATTERS) &&
			    !(shared->flags & VBSD_OPROM_LOADED)) {
				VBDEBUG(("VbEcSoftwareSync() - Reboot to "
					 "load VGA Option ROM\n"));
				VbNvSet(&vnc, VBNV_OPROM_NEEDED, 1);
				return VBERROR_VGA_OPROM_MISMATCH;
			}

			VbDisplayScreen(cparams, VB_SCREEN_WAIT, 0, &vnc);
		}

		rv = VbExEcUpdateRW(devidx, expected, expected_size);

		if (rv != VBERROR_SUCCESS) {
			VBDEBUG(("VbEcSoftwareSync() - "
				 "VbExEcUpdateRW() returned %d\n", rv));

			/*
			 * The EC may know it needs a reboot.  It may need to
			 * unprotect RW before updating, or may need to reboot
			 * after RW updated.  Either way, it's not an error
			 * requiring recovery mode.
			 *
			 * If we fail for any other reason, trigger recovery
			 * mode.
			 */
			if (rv != VBERROR_EC_REBOOT_TO_RO_REQUIRED)
				VbSetRecoveryRequest(VBNV_RECOVERY_EC_UPDATE);

			return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
		}

		/*
		 * TODO: should ask EC to recompute its hash to verify it's
		 * correct before continuing?
		 */
	}

	/* Protect EC-RW flash */
	rv = EcProtectRW(devidx);
	if (rv != VBERROR_SUCCESS)
		return rv;

	/* Tell EC to jump to its RW image */
	VBDEBUG(("VbEcSoftwareSync() jumping to EC-RW\n"));
	rv = VbExEcJumpToRW(devidx);
	if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbEcSoftwareSync() - "
			 "VbExEcJumpToRW() returned %d\n", rv));

		/*
		 * If the EC booted RO-normal and a previous AP boot has called
		 * VbExEcStayInRO(), we need to reboot the EC to unlock the
		 * ability to jump to the RW firmware.
		 *
		 * All other errors trigger recovery mode.
		 */
		if (rv != VBERROR_EC_REBOOT_TO_RO_REQUIRED)
			VbSetRecoveryRequest(VBNV_RECOVERY_EC_JUMP_RW);

		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	VBDEBUG(("VbEcSoftwareSync() jumped to EC-RW\n"));

	rv = VbExEcDisableJump(devidx);
	if (rv != VBERROR_SUCCESS) {
		VBDEBUG(("VbEcSoftwareSync() - "
			"VbExEcDisableJump() returned %d\n", rv));
		VbSetRecoveryRequest(VBNV_RECOVERY_EC_SOFTWARE_SYNC);
		return VBERROR_EC_REBOOT_TO_RO_REQUIRED;
	}

	/*
	 * Reboot to unload VGA Option ROM if:
	 * - RW update was done
	 * - the system is NOT in developer mode
	 * - the system has slow EC update flag set
	 * - the VGA Option ROM was needed and loaded
	 */
	if (need_update &&
	    !(shared->flags & VBSD_BOOT_DEV_SWITCH_ON) &&
	    (shared->flags & VBSD_EC_SLOW_UPDATE) &&
	    (shared->flags & VBSD_OPROM_MATTERS) &&
	    (shared->flags & VBSD_OPROM_LOADED)) {
		VBDEBUG(("VbEcSoftwareSync() - Reboot to "
			 "unload VGA Option ROM\n"));
		return VBERROR_VGA_OPROM_MISMATCH;
	}

	VBDEBUG(("VbEcSoftwareSync() in RW; done\n"));
	return VBERROR_SUCCESS;
}

/* This function is also used by tests */
void VbApiKernelFree(VbCommonParams *cparams)
{
	/* VbSelectAndLoadKernel() always allocates this, tests don't */
	if (cparams->gbb) {
		VbExFree(cparams->gbb);
		cparams->gbb = NULL;
	}
	if (cparams->bmp) {
		VbExFree(cparams->bmp);
		cparams->bmp = NULL;
	}
}

VbError_t VbSelectAndLoadKernel(VbCommonParams *cparams,
                                VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	VbError_t retval = VBERROR_SUCCESS;
	LoadKernelParams p;
	uint32_t tpm_status = 0;

	/* Start timer */
	shared->timer_vb_select_and_load_kernel_enter = VbExGetTimer();

	VbExNvStorageRead(vnc.raw);
	VbNvSetup(&vnc);

	/* Clear output params in case we fail */
	kparams->disk_handle = NULL;
	kparams->partition_number = 0;
	kparams->bootloader_address = 0;
	kparams->bootloader_size = 0;
	kparams->flags = 0;
	Memset(kparams->partition_guid, 0, sizeof(kparams->partition_guid));

	cparams->bmp = NULL;
	cparams->gbb = VbExMalloc(sizeof(*cparams->gbb));
	retval = VbGbbReadHeader_static(cparams, cparams->gbb);
	if (VBERROR_SUCCESS != retval)
		goto VbSelectAndLoadKernel_exit;

	/* Do EC software sync if necessary */
	if ((shared->flags & VBSD_EC_SOFTWARE_SYNC) &&
	    !(cparams->gbb->flags & GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC)) {
		int oprom_mismatch = 0;

		retval = VbEcSoftwareSync(0, cparams);
		/* Save reboot requested until after possible PD sync */
		if (retval == VBERROR_VGA_OPROM_MISMATCH)
			oprom_mismatch = 1;
		else if (retval != VBERROR_SUCCESS)
			goto VbSelectAndLoadKernel_exit;

#ifdef PD_SYNC
		if (!(cparams->gbb->flags &
		      GBB_FLAG_DISABLE_PD_SOFTWARE_SYNC)) {
			retval = VbEcSoftwareSync(1, cparams);
			if (retval == VBERROR_VGA_OPROM_MISMATCH)
				oprom_mismatch = 1;
			else if (retval != VBERROR_SUCCESS)
				goto VbSelectAndLoadKernel_exit;
		}
#endif

		/* Request reboot to unload VGA Option ROM */
		if (oprom_mismatch) {
			retval = VBERROR_VGA_OPROM_MISMATCH;
			goto VbSelectAndLoadKernel_exit;
		}
	}

	/* Read kernel version from the TPM.  Ignore errors in recovery mode. */
	tpm_status = RollbackKernelRead(&shared->kernel_version_tpm);
	if (0 != tpm_status) {
		VBDEBUG(("Unable to get kernel versions from TPM\n"));
		if (!shared->recovery_reason) {
			VbSetRecoveryRequest(VBNV_RECOVERY_RW_TPM_R_ERROR);
			retval = VBERROR_TPM_READ_KERNEL;
			goto VbSelectAndLoadKernel_exit;
		}
	}
	shared->kernel_version_tpm_start = shared->kernel_version_tpm;

	/* Fill in params for calls to LoadKernel() */
	Memset(&p, 0, sizeof(p));
	p.shared_data_blob = cparams->shared_data_blob;
	p.shared_data_size = cparams->shared_data_size;
	p.gbb_data = cparams->gbb_data;
	p.gbb_size = cparams->gbb_size;

	/*
	 * This could be set to NULL, in which case the vboot header
	 * information about the load address and size will be used.
	 */
	p.kernel_buffer = kparams->kernel_buffer;
	p.kernel_buffer_size = kparams->kernel_buffer_size;

	p.nv_context = &vnc;
	p.boot_flags = 0;
	if (shared->flags & VBSD_BOOT_DEV_SWITCH_ON)
		p.boot_flags |= BOOT_FLAG_DEVELOPER;

	/* Handle separate normal and developer firmware builds. */
#if defined(VBOOT_FIRMWARE_TYPE_NORMAL)
	/* Normal-type firmware always acts like the dev switch is off. */
	p.boot_flags &= ~BOOT_FLAG_DEVELOPER;
#elif defined(VBOOT_FIRMWARE_TYPE_DEVELOPER)
	/* Developer-type firmware fails if the dev switch is off. */
	if (!(p.boot_flags & BOOT_FLAG_DEVELOPER)) {
		/*
		 * Dev firmware should be signed with a key that only verifies
		 * when the dev switch is on, so we should never get here.
		 */
		VBDEBUG(("Developer firmware called with dev switch off!\n"));
		VbSetRecoveryRequest(VBNV_RECOVERY_RW_DEV_MISMATCH);
		retval = VBERROR_DEV_FIRMWARE_SWITCH_MISMATCH;
		goto VbSelectAndLoadKernel_exit;
	}
#else
	/*
	 * Recovery firmware, or merged normal+developer firmware.  No need to
	 * override flags.
	 */
#endif

	/* Select boot path */
	if (shared->recovery_reason) {
		/* Recovery boot */
		p.boot_flags |= BOOT_FLAG_RECOVERY;
		retval = VbBootRecovery(cparams, &p);
		VbExEcEnteringMode(0, VB_EC_RECOVERY);
		VbDisplayScreen(cparams, VB_SCREEN_BLANK, 0, &vnc);

	} else if (p.boot_flags & BOOT_FLAG_DEVELOPER) {
		/* Developer boot */
		retval = VbBootDeveloper(cparams, &p);
		VbExEcEnteringMode(0, VB_EC_DEVELOPER);
		VbDisplayScreen(cparams, VB_SCREEN_BLANK, 0, &vnc);

	} else {
		/* Normal boot */
		VbExEcEnteringMode(0, VB_EC_NORMAL);
		retval = VbBootNormal(cparams, &p);

		if ((1 == shared->firmware_index) &&
		    (shared->flags & VBSD_FWB_TRIED)) {
			/*
			 * Special cases for when we're trying a new firmware
			 * B.  These are needed because firmware updates also
			 * usually change the kernel key, which means that the
			 * B firmware can only boot a new kernel, and the old
			 * firmware in A can only boot the previous kernel.
			 *
			 * Don't advance the TPM if we're trying a new firmware
			 * B, because we don't yet know if the new kernel will
			 * successfully boot.  We still want to be able to fall
			 * back to the previous firmware+kernel if the new
			 * firmware+kernel fails.
			 *
			 * If we found only invalid kernels, reboot and try
			 * again.  This allows us to fall back to the previous
			 * firmware+kernel instead of giving up and going to
			 * recovery mode right away.  We'll still go to
			 * recovery mode if we run out of tries and the old
			 * firmware can't find a kernel it likes.
			 */
			if (VBERROR_INVALID_KERNEL_FOUND == retval) {
				VBDEBUG(("Trying firmware B, "
					 "and only found invalid kernels.\n"));
				VbSetRecoveryRequest(VBNV_RECOVERY_NOT_REQUESTED);
				goto VbSelectAndLoadKernel_exit;
			}
		} else {
			/* Not trying a new firmware B. */

			/* See if we need to update the TPM. */
			VBDEBUG(("Checking if TPM kernel version needs "
				 "advancing\n"));
			if (shared->kernel_version_tpm >
			    shared->kernel_version_tpm_start) {
				tpm_status = RollbackKernelWrite(
						shared->kernel_version_tpm);
				if (0 != tpm_status) {
					VBDEBUG(("Error writing kernel "
						 "versions to TPM.\n"));
					VbSetRecoveryRequest(VBNV_RECOVERY_RW_TPM_W_ERROR);
					retval = VBERROR_TPM_WRITE_KERNEL;
					goto VbSelectAndLoadKernel_exit;
				}
			}
		}
	}

	if (VBERROR_SUCCESS != retval)
		goto VbSelectAndLoadKernel_exit;

	/* Save disk parameters */
	kparams->disk_handle = p.disk_handle;
	kparams->partition_number = (uint32_t)p.partition_number;
	kparams->bootloader_address = p.bootloader_address;
	kparams->bootloader_size = (uint32_t)p.bootloader_size;
	kparams->flags = p.flags;
	Memcpy(kparams->partition_guid, p.partition_guid,
	       sizeof(kparams->partition_guid));

	/* Lock the kernel versions.  Ignore errors in recovery mode. */
	tpm_status = RollbackKernelLock(shared->recovery_reason);
	if (0 != tpm_status) {
		VBDEBUG(("Error locking kernel versions.\n"));
		if (!shared->recovery_reason) {
			VbSetRecoveryRequest(VBNV_RECOVERY_RW_TPM_L_ERROR);
			retval = VBERROR_TPM_LOCK_KERNEL;
			goto VbSelectAndLoadKernel_exit;
		}
	}

 VbSelectAndLoadKernel_exit:

	VbApiKernelFree(cparams);

	VbNvTeardown(&vnc);
	if (vnc.raw_changed)
		VbExNvStorageWrite(vnc.raw);

	/* Stop timer */
	shared->timer_vb_select_and_load_kernel_exit = VbExGetTimer();

	kparams->kernel_buffer = p.kernel_buffer;
	kparams->kernel_buffer_size = p.kernel_buffer_size;

	VBDEBUG(("VbSelectAndLoadKernel() returning %d\n", (int)retval));

	/* Pass through return value from boot path */
	return retval;
}
