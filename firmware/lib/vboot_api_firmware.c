/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for init, firmware selection
 */

#include "gbb_header.h"
#include "load_firmware_fw.h"
#include "rollback_index.h"
#include "tpm_bootmode.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"


/* Set recovery request */
static void VbSfRequestRecovery(VbNvContext *vnc, uint32_t recovery_request) {
  VBDEBUG(("VbSfRequestRecovery(%d)\n", (int)recovery_request));
  VbNvSet(vnc, VBNV_RECOVERY_REQUEST, recovery_request);
}


VbError_t VbSelectFirmware(VbCommonParams* cparams,
                           VbSelectFirmwareParams* fparams) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  VbNvContext vnc;
  VbError_t retval = VBERROR_UNKNOWN; /* Assume error until proven successful */
  int is_rec = (shared->recovery_reason ? 1 : 0);
  int is_dev = (shared->flags & VBSD_BOOT_DEV_SWITCH_ON ? 1 : 0);
  uint32_t tpm_version = 0;
  uint32_t tpm_status = 0;

  /* Start timer */
  shared->timer_vb_select_firmware_enter = VbExGetTimer();

  /* Load NV storage */
  VbExNvStorageRead(vnc.raw);
  VbNvSetup(&vnc);

  /* Initialize the TPM */
  VBPERFSTART("VB_TPMI");
  tpm_status = RollbackFirmwareSetup(is_rec, is_dev, &tpm_version);
  VBPERFEND("VB_TPMI");
  if (0 != tpm_status) {
    VBDEBUG(("Unable to setup TPM and read firmware version.\n"));

    if (TPM_E_MUST_REBOOT == tpm_status) {
      /* TPM wants to reboot into the same mode we're in now */
      VBDEBUG(("TPM requires a reboot.\n"));
      if (!is_rec) {
        /* Not recovery mode.  Just reboot (not into recovery). */
        retval = VBERROR_TPM_REBOOT_REQUIRED;
        goto VbSelectFirmware_exit;
      } else if (VBNV_RECOVERY_RO_TPM_REBOOT != shared->recovery_reason) {
        /* In recovery mode now, and we haven't requested a TPM reboot yet,
         * so request one. */
        VbSfRequestRecovery(&vnc, VBNV_RECOVERY_RO_TPM_REBOOT);
        retval = VBERROR_TPM_REBOOT_REQUIRED;
        goto VbSelectFirmware_exit;
      }
    }

    if (!is_rec) {
      VbSfRequestRecovery(&vnc, VBNV_RECOVERY_RO_TPM_ERROR);
      retval = VBERROR_TPM_FIRMWARE_SETUP;
      goto VbSelectFirmware_exit;
    }
  }
  shared->fw_version_tpm_start = tpm_version;
  shared->fw_version_tpm = tpm_version;

  if (is_rec) {
    /* Recovery is requested; go straight to recovery without checking the
     * RW firmware. */
    VBDEBUG(("VbSelectFirmware() detected recovery request, reason=%d.\n",
             (int)shared->recovery_reason));

    /* Go directly to recovery mode */
    fparams->selected_firmware = VB_SELECT_FIRMWARE_RECOVERY;

  } else {
    /* Chain to LoadFirmware() */
    retval = LoadFirmware(cparams, fparams, &vnc);

    /* Exit if we failed to find an acceptable firmware */
    if (VBERROR_SUCCESS != retval)
      goto VbSelectFirmware_exit;

    /* Translate the selected firmware path */
    if (shared->flags & VBSD_LF_USE_RO_NORMAL) {
      /* Request the read-only normal/dev code path */
      fparams->selected_firmware = VB_SELECT_FIRMWARE_READONLY;
    } else if (0 == shared->firmware_index)
      fparams->selected_firmware = VB_SELECT_FIRMWARE_A;
    else
      fparams->selected_firmware = VB_SELECT_FIRMWARE_B;

    /* Update TPM if necessary */
    if (shared->fw_version_tpm_start < shared->fw_version_tpm) {
      VBPERFSTART("VB_TPMU");
      tpm_status = RollbackFirmwareWrite(shared->fw_version_tpm);
      VBPERFEND("VB_TPMU");
      if (0 != tpm_status) {
        VBDEBUG(("Unable to write firmware version to TPM.\n"));
        VbSfRequestRecovery(&vnc, VBNV_RECOVERY_RO_TPM_ERROR);
        retval = VBERROR_TPM_WRITE_FIRMWARE;
        goto VbSelectFirmware_exit;
      }
    }

    /* Lock firmware versions in TPM */
    VBPERFSTART("VB_TPML");
    tpm_status = RollbackFirmwareLock();
    VBPERFEND("VB_TPML");
    if (0 != tpm_status) {
      VBDEBUG(("Unable to lock firmware version in TPM.\n"));
      VbSfRequestRecovery(&vnc, VBNV_RECOVERY_RO_TPM_ERROR);
      retval = VBERROR_TPM_LOCK_FIRMWARE;
      goto VbSelectFirmware_exit;
    }
  }

  /* At this point, we have a good idea of how we are going to
   * boot. Update the TPM with this state information. */
  tpm_status = SetTPMBootModeState(is_dev, is_rec, shared->fw_keyblock_flags);
  if (0 != tpm_status) {
    VBDEBUG(("Unable to update the TPM with boot mode information.\n"));
    if (!is_rec) {
      VbSfRequestRecovery(&vnc, VBNV_RECOVERY_RO_TPM_ERROR);
      retval = VBERROR_TPM_SET_BOOT_MODE_STATE;
      goto VbSelectFirmware_exit;
    }
  }

  /* Success! */
  retval = VBERROR_SUCCESS;

VbSelectFirmware_exit:

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
