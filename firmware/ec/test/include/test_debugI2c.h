/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_DEBUG_I2C_H
#define _TEST_DEBUG_I2C_H

#include "common/inc/ocmp_wrappers/ocmp_debugi2c.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "inc/devices/debug_oci2c.h"
#include <string.h>
#include <ti/sysbios/knl/Task.h>
#include "unity.h"

#define DEBUG_I2C_DEFAULT_VALUE 0x00
#define DEBUG_I2C_INVALID_BUS 10
#define DEBUG_I2C_INVALID_SLAVE_ADDR 0x48
#define DEBUG_I2C_NO_OF_BYTES 1
#define DEBUG_I2C_READ_WRITE_VALUE 0x5A
#define DEBUG_I2C_REG_ADDR 0x01
#define DEBUG_I2C_REG_VALUE 12
#define DEBUG_I2C_SLAVE_ADDR 0x2F

typedef enum DEBUGI2CRegs {
    DEBUG_I2C_INTERRUPT_STATUS = 0x00,
    DEBUG_I2C_INTERRUPT_MASK,
    DEBUG_I2C_END = 0xFF,
} DEBUGI2CRegs;

#endif
