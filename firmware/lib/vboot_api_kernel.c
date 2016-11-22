/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for kernel selection
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

/* Global variables */
static VbNvContext vnc;
static struct RollbackSpaceFwmp fwmp;
static LoadKernelParams lkp;
static struct vb2_context ctx;
static uint8_t *unaligned_workbuf;

#ifdef CHROMEOS_ENVIRONMENT
/* Global variable accessors for unit tests */

struct RollbackSpaceFwmp *VbApiKernelGetFwmp(void)
{
	return &fwmp;
}

struct LoadKernelParams *VbApiKernelGetParams(void)
{
	return &lkp;
}

#endif

/**
 * Set recovery request (called from vboot_api_kernel.c functions only)
 */
static void VbSetRecoveryRequest(struct vb2_context *ctx,
				 uint32_t recovery_request)
{
	VBDEBUG(("VbSetRecoveryRequest(%d)\n", (int)recovery_request));
	vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST, recovery_request);
}

static void VbSetRecoverySubcode(struct vb2_context *ctx,
				 uint32_t recovery_request)
{
	VBDEBUG(("VbSetRecoverySubcode(%d)\n", (int)recovery_request));
	vb2_nv_set(ctx, VB2_NV_RECOVERY_SUBCODE, recovery_request);
}

static void VbNvLoad(void)
{
	VbExNvStorageRead(vnc.raw);
	VbNvSetup(&vnc);
}

static void VbNvCommit(void)
{
	VbNvTeardown(&vnc);
	if (vnc.raw_changed)
		VbExNvStorageWrite(vnc.raw);
}

static void VbAllowUsbBoot(struct vb2_context *ctx)
{
	VBDEBUG(("%s\n", __func__));
	vb2_nv_set(ctx, VB2_NV_DEV_BOOT_USB, 1);
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

static void VbTryLegacy(int allowed)
{
	if (!allowed)
		VBDEBUG(("VbBootDeveloper() - Legacy boot is disabled\n"));
	else if (0 != RollbackKernelLock(0))
		VBDEBUG(("Error locking kernel versions on legacy boot.\n"));
	else
		VbExLegacy();	/* will not return if successful */

	/* If legacy boot fails, beep and return to calling UI loop. */
	VbExBeep(120, 400);
	VbExSleepMs(120);
	VbExBeep(120, 400);
}

/**
 * Attempt loading a kernel from the specified type(s) of disks.
 *
 * If successful, sets p->disk_handle to the disk for the kernel and returns
 * VBERROR_SUCCESS.
 *
 * @param ctx			Vboot context
 * @param cparams		Vboot common params
 * @param p			Parameters for loading kernel
 * @param get_info_flags	Flags to pass to VbExDiskGetInfo()
 * @return VBERROR_SUCCESS, VBERROR_NO_DISK_FOUND if no disks of the specified
 * type were found, or other non-zero VBERROR_ codes for other failures.
 */
uint32_t VbTryLoadKernel(struct vb2_context *ctx, VbCommonParams *cparams,
                         uint32_t get_info_flags)
{
	VbError_t retval = VBERROR_UNKNOWN;
	VbDiskInfo* disk_info = NULL;
	uint32_t disk_count = 0;
	uint32_t i;

	VBDEBUG(("VbTryLoadKernel() start, get_info_flags=0x%x\n",
		 (unsigned)get_info_flags));

	lkp.fwmp = &fwmp;
	lkp.nv_context = &vnc;
	lkp.disk_handle = NULL;

	/* Find disks */
	if (VBERROR_SUCCESS != VbExDiskGetInfo(&disk_info, &disk_count,
					       get_info_flags))
		disk_count = 0;

	VBDEBUG(("VbTryLoadKernel() found %d disks\n", (int)disk_count));
	if (0 == disk_count) {
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_RW_NO_DISK);
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
		    get_info_flags != (disk_info[i].flags &
				       ~VB_DISK_FLAG_EXTERNAL_GPT)) {
			VBDEBUG(("  skipping: bytes_per_lba=%" PRIu64
				 " lba_count=%" PRIu64 " flags=0x%x\n",
				 disk_info[i].bytes_per_lba,
				 disk_info[i].lba_count,
				 disk_info[i].flags));
			continue;
		}
		lkp.disk_handle = disk_info[i].handle;
		lkp.bytes_per_lba = disk_info[i].bytes_per_lba;
		lkp.gpt_lba_count = disk_info[i].lba_count;
		lkp.streaming_lba_count = disk_info[i].streaming_lba_count
						?: lkp.gpt_lba_count;
		lkp.boot_flags |= disk_info[i].flags & VB_DISK_FLAG_EXTERNAL_GPT
				? BOOT_FLAG_EXTERNAL_GPT : 0;
		retval = LoadKernel(ctx, &lkp, cparams);
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
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_RW_NO_KERNEL);
		lkp.disk_handle = NULL;
	}

	VbExDiskFreeInfo(disk_info, lkp.disk_handle);

	/*
	 * Pass through return code.  Recovery reason (if any) has already been
	 * set by LoadKernel().
	 */
	return retval;
}

