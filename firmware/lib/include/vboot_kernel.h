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
#include "gpt_misc.h"
#include "load_firmware_fw.h"
#include "load_kernel_fw.h"
#include "vboot_api.h"

/**
 * Accessors for unit tests only.
 */
VbNvContext *VbApiKernelGetVnc(void);

/**
 * Exported for unit tests only - frees memory used by VbSelectAndLoadKernel()
 */
void VbApiKernelFree(VbCommonParams *cparams);

/**
 * Try to load a kernel.
 */
uint32_t VbTryLoadKernel(VbCommonParams *cparams, LoadKernelParams *p,
                         uint32_t get_info_flags);

/* Flags for VbUserConfirms() */
#define VB_CONFIRM_MUST_TRUST_KEYBOARD (1 << 0)
#define VB_CONFIRM_SPACE_MEANS_NO      (1 << 1)

/**
 * Ask the user to confirm something.
 *
 * We should display whatever the question is first, then call this. ESC is
 * always "no", ENTER is always "yes", and we'll specify what SPACE means. We
 * don't return until one of those keys is pressed, or until asked to shut
 * down.
 *
 * Additionally, in some situations we don't accept confirmations from an
 * untrusted keyboard (such as a USB device).  In those cases, a recovery
 * button press is needed for confirmation, instead of ENTER.
 *
 * Returns: 1=yes, 0=no, -1 = shutdown.
 */
int VbUserConfirms(VbCommonParams *cparams, uint32_t confirm_flags);

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
 * Sync EC device <devidx> firmware to expected version.
 */
VbError_t VbEcSoftwareSync(int devidx, VbCommonParams *cparams);

#endif  /* VBOOT_REFERENCE_VBOOT_KERNEL_H_ */
