/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Display functions used in kernel selection.
 */

#ifndef VBOOT_REFERENCE_VBOOT_DISPLAY_H_
#define VBOOT_REFERENCE_VBOOT_DISPLAY_H_

#include "vboot_api.h"
#include "vboot_nvstorage.h"

VbError_t VbDisplayScreenFromGBB(VbCommonParams* cparams, uint32_t screen,
                                 VbNvContext *vncptr);
VbError_t VbDisplayScreen(VbCommonParams* cparams, uint32_t screen, int force,
                          VbNvContext *vncptr);
VbError_t VbDisplayDebugInfo(VbCommonParams* cparams, VbNvContext *vncptr);
VbError_t VbCheckDisplayKey(VbCommonParams* cparams, uint32_t key,
                            VbNvContext *vncptr);

#endif /* VBOOT_REFERENCE_VBOOT_DISPLAY_H_ */

