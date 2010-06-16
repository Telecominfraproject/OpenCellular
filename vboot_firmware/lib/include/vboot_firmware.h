/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure and API definitions for a verified boot kernel image.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_VBOOT_FIRMWARE_H_
#define VBOOT_REFERENCE_VBOOT_FIRMWARE_H_

#include <stdint.h>

#include "cgptlib.h"
#include "cryptolib.h"
#include "load_firmware_fw.h"
#include "vboot_common.h"

/* Alternate LoadFirmware() implementation; see load_firmware_fw.h */
int LoadFirmware2(LoadFirmwareParams* params);

#endif  /* VBOOT_REFERENCE_VBOOT_FIRMWARE_H_ */
