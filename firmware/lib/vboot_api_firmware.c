/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for init, firmware selection
 */

#include "sysincludes.h"

#include "gbb_access.h"
#include "gbb_header.h"
#include "load_firmware_fw.h"
#include "rollback_index.h"
#include "tpm_bootmode.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"

VbError_t VbSelectFirmware(VbCommonParams *cparams,
                           VbSelectFirmwareParams *fparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	VbNvContext vnc;
	VbError_t retval = VBERROR_UNKNOWN; /* Default to error */
	int is_rec = (shared->recovery_reason ? 1 : 0);
	int is_dev = (shared->flags & VBSD_BOOT_DEV_SWITCH_ON ? 1 : 0);
	uint32_t tpm_status = 0;

	cparams->gbb = NULL;
	cparams->bmp = NULL;

	/* Start timer */
	shared->timer_vb_select_firmware_enter = VbExGetTimer();

	/* Load NV storage */
	VbExNvStorageRead(vnc.raw);
	VbNvSetup(&vnc);

	if (is_rec) {
		/*
		 * Recovery is requested; go straight to recovery without
		 * checking the RW firmware.
		 */
		VBDEBUG(("VbSelectFirmware() detected recovery request\n"));

		/* Best effort to read the GBB */
		cparams->gbb = VbExMalloc(sizeof(*cparams->gbb));
		retval = VbGbbReadHeader_static(cparams, cparams->gbb);
		if (VBERROR_SUCCESS != retval) {
			VBDEBUG(("Can't read GBB. Continuing anyway...\n"));
			VbExFree(cparams->gbb);
			cparams->gbb = NULL;
		}

		/* Go directly to recovery mode */
		fparams->selected_firmware = VB_SELECT_FIRMWARE_RECOVERY;
	} else {
		cparams->gbb = VbExMalloc(sizeof(*cparams->gbb));
		retval = VbGbbReadHeader_static(cparams, cparams->gbb);
		if (VBERROR_SUCCESS != retval)
			goto VbSelectFirmware_exit;

		/* Chain to LoadFirmware() */
		retval = LoadFirmware(cparams, fparams, &vnc);

		/* Exit if we failed to find an acceptable firmware */
		if (VBERROR_SUCCESS != retval)
			goto VbSelectFirmware_exit;

		/* Translate the selected firmware path */
		if (shared->flags & VBSD_LF_USE_RO_NORMAL) {
			/* Request the read-only normal/dev code path */
			fparams->selected_firmware =
				VB_SELECT_FIRMWARE_READONLY;
		} else if (0 == shared->firmware_index)
			fparams->selected_firmware = VB_SELECT_FIRMWARE_A;
		else {
			fparams->selected_firmware = VB_SELECT_FIRMWARE_B;
		}

		/* Update TPM if necessary */
		if (shared->fw_version_tpm_start < shared->fw_version_tpm) {
			tpm_status =
				RollbackFirmwareWrite(shared->fw_version_tpm);
			if (0 != tpm_status) {
				VBDEBUG(("Can't write FW version to TPM.\n"));
				VbNvSet(&vnc, VBNV_RECOVERY_REQUEST,
					VBNV_RECOVERY_RO_TPM_W_ERROR);
				retval = VBERROR_TPM_WRITE_FIRMWARE;
				goto VbSelectFirmware_exit;
			}
		}

		/* Lock firmware versions in TPM */
		tpm_status = RollbackFirmwareLock();
		if (0 != tpm_status) {
			VBDEBUG(("Unable to lock firmware version in TPM.\n"));
			VbNvSet(&vnc, VBNV_RECOVERY_REQUEST,
				VBNV_RECOVERY_RO_TPM_L_ERROR);
			retval = VBERROR_TPM_LOCK_FIRMWARE;
			goto VbSelectFirmware_exit;
		}
	}

	/*
	 * At this point, we have a good idea of how we are going to
	 * boot. Update the TPM with this state information.
	 */
	tpm_status = SetTPMBootModeState(is_dev, is_rec,
					 shared->fw_keyblock_flags,
					 cparams->gbb);
	if (0 != tpm_status) {
		VBDEBUG(("Can't update the TPM with boot mode information.\n"));
		if (!is_rec) {
			VbNvSet(&vnc, VBNV_RECOVERY_REQUEST,
				VBNV_RECOVERY_RO_TPM_U_ERROR);
			retval = VBERROR_TPM_SET_BOOT_MODE_STATE;
			goto VbSelectFirmware_exit;
		}
	}

	/* Success! */
	retval = VBERROR_SUCCESS;

 VbSelectFirmware_exit:

	if (cparams->gbb) {
		VbExFree(cparams->gbb);
		cparams->gbb = NULL;
	}

	/* Save NV storage */
	VbNvTeardown(&vnc);
	if (vnc.raw_changed)
		VbExNvStorageWrite(vnc.raw);

	/* Stop timer */
	shared->timer_vb_select_firmware_exit = VbExGetTimer();

	/* Should always have a known error code */
	VbAssert(VBERROR_UNKNOWN != retval);

	return retval;
}