uint32_t VbTryUsb(struct vb2_context *ctx, VbCommonParams *cparams)
{
	uint32_t retval = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_REMOVABLE);
	if (VBERROR_SUCCESS == retval) {
		VBDEBUG(("VbBootDeveloper() - booting USB\n"));
	} else {
		VBDEBUG(("VbBootDeveloper() - no kernel found on USB\n"));
		VbExBeep(250, 200);
		VbExSleepMs(120);
		/*
		 * Clear recovery requests from failed
		 * kernel loading, so that powering off
		 * at this point doesn't put us into
		 * recovery mode.
		 */
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_NOT_REQUESTED);
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
			VbCheckDisplayKey(ctx, cparams, key);
		}
		VbExSleepMs(CONFIRM_KEY_DELAY);
	}

	/* Not reached, but compiler will complain without it */
	return -1;
}

VbError_t VbBootNormal(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	/* Boot from fixed disk only */
	VB2_DEBUG("Entering %s()\n", __func__);

	VbError_t rv = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_FIXED);

	VB2_DEBUG("Checking if TPM kernel version needs advancing\n");

	if ((1 == shared->firmware_index) && (shared->flags & VBSD_FWB_TRIED)) {
		/*
		 * Special cases for when we're trying a new firmware B.  These
		 * are needed because firmware updates also usually change the
		 * kernel key, which means that the B firmware can only boot a
		 * new kernel, and the old firmware in A can only boot the
		 * previous kernel.
		 *
		 * Don't advance the TPM if we're trying a new firmware B,
		 * because we don't yet know if the new kernel will
		 * successfully boot.  We still want to be able to fall back to
		 * the previous firmware+kernel if the new firmware+kernel
		 * fails.
		 *
		 * If we found only invalid kernels, reboot and try again.
		 * This allows us to fall back to the previous firmware+kernel
		 * instead of giving up and going to recovery mode right away.
		 * We'll still go to recovery mode if we run out of tries and
		 * the old firmware can't find a kernel it likes.
		 */
		if (rv == VBERROR_INVALID_KERNEL_FOUND) {
			VB2_DEBUG("Trying FW B; only found invalid kernels.\n");
			VbSetRecoveryRequest(ctx, VBNV_RECOVERY_NOT_REQUESTED);
		}

		return rv;
	}

	if ((shared->kernel_version_tpm > shared->kernel_version_tpm_start) &&
	    RollbackKernelWrite(shared->kernel_version_tpm)) {
		VB2_DEBUG("Error writing kernel versions to TPM.\n");
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_RW_TPM_W_ERROR);
		return VBERROR_TPM_WRITE_KERNEL;
	}

	return rv;
}

static const char dev_disable_msg[] =
	"Developer mode is disabled on this device by system policy.\n"
	"For more information, see http://dev.chromium.org/chromium-os/fwmp\n"
	"\n";

