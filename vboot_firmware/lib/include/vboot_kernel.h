/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure and API definitions for a verified boot kernel image.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_VBOOT_KERNEL_H_
#define VBOOT_REFERENCE_VBOOT_KERNEL_H_

#include <stdint.h>

#include "cgptlib.h"
#include "cryptolib.h"
#include "load_kernel_fw.h"
#include "vboot_common.h"

/* TODO: temporary hack */
void FakePartitionAttributes(GptData* gpt);

/* Allocates and reads GPT data from the drive.  The sector_bytes and
 * drive_sectors fields should be filled on input.  The primary and
 * secondary header and entries are filled on output.
 *
 * Returns 0 if successful, 1 if error. */
int AllocAndReadGptData(GptData* gptdata);

/* Writes any changes for the GPT data back to the drive, then frees the
 * buffers. */
void WriteAndFreeGptData(GptData* gptdata);

/* Alternate LoadKernel() implementation; see load_kernel_fw.h */
int LoadKernel2(LoadKernelParams* params);

#endif  /* VBOOT_REFERENCE_VBOOT_KERNEL_H_ */
