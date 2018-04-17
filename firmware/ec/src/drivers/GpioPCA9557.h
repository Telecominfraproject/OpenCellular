/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef _GPIOPCA9557_H_
#define _GPIOPCA9557_H_

#include "OcGpio.h"

#include "inc/common/i2cbus.h"

#include <ti/sysbios/gates/GateMutex.h>

#include <stdint.h>

extern const OcGpio_FnTable GpioPCA9557_fnTable;

typedef struct PCA9557_Cfg {
    I2C_Dev i2c_dev;
} PCA9557_Cfg;

/* Private PCA9557 driver data */
typedef struct PCA9557_Obj {
    GateMutex_Handle mutex; /*!< Prevent simultaneous editing of registers */

    /* Cached register values */
    uint8_t reg_output;
    uint8_t reg_polarity;
    uint8_t reg_config;
} PCA9557_Obj;

#endif /* _GPIOPCA9557_H_ */
