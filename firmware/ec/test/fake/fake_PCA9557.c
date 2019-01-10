/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_PCA9557.h"

uint8_t PCA9557_regs[] = {
    [PCA9557_REGS_INPUT_VALUE] = 0x00,  /* Input values */
    [PCA9557_REGS_OUTPUT_VALUE] = 0x00, /* Output values */
    [PCA9557_REGS_POLARITY] = 0x00,     /* Polarity */
    [PCA9557_REGS_DIR_CONFIG] = 0x00,   /* Dir Config */
};
