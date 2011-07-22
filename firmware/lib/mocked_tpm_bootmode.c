/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for updating the TPM state with the status of boot path.
 */

#include "tpm_bootmode.h"

#include "tss_constants.h"


uint32_t SetTPMBootModeState(int developer_mode, int recovery_mode,
                             uint64_t fw_keyblock_flags) {
  return TPM_SUCCESS;
}
