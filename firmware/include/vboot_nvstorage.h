/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Non-volatile storage routines.
 */

#ifndef VBOOT_REFERENCE_NVSTORAGE_H_
#define VBOOT_REFERENCE_NVSTORAGE_H_

#define NV_BLOCK_SIZE 16  /* Size of NV storage block in bytes */

typedef struct VbNvContext {
  /* Raw NV data.  Caller must fill this before calling VbNvSetup(). */
  uint8_t raw[NV_BLOCK_SIZE];
  /* Flag indicating whether raw data has changed.  Set by VbNvTeardown() if
   * the raw data has changed and needs to be stored to the underlying
   * non-volatile data store. */
  int raw_changed;

  /* Internal data for NV storage routines.  Caller should not touch
   * these fields. */
  int regenerate_crc;

} VbNvContext;


/* Parameter type for VbNvGet(), VbNvSet(). */
typedef enum VbNvParam {
  VBNV_FIRMWARE_SETTINGS_RESET = 0,
  VBNV_KERNEL_SETTINGS_RESET,
  VBNV_DEBUG_RESET_MODE,
  VBNV_TRY_B_COUNT,
  VBNV_RECOVERY_REQUEST,
  VBNV_LOCALIZATION_INDEX,
  VBNV_KERNEL_FIELD,
} VbNvParam;


/* Initialize the NV storage library.  This must be called before any
 * other functions in this library. Returns 0 if success, non-zero if
 * error.
 *
 * If you have access to global variables, you may want to wrap this
 * in your own VbNvOpen() function which allocates a context, acquires
 * a lock to prevent race conditions accessing the underlying storage,
 * reads the raw data from underlying storage, and calls VbNvSetup().
 * We don't do that in here because there are no global variables in
 * UEFI BIOS during PEI phase. */
int VbNvSetup(VbNvContext* context);

/* Clean up and flush changes back to the raw data.  This must be
 * called after other functions in this library.  Caller must check
 * context.raw_changed after calling this function.  Returns 0 if
 * success, non-zero if error.
 *
 * If you have access to global variables, you may want to wrap this
 * in your own VbNvClose() function which calls VbNvTeardown(), writes
 * the underlying storage if context.raw_changed, releases the lock
 * acquired in VbNvOpen, and frees the context. */
int VbNvTeardown(VbNvContext* context);

/* Read a NV storage parameter into *dest.  Returns 0 if success,
 * non-zero if error. */
int VbNvGet(VbNvContext* context, VbNvParam param, uint32_t* dest);

/* Set a NV storage param to a new value.  Returns 0 if success,
 * non-zero if error. */
int VbNvSet(VbNvContext* context, VbNvParam param, uint32_t value);


#endif  /* VBOOT_REFERENCE_NVSTORAGE_H_ */
