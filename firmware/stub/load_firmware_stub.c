/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * TEMPORARY stub for calling LoadFirmware() which looks like the old
 * VerifyFirmwareDriver_f() call.
 * (Firmware portion)
 */

#include "load_firmware_fw.h"
#include "utility.h"

#define  BOOT_FIRMWARE_A_CONTINUE 1
#define  BOOT_FIRMWARE_B_CONTINUE 2
#define  BOOT_FIRMWARE_RECOVERY_CONTINUE 3

typedef struct CallerInternal {
  uint8_t *firmwareA;
  uint64_t firmwareA_size;
  uint8_t *firmwareB;
  uint64_t firmwareB_size;
} CallerInternal;

int GetFirmwareBody(LoadFirmwareParams* params, uint64_t index) {

  CallerInternal* ci = (CallerInternal*)params->caller_internal;
  uint8_t *fw;
  uint64_t size;

  /* In a real implementation, GetFirmwareBody() should be what reads
   * and decompresses the firmware volume.  In this temporary hack, we
   * just pass the pointer which we got from
   * VerifyFirmwareDriver_Stub(). */
  switch(index) {
    case 0:
      size = ci->firmwareA_size;
      fw = ci->firmwareA;
      break;

    case 1:
      size = ci->firmwareB_size;
      fw = ci->firmwareB;
      break;

    default:
      /* Anything else is invalid */
      return 1;
  }

  /* Need to call UpdateFirmwareBodyHash() with the firmware volume
   * data.  In this temporary hack, the FV is already decompressed, so
   * we pass in the entire volume at once.  In a real implementation,
   * you should call this as the FV is being decompressed. */
  UpdateFirmwareBodyHash(params, fw, size);

  /* Success */
  return 0;
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

  CallerInternal ci;
  LoadFirmwareParams p;
  VbNvContext vnc;

  /* TODO: YOU SHOULD CALL LoadFirmwareSetup() AS SOON AS THE TPM
   * INTERFACE IS AVAILABLE */
  LoadFirmwareSetup();

  /* Copy the firmware volume pointers to our global variables. */
  ci.firmwareA = firmwareA;
  ci.firmwareB = firmwareB;

  /* TODO: YOU NEED TO PASS IN THE FIRMWARE VOLUME SIZES SOMEHOW */
  ci.firmwareA_size = 0;
  ci.firmwareB_size = 0;

  /* TODO: YOU NEED TO LOAD vnc.raw[] FROM NON-VOLATILE STORAGE */

  /* Set up the params for LoadFirmware() */
  p.caller_internal = &ci;
  p.firmware_root_key_blob = root_key_blob;
  p.verification_block_0 = verification_headerA;
  p.verification_block_1 = verification_headerB;
  p.nv_context = &vnc;

  /* Allocate a key blob buffer */
  p.kernel_sign_key_blob = Malloc(LOAD_FIRMWARE_KEY_BLOB_REC_SIZE);
  p.kernel_sign_key_size = LOAD_FIRMWARE_KEY_BLOB_REC_SIZE;

  /* TODO: YOU NEED TO SET THE BOOT FLAGS SOMEHOW */
  p.boot_flags = 0;

  /* Call LoadFirmware() */
  rv = LoadFirmware(&p);

  if (vnc.raw_changed) {
    /* TODO: YOU NEED TO SAVE vnc.raw TO NON-VOLATILE STORAGE */
  }

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
