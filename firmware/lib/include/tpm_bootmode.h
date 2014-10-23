/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for updating the TPM state with the status of boot path.
 */

#ifndef VBOOT_REFERENCE_TPM_BOOTMODE_H_
#define VBOOT_REFERENCE_TPM_BOOTMODE_H_

#include "gbb_header.h"
#include "sysincludes.h"

/**
 * Update TPM PCR State with the boot path status.
 *
 *  [developer_mode]: State of the developer switch.
 *  [recovery_mode]: State of the recovery mode.
 *  [fw_keyblock_flags]: Keyblock flags of the to-be-booted
 *                       RW firmware keyblock.
 *  [gbb]: Pointer to GBB header from RO firmware.
 *
 * Returns: TPM_SUCCESS if the TPM extend operation succeeds.
 */
uint32_t SetTPMBootModeState(int developer_mode, int recovery_mode,
			     uint64_t fw_keyblock_flags,
			     GoogleBinaryBlockHeader *gbb);

#endif  /* VBOOT_REFERENCE_TPM_BOOTMODE_H_ */
