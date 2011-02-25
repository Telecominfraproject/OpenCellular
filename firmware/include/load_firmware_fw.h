/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_LOAD_FIRMWARE_FW_H_
#define VBOOT_REFERENCE_LOAD_FIRMWARE_FW_H_

#include "sysincludes.h"
#include "vboot_nvstorage.h"

/* Recommended size of kernel_sign_key_blob in bytes, for
 * implementations which must preallocate a transfer buffer between
 * boot phases */
#define LOAD_FIRMWARE_KEY_BLOB_REC_SIZE 2104

/* Return codes for LoadFirmware() and S3Resume(). */
#define LOAD_FIRMWARE_SUCCESS 0   /* Success */
#define LOAD_FIRMWARE_RECOVERY 1  /* Reboot to recovery mode.  The specific
                                   * recovery reason has been set in
                                   * VbNvContext (VBNV_RECOVERY_REQUEST). */
#define LOAD_FIRMWARE_REBOOT 2    /* Reboot to same mode as current boot */

/* Boot flags for LoadFirmware().boot_flags */
#define BOOT_FLAG_DEVELOPER UINT64_C(0x01)  /* Developer switch is on */

typedef struct LoadFirmwareParams {
  /* Inputs to LoadFirmware() */
  void *firmware_root_key_blob;  /* Key used to sign firmware header */
  void *verification_block_0;    /* Key block + preamble for firmware 0 */
  void *verification_block_1;    /* Key block + preamble for firmware 1 */
  uint64_t verification_size_0;  /* Verification block 0 size in bytes */
  uint64_t verification_size_1;  /* Verification block 1 size in bytes */
  void *kernel_sign_key_blob;    /* Destination buffer for key to use
                                  * when loading kernel.  Pass this
                                  * data to LoadKernel() in
                                  * LoadKernelParams.header_sign_key_blob. */
  uint64_t kernel_sign_key_size; /* Size of kernel signing key blob
                                  * buffer, in bytes.  On output, this
                                  * will contain the actual key blob
                                  * size placed into the buffer. */
  uint64_t boot_flags;           /* Boot flags */
  VbNvContext* nv_context;       /* Context for NV storage.  nv_context->raw
                                  * must be filled before calling
                                  * LoadFirmware().  On output, check
                                  * nv_context->raw_changed to see if
                                  * nv_context->raw has been modified and
                                  * needs saving. */

  /* Outputs from LoadFirmware(); valid only if LoadFirmware() returns
   * LOAD_FIRMWARE_SUCCESS. */
  uint64_t firmware_index;       /* Firmware index to run. */

  /* Internal data for LoadFirmware() / UpdateFirmwareBodyHash(). */
  void* load_firmware_internal;

  /* Internal data for caller / GetFirmwareBody(). */
  void* caller_internal;

} LoadFirmwareParams;


/* Functions provided by PEI to LoadFirmware() */

/* Get the firmware body data for [firmware_index], which is either
 * 0 (the first firmware image) or 1 (the second firmware image).
 *
 * This function must call UpdateFirmwareBodyHash() before returning,
 * to update the secure hash for the firmware image.  For best
 * performance, the reader should call this function periodically
 * during the read, so that updating the hash can be pipelined with
 * the read.  If the reader cannot update the hash during the read
 * process, it should call UpdateFirmwareBodyHash() on the entire
 * firmeware data after the read, before returning.
 *
 * Returns 0 if successful or non-zero if error. */
int GetFirmwareBody(LoadFirmwareParams* params, uint64_t firmware_index);


/* Functions provided by verified boot library to PEI */

/* Early setup for LoadFirmware().  This should be called as soon as the TPM
 * is available in the boot process.
 *
 * Returns LOAD_FIRMWARE_SUCCESS if successful, error code on failure. */
int LoadFirmwareSetup(void);

/* Attempts to load the rewritable firmware.
 *
 * Returns LOAD_FIRMWARE_SUCCESS if successful, error code on failure. */
int LoadFirmware(LoadFirmwareParams* params);


/* Update the data hash for the current firmware image, extending it
 * by [size] bytes stored in [*data].  This function must only be
 * called inside GetFirmwareBody(). */
void UpdateFirmwareBodyHash(LoadFirmwareParams* params,
                            uint8_t* data, uint64_t size);

/* Handle S3 resume.
 *
 * Returns LOAD_FIRMWARE_SUCCESS if successful, error code on failure. */
int S3Resume(void);

#endif  /* VBOOT_REFERENCE_LOAD_FIRMWARE_FW_H_ */
