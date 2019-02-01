/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_PCA9557_H
#define _TEST_PCA9557_H

#include "drivers/GpioPCA9557.h"
#include <stdint.h>

typedef enum PCA9557Regs {
    PCA9557_REGS_INPUT_VALUE = 0x00,
    PCA9557_REGS_OUTPUT_VALUE,
    PCA9557_REGS_POLARITY,
    PCA9557_REGS_DIR_CONFIG,
    PCA9557_REGS_END = 0x10,
} PCA9557Regs;
#endif
