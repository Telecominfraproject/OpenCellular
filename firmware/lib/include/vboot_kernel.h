/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure and API definitions for a verified boot kernel image.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_VBOOT_KERNEL_H_
#define VBOOT_REFERENCE_VBOOT_KERNEL_H_

#include "cgptlib.h"
#include "vboot_api.h"

/**
 * Allocate and read GPT data from the drive.  The sector_bytes and
 * drive_sectors fields should be filled on input.  The primary and secondary
 * header and entries are filled on output.
 *
 * Returns 0 if successful, 1 if error.
 */
int AllocAndReadGptData(VbExDiskHandle_t disk_handle, GptData *gptdata);

/**
 * Write any changes for the GPT data back to the drive, then free the buffers.
 */
int WriteAndFreeGptData(VbExDiskHandle_t disk_handle, GptData *gptdata);

#endif  /* VBOOT_REFERENCE_VBOOT_KERNEL_H_ */