VbError_t VbBootDeveloper(struct vb2_context *ctx, VbCommonParams *cparams)
{
	GoogleBinaryBlockHeader *gbb = cparams->gbb;
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	uint32_t disable_dev_boot = 0;
	uint32_t use_usb = 0;
	uint32_t use_legacy = 0;
	uint32_t ctrl_d_pressed = 0;

	VbAudioContext *audio = 0;

	VBDEBUG(("Entering %s()\n", __func__));

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
	if (fwmp.flags & FWMP_DEV_ENABLE_USB)
		allow_usb = 1;
	if (fwmp.flags & FWMP_DEV_ENABLE_LEGACY)
		allow_legacy = 1;
	if (fwmp.flags & FWMP_DEV_DISABLE_BOOT) {
		if (gbb->flags & GBB_FLAG_FORCE_DEV_SWITCH_ON) {
			VBDEBUG(("%s() - FWMP_DEV_DISABLE_BOOT rejected by "
				 "FORCE_DEV_SWITCH_ON\n",
				 __func__));
		} else {
			disable_dev_boot = 1;
		}
	}

	/* If dev mode is disabled, only allow TONORM */
	while (disable_dev_boot) {
		VBDEBUG(("%s() - dev_disable_boot is set.\n", __func__));
		VbDisplayScreen(ctx, cparams, VB_SCREEN_DEVELOPER_TO_NORM, 0);
		VbExDisplayDebugInfo(dev_disable_msg);

		/* Ignore space in VbUserConfirms()... */
		switch (VbUserConfirms(ctx, cparams, 0)) {
		case 1:
			VBDEBUG(("%s() - leaving dev-mode.\n", __func__));
			vb2_nv_set(ctx, VB2_NV_DISABLE_DEV_REQUEST, 1);
			VbDisplayScreen(ctx, cparams,
					VB_SCREEN_TO_NORM_CONFIRMED,
					0);
			VbExSleepMs(5000);
			return VBERROR_REBOOT_REQUIRED;
		case -1:
			VBDEBUG(("%s() - shutdown requested\n", __func__));
			return VBERROR_SHUTDOWN_REQUESTED;
		default:
			/* Ignore user attempt to cancel */
			VBDEBUG(("%s() - ignore cancel TONORM\n", __func__));
		}
	}

	/* Show the dev mode warning screen */
	VbDisplayScreen(ctx, cparams, VB_SCREEN_DEVELOPER_WARNING, 0);

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
				VbDisplayScreen(ctx, cparams,
						VB_SCREEN_DEVELOPER_TO_NORM,
						0);
				/* Ignore space in VbUserConfirms()... */
				switch (VbUserConfirms(ctx, cparams, 0)) {
				case 1:
					VBDEBUG(("%s() - leaving dev-mode.\n",
						 __func__));
					vb2_nv_set(ctx, VB2_NV_DISABLE_DEV_REQUEST,
						1);
					VbDisplayScreen(ctx,
						cparams,
						VB_SCREEN_TO_NORM_CONFIRMED,
						0);
					VbExSleepMs(5000);
					return VBERROR_REBOOT_REQUIRED;
				case -1:
					VBDEBUG(("%s() - shutdown requested\n",
						 __func__));
					return VBERROR_SHUTDOWN_REQUESTED;
				default:
					/* Stay in dev-mode */
					VBDEBUG(("%s() - stay in dev-mode\n",
						 __func__));
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
				VBDEBUG(("%s() - going to recovery\n",
					 __func__));
				VbSetRecoveryRequest(ctx,
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
			VbTryLegacy(allow_legacy);
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
			VBDEBUG(("VbBootDeveloper() - pressed key %d\n", key));
			VbCheckDisplayKey(ctx, cparams, key);
			break;
		}
	} while(VbAudioLooping(audio));

 fallout:

	/* If defaulting to legacy boot, try that unless Ctrl+D was pressed */
	if (use_legacy && !ctrl_d_pressed) {
		VBDEBUG(("VbBootDeveloper() - defaulting to legacy\n"));
		VbTryLegacy(allow_legacy);
	}

	if ((use_usb && !ctrl_d_pressed) && allow_usb) {
		if (VBERROR_SUCCESS == VbTryUsb(ctx, cparams)) {
			VbAudioClose(audio);
			return VBERROR_SUCCESS;
		}
	}

	/* Timeout or Ctrl+D; attempt loading from fixed disk */
	VBDEBUG(("VbBootDeveloper() - trying fixed disk\n"));
	VbAudioClose(audio);
	return VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_FIXED);
}

