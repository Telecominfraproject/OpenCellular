/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TEMPORARY stub for calling LoadFirmware() which looks like the old
 * VerifyFirmwareDriver_f() call.
 * (Firmware portion)
 */

#include "load_firmware_fw.h"

#include "firmware_image_fw.h"
#include "utility.h"


static uint8_t *g_firmwareA;
static uint64_t g_firmwareA_size;
static uint8_t *g_firmwareB;
static uint64_t g_firmwareB_size;


void *GetFirmwareBody(uint64_t firmware_index, uint64_t* size) {

  uint8_t *fw;

  /* In a real implementation, GetFirmwareBody() should be what reads
   * and decompresses the firmware volume.  In this temporary hack, we
   * just pass the pointer which we got from
   * VerifyFirmwareDriver_Stub(). */
  switch(firmware_index) {
    case 0:
      *size = g_firmwareA_size;
      fw = g_firmwareA;
    case 1:
      *size = g_firmwareB_size;
      fw = g_firmwareB;
    default:
      /* Anything else is invalid */
      *size = 0;
      return NULL;
  }

  /* Need to call UpdateFirmwareBodyHash() with the firmware volume
   * data.  In this temporary hack, the FV is already decompressed, so
   * we pass in the entire volume at once.  In a real implementation,
   * you should call this as the FV is being decompressed. */
  UpdateFirmwareBodyHash(fw, *size);

  /* Return the firmware body pointer */
  return fw;
}


/* Where you're currently calling VerifyFirmwareDriver_f(), call this
 * function instead.  Because you still need to read in both firmware
 * volumes, this call will still be slow.  Once we reach feature
 * complete, you should modify your code to call LoadImage()
 * directly. */
int VerifyFirmwareDriver_stub(uint8_t* root_key_blob,
                              uint8_t* verification_headerA,
                              uint8_t* firmwareA,
                              uint8_t* verification_headerB,
                              uint8_t* firmwareB) {

  int rv;

  /* Copy the firmware volume pointers to our global variables. */
  g_firmwareA = firmwareA;
  g_firmwareB = firmwareB;

  /* TODO: YOU NEED TO PASS IN THE FIRMWARE VOLUME SIZES SOMEHOW */
  g_firmwareA_size = 0;
  g_firmwareB_size = 0;

  /* Set up the params for LoadFirmware() */
  LoadFirmwareParams p;
  p.firmware_root_key_blob = root_key_blob;
  p.verification_block_0 = verification_headerA;
  p.verification_block_1 = verification_headerB;

  /* Call LoadFirmware() */
  rv = LoadFirmware(&p);
  if (LOAD_FIRMWARE_SUCCESS == rv) {
    /* TODO: YOU NEED TO KEEP TRACK OF p.kernel_sign_key_blob AND
     * p.kernel_sign_key_size SO YOU CAN PASS THEM TO LoadKernel(). */

    return (0 == p.firmware_index ? BOOT_FIRMWARE_A_CONTINUE :
            BOOT_FIRMWARE_B_CONTINUE);

  } else {
    /* Error */
    return BOOT_FIRMWARE_RECOVERY_CONTINUE;
  }
}
