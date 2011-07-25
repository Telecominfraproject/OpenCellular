/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for init, firmware selection
 */

#include "gbb_header.h"
#include "load_firmware_fw.h"
#include "rollback_index.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"


VbError_t VbInit(VbCommonParams* cparams, VbInitParams* iparams) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  VbNvContext vnc;
  VbError_t retval = VBERROR_SUCCESS;
  uint32_t recovery = VBNV_RECOVERY_NOT_REQUESTED;
  int is_s3_resume = 0;
  uint32_t s3_debug_boot = 0;

  VBDEBUG(("VbInit() input flags 0x%x\n", iparams->flags));

  /* Initialize output flags */
  iparams->out_flags = 0;

  /* Set up NV storage */
  VbExNvStorageRead(vnc.raw);
  VbNvSetup(&vnc);

  /* Initialize shared data structure */
  if (0 != VbSharedDataInit(shared, cparams->shared_data_size)) {
    VBDEBUG(("Shared data init error\n"));
    return VBERROR_INIT_SHARED_DATA;
  }

  shared->timer_vb_init_enter = VbExGetTimer();

  /* Copy boot switch flags */
  shared->flags = 0;
  if (iparams->flags & VB_INIT_FLAG_DEV_SWITCH_ON)
    shared->flags |= VBSD_BOOT_DEV_SWITCH_ON;
  if (iparams->flags & VB_INIT_FLAG_REC_BUTTON_PRESSED)
    shared->flags |= VBSD_BOOT_REC_SWITCH_ON;
  if (iparams->flags & VB_INIT_FLAG_WP_ENABLED)
    shared->flags |= VBSD_BOOT_FIRMWARE_WP_ENABLED;
  if (iparams->flags & VB_INIT_FLAG_S3_RESUME)
    shared->flags |= VBSD_BOOT_S3_RESUME;
  if (iparams->flags & VB_INIT_FLAG_RO_NORMAL_SUPPORT)
    shared->flags |= VBSD_BOOT_RO_NORMAL_SUPPORT;

  is_s3_resume = (iparams->flags & VB_INIT_FLAG_S3_RESUME ? 1 : 0);

  /* Check if the OS is requesting a debug S3 reset */
  VbNvGet(&vnc, VBNV_DEBUG_RESET_MODE, &s3_debug_boot);
  if (s3_debug_boot) {
    if (is_s3_resume) {
      VBDEBUG(("VbInit() requesting S3 debug boot\n"));
      iparams->out_flags |= VB_INIT_OUT_S3_DEBUG_BOOT;
      is_s3_resume = 0;         /* Proceed as if this is a normal boot */
    }

    /* Clear the request even if this is a normal boot, since we don't
     * want the NEXT S3 resume to be a debug reset unless the OS
     * asserts the request again. */
    VbNvSet(&vnc, VBNV_DEBUG_RESET_MODE, 0);
  }

  /* If this isn't a S3 resume, read the current recovery request, then clear
   * it so we don't get stuck in recovery mode. */
  if (!is_s3_resume) {
    VbNvGet(&vnc, VBNV_RECOVERY_REQUEST, &recovery);
    if (VBNV_RECOVERY_NOT_REQUESTED != recovery)
      VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, VBNV_RECOVERY_NOT_REQUESTED);
  }

  /* If the previous boot failed in the firmware somewhere outside of verified
   * boot, and recovery is not requested for our own reasons, request recovery
   * mode.  This gives the calling firmware a way to request recovery if it
   * finds something terribly wrong. */
  if (VBNV_RECOVERY_NOT_REQUESTED == recovery &&
      iparams->flags & VB_INIT_FLAG_PREVIOUS_BOOT_FAIL) {
    recovery = VBNV_RECOVERY_RO_FIRMWARE;
  }

  /* If recovery button is pressed, override recovery reason.  Note that we
   * do this in the S3 resume path also. */
  if (iparams->flags & VB_INIT_FLAG_REC_BUTTON_PRESSED)
    recovery = VBNV_RECOVERY_RO_MANUAL;

  /* Set output flags */
  if (VBNV_RECOVERY_NOT_REQUESTED != recovery) {
    /* Requesting recovery mode */
    iparams->out_flags |= (VB_INIT_OUT_ENABLE_RECOVERY |
                          VB_INIT_OUT_CLEAR_RAM |
                          VB_INIT_OUT_ENABLE_DISPLAY |
                          VB_INIT_OUT_ENABLE_USB_STORAGE);
  }
  else if (iparams->flags & VB_INIT_FLAG_DEV_SWITCH_ON) {
    /* Developer switch is on, so need to support dev mode */
    iparams->out_flags |= (VB_INIT_OUT_CLEAR_RAM |
                          VB_INIT_OUT_ENABLE_DISPLAY |
                          VB_INIT_OUT_ENABLE_USB_STORAGE);
  }

  /* Copy current recovery reason to shared data */
  shared->recovery_reason = (uint8_t)recovery;

  /* If this is a S3 resume, resume the TPM */
  if (is_s3_resume) {
    if (TPM_SUCCESS != RollbackS3Resume()) {
      /* If we can't resume, just do a full reboot.  No need to go to recovery
       * mode here, since if the TPM is really broken we'll catch it on the
       * next boot. */
      retval = VBERROR_TPM_S3_RESUME;
    }
  }

  /* Tear down NV storage */
  VbNvTeardown(&vnc);
  if (vnc.raw_changed)
    VbExNvStorageWrite(vnc.raw);

  VBDEBUG(("VbInit() output flags 0x%x\n", iparams->out_flags));

  shared->timer_vb_init_exit = VbExGetTimer();

  return retval;
}
