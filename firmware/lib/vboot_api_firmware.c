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
  VbNvSetup(vnc);
  VbNvSet(vnc, VBNV_RECOVERY_REQUEST, recovery_request);
  VbNvTeardown(vnc);
  if (vnc->raw_changed)
    VbExNvStorageWrite(vnc->raw);
}


VbError_t VbSelectFirmware(VbCommonParams* cparams,
                           VbSelectFirmwareParams* fparams) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  LoadFirmwareParams p;
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
  vnc.raw_changed = 0;

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
    /* Check the RW firmware */
    /* Copy parameters from wrapper API structs to old struct */
    p.gbb_data              = cparams->gbb_data;
    p.gbb_size              = cparams->gbb_size;
    p.shared_data_blob      = cparams->shared_data_blob;
    p.shared_data_size      = cparams->shared_data_size;
    p.nv_context            = &vnc;

    p.verification_block_0  = fparams->verification_block_A;
    p.verification_block_1  = fparams->verification_block_B;
    p.verification_size_0   = fparams->verification_size_A;
    p.verification_size_1   = fparams->verification_size_B;

    /* Use vboot_context and caller_internal to link our params with
     * LoadFirmware()'s params. */
    // TODO: clean up LoadFirmware() to use common params?
    p.caller_internal = (void*)cparams;
    cparams->vboot_context = (void*)&p;

    /* Chain to LoadFirmware() */
    retval = LoadFirmware(&p);

    /* Save NV storage, if necessary */
    if (vnc.raw_changed)
      VbExNvStorageWrite(vnc.raw);

    /* Copy amount of used shared data back to the wrapper API struct */
    cparams->shared_data_size = (uint32_t)p.shared_data_size;

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

  /* Stop timer */
  shared->timer_vb_select_firmware_exit = VbExGetTimer();

  /* Should always have a known error code */
  VbAssert(VBERROR_UNKNOWN != retval);

  return retval;
}


/* TODO: Move this inside vboot_firmware.c; for now this just translates to
 * the original function call. */
void VbUpdateFirmwareBodyHash(VbCommonParams* cparams, uint8_t* data,
                              uint32_t size) {
  LoadFirmwareParams* lfparams = (LoadFirmwareParams*)cparams->vboot_context;

  UpdateFirmwareBodyHash(lfparams, data, size);
}


/* Translation layer from LoadFirmware()'s GetFirmwareBody() to the new
 * wrapper API call.
 *
 * TODO: call directly from LoadFirmware() */
int GetFirmwareBody(LoadFirmwareParams* lfparams, uint64_t index) {
  VbCommonParams* cparams = (VbCommonParams*)lfparams->caller_internal;
  VbError_t rv;

  rv = VbExHashFirmwareBody(cparams, (index ? VB_SELECT_FIRMWARE_B :
                                      VB_SELECT_FIRMWARE_A));
  return (VBERROR_SUCCESS == rv ? 0 : 1);
}
