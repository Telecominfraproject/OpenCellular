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
#include "load_firmware_fw.h"
#include "vboot_api.h"
#include "vboot_kernel.h"

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

/**
 * Accessors for unit tests only.
 */
VbNvContext *VbApiKernelGetVnc(void);

/**
 * Try to load a kernel.
 */
uint32_t VbTryLoadKernel(VbCommonParams *cparams, LoadKernelParams *p,
                         uint32_t get_info_flags);

/**
 * Ask the user to confirm something.
 *
 * We should display whatever the question is first, then call this. ESC is
 * always "no", ENTER is always "yes", and we'll specify what SPACE means. We
 * don't return until one of those keys is pressed, or until asked to shut
 * down.
 *
 * Returns: 1=yes, 0=no, -1 = shutdown.
 */
int VbUserConfirms(VbCommonParams *cparams, int space_means_no);

/**
 * Handle a normal boot.
 */
VbError_t VbBootNormal(VbCommonParams *cparams, LoadKernelParams *p);

/**
 * Handle a developer-mode boot.
 */
VbError_t VbBootDeveloper(VbCommonParams *cparams, LoadKernelParams *p);

/**
 * Handle a recovery-mode boot.
 */
VbError_t VbBootRecovery(VbCommonParams *cparams, LoadKernelParams *p);

/**
 * Sync EC firmware to expected version.
 */
VbError_t VbEcSoftwareSync(VbCommonParams *cparams);

#endif  /* VBOOT_REFERENCE_VBOOT_KERNEL_H_ */