/* Delay in recovery mode */
#define REC_DISK_DELAY       1000     /* Check disks every 1s */
#define REC_KEY_DELAY        20       /* Check keys every 20ms */
#define REC_MEDIA_INIT_DELAY 500      /* Check removable media every 500ms */

VbError_t VbBootRecovery(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	uint32_t retval;
	uint32_t key;
	int i;

	VBDEBUG(("VbBootRecovery() start\n"));

	/*
	 * If the dev-mode switch is off and the user didn't press the recovery
	 * button (recovery was triggerred automatically), show 'broken' screen.
	 * The user can either only shutdown to abort or hit esc+refresh+power
	 * to initiate recovery as instructed on the screen.
	 */
	if (!(shared->flags & VBSD_BOOT_DEV_SWITCH_ON) &&
	    !(shared->flags & VBSD_BOOT_REC_SWITCH_ON)) {
		/*
		 * We have to save the reason here so that it will survive
		 * coming up three-finger-salute. We're saving it in
		 * VBNV_RECOVERY_SUBCODE to avoid a recovery loop.
		 * If we save the reason in VBNV_RECOVERY_REQUEST, we will come
		 * back here, thus, we won't be able to give a user a chance to
		 * reboot to workaround boot hicups.
		 */
		VBDEBUG(("VbBootRecovery() saving recovery reason (%#x)\n",
				shared->recovery_reason));
		VbSetRecoverySubcode(ctx, shared->recovery_reason);
		VbNvCommit();
		VbDisplayScreen(ctx, cparams, VB_SCREEN_OS_BROKEN, 0);
		VBDEBUG(("VbBootRecovery() waiting for manual recovery\n"));
		while (1) {
			VbCheckDisplayKey(ctx, cparams, VbExKeyboardRead());
			if (VbWantShutdown(cparams->gbb->flags))
				return VBERROR_SHUTDOWN_REQUESTED;
			VbExSleepMs(REC_KEY_DELAY);
		}
	}

	/* Loop and wait for a recovery image */
	VBDEBUG(("VbBootRecovery() waiting for a recovery image\n"));
	while (1) {
		VBDEBUG(("VbBootRecovery() attempting to load kernel2\n"));
		retval = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_REMOVABLE);

		/*
		 * Clear recovery requests from failed kernel loading, since
		 * we're already in recovery mode.  Do this now, so that
		 * powering off after inserting an invalid disk doesn't leave
		 * us stuck in recovery mode.
		 */
		VbSetRecoveryRequest(ctx, VBNV_RECOVERY_NOT_REQUESTED);

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
					VBDEBUG(("%s() Enabling dev-mode...\n",
						 __func__));
					if (TPM_SUCCESS != SetVirtualDevMode(1))
						return VBERROR_TPM_SET_BOOT_MODE_STATE;
					VBDEBUG(("%s() Reboot so it will take "
						 "effect\n", __func__));
					if (VbExGetSwitches
					    (VB_INIT_FLAG_ALLOW_USB_BOOT))
						VbAllowUsbBoot(ctx);
					return VBERROR_REBOOT_REQUIRED;
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
				VbCheckDisplayKey(ctx, cparams, key);
			}
			if (VbWantShutdown(cparams->gbb->flags))
				return VBERROR_SHUTDOWN_REQUESTED;
			VbExSleepMs(REC_KEY_DELAY);
		}
	}

	return VBERROR_SUCCESS;
}

