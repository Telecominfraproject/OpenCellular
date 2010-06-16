/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware portion)
 */

#include "load_firmware_fw.h"

#include "firmware_image_fw.h"
#include "utility.h"


static const char kFakeKernelBlob[2088] = "Fake kernel sign key blob";


int LoadFirmware(LoadFirmwareParams* params) {
  /* TODO: real implementation!  This is now sufficiently broken due
   * to refactoring that we'll just trust firmware A. */
  Memcpy(params->kernel_sign_key_blob, kFakeKernelBlob,
         sizeof(kFakeKernelBlob));
  params->kernel_sign_key_size = sizeof(kFakeKernelBlob);
  params->firmware_index = 0;
  return LOAD_FIRMWARE_SUCCESS;
}
