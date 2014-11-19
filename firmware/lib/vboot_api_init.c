/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for init, firmware selection
 */

#include "sysincludes.h"

#include "region.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_firmware_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

VbError_t VbInit(VbCommonParams *cparams, VbInitParams *iparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	GoogleBinaryBlockHeader gbb;
	VbNvContext vnc;
	VbError_t retval = VBERROR_SUCCESS;
	uint32_t recovery = VBNV_RECOVERY_NOT_REQUESTED;
	int is_s3_resume = 0;
	uint32_t s3_debug_boot = 0;
	uint32_t require_official_os = 0;
	uint32_t tpm_version = 0;
	uint32_t tpm_status = 0;
	int has_virt_dev_switch = 0;
	int is_hw_dev = 0;
	int is_virt_dev = 0;
	uint32_t disable_dev_request = 0;
	uint32_t clear_tpm_owner_request = 0;
	int is_dev = 0;
	uint32_t backup_requested = 0;
	uint32_t backup_for_safety = 0;
	int lost_nvram;

	/* Initialize output flags */
	iparams->out_flags = 0;

	retval = VbGbbReadHeader_static(cparams, &gbb);
	if (retval)
		return retval;

	VBDEBUG(("VbInit() input flags 0x%x gbb flags 0x%x\n", iparams->flags,
		 gbb.flags));

	/* Set up NV storage */
	VbExNvStorageRead(vnc.raw);
	VbNvSetup(&vnc);
	lost_nvram = vnc.regenerate_crc;

	/* Initialize shared data structure */
	if (0 != VbSharedDataInit(shared, cparams->shared_data_size)) {
		VBDEBUG(("Shared data init error\n"));
		return VBERROR_INIT_SHARED_DATA;
	}

	shared->timer_vb_init_enter = VbExGetTimer();

	/* Copy some boot switch flags */
	/* TODO: in next refactor, just save in/out flags in VbSharedData */
	shared->flags = 0;
	if (iparams->flags & VB_INIT_FLAG_REC_BUTTON_PRESSED)
		shared->flags |= VBSD_BOOT_REC_SWITCH_ON;
	if (iparams->flags & VB_INIT_FLAG_WP_ENABLED)
		shared->flags |= VBSD_BOOT_FIRMWARE_WP_ENABLED;
	if (iparams->flags & VB_INIT_FLAG_SW_WP_ENABLED)
		shared->flags |= VBSD_BOOT_FIRMWARE_SW_WP_ENABLED;
	if (iparams->flags & VB_INIT_FLAG_S3_RESUME)
		shared->flags |= VBSD_BOOT_S3_RESUME;
	if (iparams->flags & VB_INIT_FLAG_RO_NORMAL_SUPPORT)
		shared->flags |= VBSD_BOOT_RO_NORMAL_SUPPORT;
	if (iparams->flags & VB_INIT_FLAG_EC_SOFTWARE_SYNC)
		shared->flags |= VBSD_EC_SOFTWARE_SYNC;
	if (iparams->flags & VB_INIT_FLAG_EC_SLOW_UPDATE)
		shared->flags |= VBSD_EC_SLOW_UPDATE;
	if (iparams->flags & VB_INIT_FLAG_VIRTUAL_REC_SWITCH)
		shared->flags |= VBSD_BOOT_REC_SWITCH_VIRTUAL;
	if (iparams->flags & VB_INIT_FLAG_OPROM_MATTERS)
		shared->flags |= VBSD_OPROM_MATTERS;
	if (iparams->flags & VB_INIT_FLAG_OPROM_LOADED)
		shared->flags |= VBSD_OPROM_LOADED;

	is_s3_resume = (iparams->flags & VB_INIT_FLAG_S3_RESUME ? 1 : 0);

	/* Check if the OS is requesting a debug S3 reset */
	VbNvGet(&vnc, VBNV_DEBUG_RESET_MODE, &s3_debug_boot);
	if (s3_debug_boot) {
		if (is_s3_resume) {
			VBDEBUG(("VbInit() requesting S3 debug boot\n"));
			iparams->out_flags |= VB_INIT_OUT_S3_DEBUG_BOOT;
			is_s3_resume = 0;  /* Proceed as if normal boot */
		}

		/*
		 * Clear the request even if this is a normal boot, since we
		 * don't want the NEXT S3 resume to be a debug reset unless the
		 * OS asserts the request again.
		 */
		VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 0);
	}

	/*
	 * If this isn't a S3 resume, read the current recovery request, then
	 * clear it so we don't get stuck in recovery mode.
	 */
	if (!is_s3_resume) {
		VbNvGet(&vnc, VBNV_RECOVERY_REQUEST, &recovery);
		VBDEBUG(("VbInit sees recovery request = %d\n", recovery));
		if (VBNV_RECOVERY_NOT_REQUESTED != recovery)
			VbNvSet(&vnc, VBNV_RECOVERY_REQUEST,
				VBNV_RECOVERY_NOT_REQUESTED);
	}

	/*
	 * If the previous boot failed in the firmware somewhere outside of
	 * verified boot, and recovery is not requested for our own reasons,
	 * request recovery mode.  This gives the calling firmware a way to
	 * request recovery if it finds something terribly wrong.
	 */
	if (VBNV_RECOVERY_NOT_REQUESTED == recovery &&
	    iparams->flags & VB_INIT_FLAG_PREVIOUS_BOOT_FAIL) {
		recovery = VBNV_RECOVERY_RO_FIRMWARE;
	}

	/*
	 * If recovery button is pressed, override recovery reason.  Note that
	 * we do this in the S3 resume path also.
	 */
	if (iparams->flags & VB_INIT_FLAG_REC_BUTTON_PRESSED)
		recovery = VBNV_RECOVERY_RO_MANUAL;

	/*
	 * Copy current recovery reason to shared data. If we fail later on, it
	 * won't matter, since we'll just reboot.
	 */
	shared->recovery_reason = (uint8_t)recovery;
	VBDEBUG(("VbInit now sets shared->recovery_reason = %d\n", recovery));

	/*
	 * If this is a S3 resume, resume the TPM.
	 *
	 * FIXME: I think U-Boot won't ever ask us to do this. Can we remove
	 * it?
	 */
	if (is_s3_resume) {
		if (TPM_SUCCESS != RollbackS3Resume()) {
			/*
			 * If we can't resume, just do a full reboot.  No need
			 * to go to recovery mode here, since if the TPM is
			 * really broken we'll catch it on the next boot.
			 */
			retval = VBERROR_TPM_S3_RESUME;
		}
	} else {
		/* Should we pay attention to the TPM's virtual dev-switch? */
		if (iparams->flags & VB_INIT_FLAG_VIRTUAL_DEV_SWITCH) {
			shared->flags |= VBSD_HONOR_VIRT_DEV_SWITCH;
			has_virt_dev_switch = 1;
		}

		/*
		 * We always believe the HW dev-switch, since there's one
		 * attached to servo which may be active even on systems
		 * without a physical switch. The EC may also implement a fake
		 * dev-switch for testing.
		 */
		if (iparams->flags & VB_INIT_FLAG_DEV_SWITCH_ON)
			is_hw_dev = 1;

		/* We may be asked to clear the virtual dev-switch at boot. */
		VbNvGet(&vnc, VBNV_DISABLE_DEV_REQUEST, &disable_dev_request);

		/* Allow GBB flag to override dev switch */
		if (gbb.flags & GBB_FLAG_FORCE_DEV_SWITCH_ON)
			is_hw_dev = 1;

		/* Have we been explicitly asked to clear the TPM owner? */
		VbNvGet(&vnc, VBNV_CLEAR_TPM_OWNER_REQUEST,
			&clear_tpm_owner_request);

		/*
		 * Initialize the TPM. If the developer mode state has changed
		 * since the last boot, we need to clear TPM ownership. If the
		 * TPM space is initialized by this call, the virtual
		 * dev-switch will be disabled by default)
		 */
		VBDEBUG(("TPM: Call RollbackFirmwareSetup(r%d, d%d)\n",
			 recovery, is_hw_dev));
		tpm_status = RollbackFirmwareSetup(is_hw_dev,
						   disable_dev_request,
						   clear_tpm_owner_request,
						   /* two outputs on success */
						   &is_virt_dev, &tpm_version);

		if (0 != tpm_status) {
			VBDEBUG(("Unable to setup TPM and read "
				 "firmware version (0x%x)\n", tpm_status));

			if (TPM_E_MUST_REBOOT == tpm_status) {
				/*
				 * TPM wants to reboot into the same mode we're
				 * in now
				 */
				VBDEBUG(("TPM requires a reboot.\n"));
				if (!recovery) {
					/*
					 * Not recovery mode.  Just reboot (not
					 * into recovery).
					 */
					retval = VBERROR_TPM_REBOOT_REQUIRED;
					goto VbInit_exit;
				} else if (VBNV_RECOVERY_RO_TPM_REBOOT !=
					   shared->recovery_reason) {
					/*
					 * In recovery mode now, and we haven't
					 * requested a TPM reboot yet, so
					 * request one.
					 */
					VbNvSet(&vnc, VBNV_RECOVERY_REQUEST,
						VBNV_RECOVERY_RO_TPM_REBOOT);
					retval = VBERROR_TPM_REBOOT_REQUIRED;
					goto VbInit_exit;
				}
			}

			if (!recovery) {
				VbNvSet(&vnc, VBNV_RECOVERY_REQUEST,
					VBNV_RECOVERY_RO_TPM_S_ERROR);
				VbNvSet(&vnc, VBNV_RECOVERY_SUBCODE,
					tpm_status);
				retval = VBERROR_TPM_FIRMWARE_SETUP;
				goto VbInit_exit;
			}
		}

		/* TPM setup succeeded, or we're in recovery mode and ignoring
		 * errors. What did we learn? */
		shared->fw_version_tpm_start = tpm_version;
		shared->fw_version_tpm = tpm_version;
		if (is_hw_dev || (has_virt_dev_switch && is_virt_dev)) {
			is_dev = 1;
			shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
		}
		if (disable_dev_request && !is_virt_dev)
			VbNvSet(&vnc, VBNV_DISABLE_DEV_REQUEST, 0);
		if (clear_tpm_owner_request) {
			VbNvSet(&vnc, VBNV_CLEAR_TPM_OWNER_REQUEST, 0);
			VbNvSet(&vnc, VBNV_CLEAR_TPM_OWNER_DONE, 1);
		}
	}

	/*
	 * If the nvram state was lost, try to restore the bits we care about
	 * from the backup in the TPM. It's okay if we can't, though.
	 * Note: None of the bits that we back up should have been referenced
	 * before this point. Otherwise, they'll just be overwritten here.
	 * All the other bits will be unchanged from whatever has happened to
	 * them since VbNvSetup() reinitialized the VbNvContext.
	 */
	if (lost_nvram)
		RestoreNvFromBackup(&vnc);

	/* Allow BIOS to load arbitrary option ROMs? */
	if (gbb.flags & GBB_FLAG_LOAD_OPTION_ROMS)
		iparams->out_flags |= VB_INIT_OUT_ENABLE_OPROM;

	/* Factory may need to boot custom OSes when the dev-switch is on */
	if (is_dev && (gbb.flags & GBB_FLAG_ENABLE_ALTERNATE_OS))
		iparams->out_flags |= VB_INIT_OUT_ENABLE_ALTERNATE_OS;

	/* Set output flags */
	if (VBNV_RECOVERY_NOT_REQUESTED != recovery) {
		/* Requesting recovery mode */
		iparams->out_flags |= (VB_INIT_OUT_ENABLE_RECOVERY |
				       VB_INIT_OUT_CLEAR_RAM |
				       VB_INIT_OUT_ENABLE_DISPLAY |
				       VB_INIT_OUT_ENABLE_USB_STORAGE);
	} else if (is_dev) {
		/* Developer switch is on, so need to support dev mode */
		iparams->out_flags |= (VB_INIT_OUT_ENABLE_DEVELOPER |
				       VB_INIT_OUT_CLEAR_RAM |
				       VB_INIT_OUT_ENABLE_DISPLAY |
				       VB_INIT_OUT_ENABLE_USB_STORAGE);
		/* ... which may or may not include custom OSes */
		VbNvGet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, &require_official_os);
		if (!require_official_os)
			iparams->out_flags |= VB_INIT_OUT_ENABLE_ALTERNATE_OS;

		/*
		 * Dev-mode needs the VGA option ROM to be loaded so it can
		 * display the scary boot screen. If we don't have it, we need
		 * to request it and reboot so it can be loaded.
		 */
		if ((iparams->flags & VB_INIT_FLAG_OPROM_MATTERS) &&
		    !(iparams->flags & VB_INIT_FLAG_OPROM_LOADED)) {
			VbNvSet(&vnc, VBNV_OPROM_NEEDED, 1);
			/*
			 * If VbInit() is run before Option ROMs are run it
			 * can still respond to the VbNv flag and does not
			 * need to reboot here.
			 */
			if (!(iparams->flags & VB_INIT_FLAG_BEFORE_OPROM_LOAD))
				retval = VBERROR_VGA_OPROM_MISMATCH;
			VBDEBUG(("VbInit() needs oprom, doesn't have it\n"));
		}

	} else {
		/*
		 * Normal mode, so disable dev_boot_* flags.  This ensures they
		 * will be initially disabled if the user later transitions
		 * back into developer mode.
		 */
		VbNvSet(&vnc, VBNV_DEV_BOOT_USB, 0);
		VbNvSet(&vnc, VBNV_DEV_BOOT_LEGACY, 0);
		VbNvSet(&vnc, VBNV_DEV_BOOT_SIGNED_ONLY, 0);
		/*
		 * Back up any changes now, so these values can't be forgotten
		 * by draining the battery. We really only care about these
		 * three fields, but it's uncommon for any others to change so
		 * this is an easier test than checking each one.
		 */
		if (vnc.regenerate_crc)
			backup_for_safety = 1;

		/*
		 * If we don't need the VGA option ROM but got it anyway, stop
		 * asking for it and reboot in case there's some vulnerability
		 * in using it.
		 */
		if ((iparams->flags & VB_INIT_FLAG_OPROM_MATTERS) &&
		    (iparams->flags & VB_INIT_FLAG_OPROM_LOADED)) {
			VbNvSet(&vnc, VBNV_OPROM_NEEDED, 0);
			/*
			 * If VbInit() is run before Option ROMs are run it
			 * can still respond to the VbNv flag and does not
			 * need to reboot here.
			 */
			if (!(iparams->flags & VB_INIT_FLAG_BEFORE_OPROM_LOAD))
				retval = VBERROR_VGA_OPROM_MISMATCH;
			VBDEBUG(("VbInit() has oprom, doesn't need it\n"));
		}
	}

VbInit_exit:
	/*
	 * If we successfully backup the NV storage, it will clear the
	 * VBNV_BACKUP_NVRAM_REQUEST field, so we want to do it before
	 * calling VbNvTeardown(). It's okay if we can't backup, though.
	 */
	VbNvGet(&vnc, VBNV_BACKUP_NVRAM_REQUEST, &backup_requested);
	if (backup_requested || backup_for_safety)
		SaveNvToBackup(&vnc);

	/* Tear down NV storage */
	VbNvTeardown(&vnc);
	if (vnc.raw_changed)
		VbExNvStorageWrite(vnc.raw);

	VBDEBUG(("VbInit() output flags 0x%x\n", iparams->out_flags));

	shared->timer_vb_init_exit = VbExGetTimer();

	VBDEBUG(("VbInit() returning 0x%x\n", retval));

	return retval;
}