/* This function is also used by tests */
void VbApiKernelFree(VbCommonParams *cparams)
{
	/* VbSelectAndLoadKernel() always allocates this, tests don't */
	if (cparams->gbb) {
		free(cparams->gbb);
		cparams->gbb = NULL;
	}
	if (cparams->bmp) {
		free(cparams->bmp);
		cparams->bmp = NULL;
	}
}

static VbError_t vb2_kernel_setup(VbCommonParams *cparams,
				  VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	/* Start timer */
	shared->timer_vb_select_and_load_kernel_enter = VbExGetTimer();

	/*
	 * Set up vboot context.
	 *
	 * TODO: Propagate this up to higher API levels, and use more of the
	 * context fields (e.g. secdatak) and flags.
	 */
	memset(&ctx, 0, sizeof(ctx));

	VbNvLoad();
	memcpy(ctx.nvdata, vnc.raw, VB2_NVDATA_SIZE);

	if (shared->recovery_reason)
		ctx.flags |= VB2_CONTEXT_RECOVERY_MODE;
	if (shared->flags & VBSD_BOOT_DEV_SWITCH_ON)
		ctx.flags |= VB2_CONTEXT_DEVELOPER_MODE;

	ctx.workbuf_size = VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE +
			VB2_WORKBUF_ALIGN;

	unaligned_workbuf = ctx.workbuf = malloc(ctx.workbuf_size);
	if (!unaligned_workbuf) {
		VB2_DEBUG("%s: Can't allocate work buffer\n", __func__);
		VbSetRecoveryRequest(&ctx, VB2_RECOVERY_RW_SHARED_DATA);
		return VBERROR_INIT_SHARED_DATA;
	}

	if (VB2_SUCCESS != vb2_align(&ctx.workbuf, &ctx.workbuf_size,
				     VB2_WORKBUF_ALIGN,
				     VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE)) {
		VB2_DEBUG("%s: Can't align work buffer\n", __func__);
		VbSetRecoveryRequest(&ctx, VB2_RECOVERY_RW_SHARED_DATA);
		return VBERROR_INIT_SHARED_DATA;
	}

	if (VB2_SUCCESS != vb2_init_context(&ctx)) {
		VB2_DEBUG("%s: Can't init vb2_context\n", __func__);
		free(unaligned_workbuf);
		VbSetRecoveryRequest(&ctx, VB2_RECOVERY_RW_SHARED_DATA);
		return VBERROR_INIT_SHARED_DATA;
	}

	struct vb2_shared_data *sd = vb2_get_sd(&ctx);
	sd->recovery_reason = shared->recovery_reason;

	/*
	 * If we're in recovery mode just to do memory retraining, all we
	 * need to do is reboot.
	 */
	if (shared->recovery_reason == VBNV_RECOVERY_TRAIN_AND_REBOOT) {
		VB2_DEBUG("Reboot after retraining in recovery.\n");
		return VBERROR_REBOOT_REQUIRED;
	}

	/* Fill in params for calls to LoadKernel() */
	memset(&lkp, 0, sizeof(lkp));
	lkp.kernel_buffer = kparams->kernel_buffer;
	lkp.kernel_buffer_size = kparams->kernel_buffer_size;

	/* Clear output params in case we fail */
	kparams->disk_handle = NULL;
	kparams->partition_number = 0;
	kparams->bootloader_address = 0;
	kparams->bootloader_size = 0;
	kparams->flags = 0;
	memset(kparams->partition_guid, 0, sizeof(kparams->partition_guid));

	/* Read GBB header, since we'll needs flags from it */
	cparams->bmp = NULL;
	cparams->gbb = malloc(sizeof(*cparams->gbb));
	uint32_t retval = VbGbbReadHeader_static(cparams, cparams->gbb);
	if (retval)
		return retval;

	/* Read kernel version from the TPM.  Ignore errors in recovery mode. */
	if (RollbackKernelRead(&shared->kernel_version_tpm)) {
		VB2_DEBUG("Unable to get kernel versions from TPM\n");
		if (!shared->recovery_reason) {
			VbSetRecoveryRequest(&ctx,
					     VBNV_RECOVERY_RW_TPM_R_ERROR);
			return VBERROR_TPM_READ_KERNEL;
		}
	}

	shared->kernel_version_tpm_start = shared->kernel_version_tpm;

	/* Read FWMP.  Ignore errors in recovery mode. */
	if (cparams->gbb->flags & GBB_FLAG_DISABLE_FWMP) {
		memset(&fwmp, 0, sizeof(fwmp));
	} else if (RollbackFwmpRead(&fwmp)) {
		VB2_DEBUG("Unable to get FWMP from TPM\n");
		if (!shared->recovery_reason) {
			VbSetRecoveryRequest(&ctx,
					     VBNV_RECOVERY_RW_TPM_R_ERROR);
			return VBERROR_TPM_READ_FWMP;
		}
	}

	return VBERROR_SUCCESS;
}

