/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
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
#include "vboot_struct.h"

typedef struct LoadFirmwareParams {
  /* Inputs to LoadFirmware() */
  void* gbb_data;                /* Pointer to GBB data */
  uint64_t gbb_size;             /* Size of GBB data in bytes */
  void* verification_block_0;    /* Key block + preamble for firmware 0 */
  void* verification_block_1;    /* Key block + preamble for firmware 1 */
  uint64_t verification_size_0;  /* Verification block 0 size in bytes */
  uint64_t verification_size_1;  /* Verification block 1 size in bytes */

  /* Shared data blob for data shared between LoadFirmware() and LoadKernel().
   * This should be at least VB_SHARED_DATA_MIN_SIZE bytes long, and ideally
   * is VB_SHARED_DATA_REC_SIZE bytes long. */
  void* shared_data_blob;        /* Shared data blob buffer.  Pass this
                                  * data to LoadKernel() in
                                  * LoadKernelParams.shared_data_blob. */
  uint32_t shared_data_size;     /* On input, set to size of shared data blob
                                  * buffer, in bytes.  On output, this will
                                  * contain the actual data size placed into
                                  * the buffer.  Caller need only pass that
                                  * much data to LoadKernel().*/

  VbNvContext* nv_context;       /* Context for NV storage.  Caller is
                                  * responsible for calling VbNvSetup() and
                                  * VbNvTeardown() on the context. */

  /* Internal data for LoadFirmware() / UpdateFirmwareBodyHash(). */
  void* load_firmware_internal;

  /* Internal data for caller / GetFirmwareBody(). */
  void* caller_internal;

} LoadFirmwareParams;


/* Functions provided by wrapper to LoadFirmware() */

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


/* Functions provided by vboot_firmware to wrapper */

/* Load the rewritable firmware.
 *
 * Returns VBERROR_SUCCESS if successful.  If unsuccessful, sets a recovery
 * reason via VbNvStorage and returns an error code. */
int LoadFirmware(LoadFirmwareParams* params);


/* Update the data hash for the current firmware image, extending it
 * by [size] bytes stored in [*data].  This function must only be
 * called inside GetFirmwareBody(). */
void UpdateFirmwareBodyHash(LoadFirmwareParams* params,
                            uint8_t* data, uint32_t size);

#endif  /* VBOOT_REFERENCE_LOAD_FIRMWARE_FW_H_ */
