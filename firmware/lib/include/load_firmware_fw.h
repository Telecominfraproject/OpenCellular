/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying rewritable firmware.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_LOAD_FIRMWARE_FW_H_
#define VBOOT_REFERENCE_LOAD_FIRMWARE_FW_H_

#include "vboot_api.h"
#include "vboot_nvstorage.h"
#include "vboot_struct.h"

/**
 * Load the rewritable firmware.
 *
 * Pass the common and firmware params from VbSelectFirmware(), and a
 * VbNvContext.  Caller is responsible for calling VbNvSetup() and
 * VbNvTeardown() on the VbNvContext.
 *
 * Returns VBERROR_SUCCESS if successful.  If unsuccessful, sets a recovery
 * reason via VbNvStorage and returns an error code.
 */
int LoadFirmware(VbCommonParams *cparams, VbSelectFirmwareParams *fparams,
		 VbNvContext *vnc);

#endif  /* VBOOT_REFERENCE_LOAD_FIRMWARE_FW_H_ */