static VbError_t vb2_kernel_phase4(VbCommonParams *cparams,
				   VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	/* Save disk parameters */
	kparams->disk_handle = lkp.disk_handle;
	kparams->partition_number = lkp.partition_number;
	kparams->bootloader_address = lkp.bootloader_address;
	kparams->bootloader_size = lkp.bootloader_size;
	kparams->flags = lkp.flags;
	kparams->kernel_buffer = lkp.kernel_buffer;
	kparams->kernel_buffer_size = lkp.kernel_buffer_size;
	memcpy(kparams->partition_guid, lkp.partition_guid,
	       sizeof(kparams->partition_guid));

	/* Lock the kernel versions if not in recovery mode */
	if (!shared->recovery_reason &&
	    RollbackKernelLock(shared->recovery_reason)) {
		VB2_DEBUG("Error locking kernel versions.\n");
		VbSetRecoveryRequest(&ctx, VBNV_RECOVERY_RW_TPM_L_ERROR);
		return VBERROR_TPM_LOCK_KERNEL;
	}

	return VBERROR_SUCCESS;
}

static void vb2_kernel_cleanup(VbCommonParams *cparams,
			       VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
			(VbSharedDataHeader *)cparams->shared_data_blob;

	/*
	 * Clean up vboot context.
	 *
	 * TODO: This should propagate up to higher levels
	 */
	/* Free buffers */
	free(unaligned_workbuf);
	/* Copy nvdata back to old vboot1 nv context if needed */
	if (ctx.flags & VB2_CONTEXT_NVDATA_CHANGED) {
		memcpy(vnc.raw, ctx.nvdata, VB2_NVDATA_SIZE);
		vnc.raw_changed = 1;
		ctx.flags &= ~VB2_CONTEXT_NVDATA_CHANGED;
	}

	VbApiKernelFree(cparams);

	VbNvCommit();

	/* Stop timer */
	shared->timer_vb_select_and_load_kernel_exit = VbExGetTimer();
}

VbError_t VbSelectAndLoadKernel(VbCommonParams *cparams,
                                VbSelectAndLoadKernelParams *kparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	VbError_t retval = vb2_kernel_setup(cparams, kparams);
	if (retval)
		goto VbSelectAndLoadKernel_exit;

	/*
	 * Do EC software sync if necessary.  This has UI, but it's just a
	 * single non-interactive WAIT screen.
	 */
	retval = ec_sync_all(&ctx, cparams);
	if (retval)
		goto VbSelectAndLoadKernel_exit;

	/* Select boot path */
	if (shared->recovery_reason) {
		/* Recovery boot.  This has UI. */
		retval = VbBootRecovery(&ctx, cparams);
		VbExEcEnteringMode(0, VB_EC_RECOVERY);
		VbDisplayScreen(&ctx, cparams, VB_SCREEN_BLANK, 0);

	} else if (shared->flags & VBSD_BOOT_DEV_SWITCH_ON) {
		/* Developer boot.  This has UI. */
		retval = VbBootDeveloper(&ctx, cparams);
		VbExEcEnteringMode(0, VB_EC_DEVELOPER);
		VbDisplayScreen(&ctx, cparams, VB_SCREEN_BLANK, 0);

	} else {
		/* Normal boot */
		retval = VbBootNormal(&ctx, cparams);
		VbExEcEnteringMode(0, VB_EC_NORMAL);
	}

 VbSelectAndLoadKernel_exit:

	if (VBERROR_SUCCESS == retval)
		retval = vb2_kernel_phase4(cparams, kparams);

	vb2_kernel_cleanup(cparams, kparams);

	/* Pass through return value from boot path */
	VB2_DEBUG("%s returning %d\n", __func__, (int)retval);
	return retval;
}

