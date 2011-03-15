/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for updating the TPM state with the status of boot path.
 */

#ifndef VBOOT_REFERENCE_TPM_BOOTMODE_H_
#define VBOOT_REFERENCE_TPM_BOOTMODE_H_

#include "sysincludes.h"

/* Update TPM PCR State with the boot path status.
 *  [developer_mode]: State of the developer switch.
 *  [recovery_mode}: State of the recovery mode.
 *  [fw_keyblock_flags]: Keyblock flags on the to-be-booted
 *                       RW firmware keyblock.
 *
 *  Returns: TPM_SUCCESS if the TPM extend operation succeeds.
 */

uint32_t SetTPMBootModeState(int developer_mode, int recovery_mode,
                             int fw_keyblock_flags);

#endif  /* VBOOT_REFERENCE_TPM_BOOTMODE_H_ */
