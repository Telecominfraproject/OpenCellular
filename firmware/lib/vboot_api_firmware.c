/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - entry points for init, firmware selection
 */

#include "gbb_header.h"
#include "load_firmware_fw.h"
#include "utility.h"
#include "vboot_api.h"
#include "vboot_common.h"
#include "vboot_nvstorage.h"


VbError_t VbSelectFirmware(VbCommonParams* cparams,
                           VbSelectFirmwareParams* fparams) {
  VbSharedDataHeader* shared = (VbSharedDataHeader*)cparams->shared_data_blob;
  LoadFirmwareParams p;
  VbNvContext vnc;
  int rv;

  /* If recovery is requested, go straight to recovery without checking the
   * RW firmware. */
  if (VBNV_RECOVERY_NOT_REQUESTED != shared->recovery_reason) {
    VBDEBUG(("VbSelectFirmware() detected recovery request, reason=%d.\n",
             (int)shared->recovery_reason));
    fparams->selected_firmware = VB_SELECT_FIRMWARE_RECOVERY;
    return VBERROR_SUCCESS;
  }

  /* Copy parameters from wrapper API structs to old struct */
  p.gbb_data              = cparams->gbb_data;
  p.gbb_size              = cparams->gbb_size;
  p.shared_data_blob      = cparams->shared_data_blob;
  p.shared_data_size      = cparams->shared_data_size;
  p.nv_context            = &vnc;

  /* TODO: LoadFirmware() should use VbSharedDataHeader.flags directly. */
  p.boot_flags = 0;
  if (shared->flags & VBSD_BOOT_DEV_SWITCH_ON)
    p.boot_flags |= BOOT_FLAG_DEVELOPER;

  p.verification_block_0  = fparams->verification_block_A;
  p.verification_block_1  = fparams->verification_block_B;
  p.verification_size_0   = fparams->verification_size_A;
  p.verification_size_1   = fparams->verification_size_B;

  /* Load NV storage */
  VbExNvStorageRead(vnc.raw);
  vnc.raw_changed = 0;

  /* Use vboot_context and caller_internal to link our params with
   * LoadFirmware()'s params. */
  // TODO: clean up LoadFirmware() to use common params?
  p.caller_internal = (void*)cparams;
  cparams->vboot_context = (void*)&p;

  /* Chain to LoadFirmware() */
  rv = LoadFirmware(&p);

  /* Save NV storage, if necessary */
  if (vnc.raw_changed)
    VbExNvStorageWrite(vnc.raw);

  /* Copy amount of used shared data back to the wrapper API struct */
  cparams->shared_data_size = (uint32_t)p.shared_data_size;

  /* Translate return codes */
  if (LOAD_FIRMWARE_SUCCESS == rv) {
    /* Found good firmware in either A or B */
    if (0 == p.firmware_index)
      fparams->selected_firmware = VB_SELECT_FIRMWARE_A;
    else
      fparams->selected_firmware = VB_SELECT_FIRMWARE_B;
    return VBERROR_SUCCESS;

  } else if (LOAD_FIRMWARE_REBOOT == rv) {
    /* Reboot in the same mode we just left; copy the recovery reason */
    VbNvSetup(&vnc);
    VbNvSet(&vnc, VBNV_RECOVERY_REQUEST, shared->recovery_reason);
    VbNvTeardown(&vnc);
    if (vnc.raw_changed)
      VbExNvStorageWrite(vnc.raw);
    return 1;

  } else {
    /* Other error */
    return 1;
  }
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