VbError_t VbVerifyMemoryBootImage(VbCommonParams *cparams,
				  VbSelectAndLoadKernelParams *kparams,
				  void *boot_image,
				  size_t image_size)
{
	VbError_t retval;
	VbPublicKey* kernel_subkey = NULL;
	uint8_t *kbuf;
	VbKeyBlockHeader *key_block;
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	VbKernelPreambleHeader *preamble;
	uint64_t body_offset;
	int hash_only = 0;
	int dev_switch;
	uint32_t allow_fastboot_full_cap = 0;
	uint8_t *workbuf = NULL;
	struct vb2_workbuf wb;

	if ((boot_image == NULL) || (image_size == 0))
		return VBERROR_INVALID_PARAMETER;

	/* Clear output params in case we fail. */
	kparams->disk_handle = NULL;
	kparams->partition_number = 0;
	kparams->bootloader_address = 0;
	kparams->bootloader_size = 0;
	kparams->flags = 0;
	memset(kparams->partition_guid, 0, sizeof(kparams->partition_guid));

	kbuf = boot_image;

	/* Read GBB Header */
	cparams->bmp = NULL;
	cparams->gbb = malloc(sizeof(*cparams->gbb));
	retval = VbGbbReadHeader_static(cparams, cparams->gbb);
	if (VBERROR_SUCCESS != retval) {
		VBDEBUG(("Gbb read header failed.\n"));
		return retval;
	}

	/*
	 * We don't care verifying the image if:
	 * 1. dev-mode switch is on and
	 * 2a. GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP is set, or
	 * 2b. DEV_BOOT_FASTBOOT_FULL_CAP flag is set in NvStorage
	 *
	 * Check only the integrity of the image.
	 */
	dev_switch = shared->flags & VBSD_BOOT_DEV_SWITCH_ON;

	VbNvLoad();
	VbNvGet(&vnc, VB2_NV_DEV_BOOT_FASTBOOT_FULL_CAP,
		&allow_fastboot_full_cap);

	if (0 == allow_fastboot_full_cap) {
		allow_fastboot_full_cap = !!(cparams->gbb->flags &
				GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP);
	}

	if (dev_switch && allow_fastboot_full_cap) {
		VBDEBUG(("Only performing integrity-check.\n"));
		hash_only = 1;
	} else {
		/* Get recovery key. */
		retval = VbGbbReadRecoveryKey(cparams, &kernel_subkey);
		if (VBERROR_SUCCESS != retval) {
			VBDEBUG(("Gbb Read Recovery key failed.\n"));
			return retval;
		}
	}

	/* If we fail at any step, retval returned would be invalid kernel. */
	retval = VBERROR_INVALID_KERNEL_FOUND;

	/* Allocate work buffer */
	workbuf = (uint8_t *)malloc(VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE);
	if (!workbuf)
		goto fail;
	vb2_workbuf_init(&wb, workbuf, VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE);

	/* Verify the key block. */
	key_block = (VbKeyBlockHeader *)kbuf;
	struct vb2_keyblock *keyblock2 = (struct vb2_keyblock *)kbuf;
	int rv;
	if (hash_only) {
		rv = vb2_verify_keyblock_hash(keyblock2, image_size, &wb);
	} else {
		/* Unpack kernel subkey */
		struct vb2_public_key kernel_subkey2;
		if (VB2_SUCCESS !=
		    vb2_unpack_key(&kernel_subkey2,
				   (struct vb2_packed_key *)kernel_subkey)) {
			VBDEBUG(("Unable to unpack kernel subkey\n"));
			goto fail;
		}
		rv = vb2_verify_keyblock(keyblock2, image_size,
					 &kernel_subkey2, &wb);
	}

	if (VB2_SUCCESS != rv) {
		VBDEBUG(("Verifying key block signature/hash failed.\n"));
		goto fail;
	}

	/* Check the key block flags against the current boot mode. */
	if (!(key_block->key_block_flags &
	      (dev_switch ? KEY_BLOCK_FLAG_DEVELOPER_1 :
	       KEY_BLOCK_FLAG_DEVELOPER_0))) {
		VBDEBUG(("Key block developer flag mismatch.\n"));
		if (hash_only == 0)
			goto fail;
	}

	if (!(key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_1)) {
		VBDEBUG(("Key block recovery flag mismatch.\n"));
		if (hash_only == 0)
			goto fail;
	}

	/* Get key for preamble/data verification from the key block. */
	struct vb2_public_key data_key2;
	if (VB2_SUCCESS != vb2_unpack_key(&data_key2, &keyblock2->data_key)) {
		VBDEBUG(("Unable to unpack kernel data key\n"));
		goto fail;
	}

	/* Verify the preamble, which follows the key block */
	preamble = (VbKernelPreambleHeader *)(kbuf + key_block->key_block_size);
	struct vb2_kernel_preamble *preamble2 =
			(struct vb2_kernel_preamble *)
			(kbuf + key_block->key_block_size);

	if (VB2_SUCCESS != vb2_verify_kernel_preamble(
			preamble2,
			image_size - key_block->key_block_size,
			&data_key2,
			&wb)) {
		VBDEBUG(("Preamble verification failed.\n"));
		goto fail;
	}

	VBDEBUG(("Kernel preamble is good.\n"));

	/* Verify kernel data */
	body_offset = key_block->key_block_size + preamble->preamble_size;
	if (VB2_SUCCESS != vb2_verify_data(
			(const uint8_t *)(kbuf + body_offset),
			image_size - body_offset,
			(struct vb2_signature *)&preamble->body_signature,
			&data_key2, &wb)) {
		VBDEBUG(("Kernel data verification failed.\n"));
		goto fail;
	}

	VBDEBUG(("Kernel is good.\n"));

	/* Fill in output parameters. */
	kparams->kernel_buffer = kbuf + body_offset;
	kparams->kernel_buffer_size = image_size - body_offset;
	kparams->bootloader_address = preamble->bootloader_address;
	kparams->bootloader_size = preamble->bootloader_size;
	if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS)
		kparams->flags = preamble->flags;

	retval = VBERROR_SUCCESS;

fail:
	VbApiKernelFree(cparams);
	if (NULL != kernel_subkey)
		free(kernel_subkey);
	if (NULL != workbuf)
		free(workbuf);
	return retval;
}

VbError_t VbUnlockDevice(void)
{
	VBDEBUG(("%s() Enabling dev-mode...\n", __func__));
	if (TPM_SUCCESS != SetVirtualDevMode(1))
		return VBERROR_TPM_SET_BOOT_MODE_STATE;

	VBDEBUG(("%s() Mode change will take effect on next reboot.\n",
		 __func__));
	return VBERROR_SUCCESS;
}

VbError_t VbLockDevice(void)
{
	VbNvLoad();

	VBDEBUG(("%s() - Storing request to leave dev-mode.\n",
		 __func__));
	VbNvSet(&vnc, VBNV_DISABLE_DEV_REQUEST, 1);

	VbNvCommit();

	VBDEBUG(("%s() Mode change will take effect on next reboot.\n",
		 __func__));

	return VBERROR_SUCCESS;
}
