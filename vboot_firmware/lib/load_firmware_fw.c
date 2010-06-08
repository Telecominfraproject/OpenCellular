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


static const char kFakeKernelBlob[] = "Fake kernel sign key blob";

void UpdateFirmwareBodyHash(uint8_t* data, uint64_t size) {
  /* TODO: actually update the hash. */
}


int LoadFirmware(LoadFirmwareParams* params) {
  /* TODO: real implementation!  For now, this just chains to the old
   * implementation. */

  uint8_t* fw0;
  uint8_t* fw1;
  uint64_t len;
  int rv;

  /* Get the firmware data */
  fw0 = GetFirmwareBody(0, &len);
  fw1 = GetFirmwareBody(1, &len);

  /* Call the old firmware image driver */
  rv = VerifyFirmwareDriver_f(params->firmware_root_key_blob,
                              params->verification_block_0,
                              fw0,
                              params->verification_block_1,
                              fw1);

  /* Pass back a dummy key blob, since we can't extract the real
   * kernel sign key blob yet */
  params->kernel_sign_key_blob = (void*)kFakeKernelBlob;
  params->kernel_sign_key_size = sizeof(kFakeKernelBlob);

  switch(rv) {
    case BOOT_FIRMWARE_A_CONTINUE:
      params->firmware_index = 0;
      return LOAD_FIRMWARE_SUCCESS;
    case BOOT_FIRMWARE_B_CONTINUE:
      params->firmware_index = 1;
      return LOAD_FIRMWARE_SUCCESS;
  }

  /* If we're still here, we failed */
  return LOAD_FIRMWARE_RECOVERY;

}
