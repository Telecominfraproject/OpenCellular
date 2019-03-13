/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_INA226_H
#define _TEST_INA226_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "inc/devices/ina226.h"
#include <string.h>
#include "unity.h"
/* ======================== Constants & variables =========================== */
#define INA226_ALERT_MASK 0x8000
#define INA226_CAL_VALUE 0
#define INA226_DEFAULT_ACTION 4
#define INA226_DEFAULT_TEMP 600
#define INA226_DEVICE_ID 0x2260
#define INA226_INVALID_CONFIG_PARAMID 1
#define INA226_INVALID_DEVICE_ID 0xC802
#define INA226_INVALID_MANF_ID 0x5DC7
#define INA226_INVALID_STATUS_PARAMID 4
#define INA226_LOWER_BOUND_VAL -345
#define INA226_MANF_ID 0x5449
#define INA226_POST_DATA_NULL 0x00
#define INA226_RESET 0x0001
#define INA226_REG_VALUE 0x0960
#define INA226_UPPER_BOUND_VAL 65535

typedef enum INA226Regs {
    INA226_CONFIG_REG = 0x0000,
    INA226_SHUNT_VOLT_REG,
    INA226_BUS_VOLT_REG,
    INA226_POWER_REG,
    INA226_CURRENT_REG,
    INA226_CALIBRATION_REG,
    INA226_MASK_ENABLE_REG,
    INA226_ALERT_REG,
    INA226_MANF_ID_REG = 0xFE,
    INA226_DIE_ID_REG = 0xFF,
    INA226_END_REG = 0xFFFF,
} INA226Regs;

uint16_t ina226_stub_set_currentLimit(uint16_t currentLimt);
uint16_t ina226_stub_get_currentLimit(uint16_t currentLimt);
uint16_t ina226_stub_get_busVlotage_status(uint16_t busVoltage);
uint16_t ina226_stub_get_shuntVlotage_status(uint16_t shuntVoltage);
uint16_t ina226_stub_get_current_status(uint16_t current);
uint16_t ina226_stub_get_power_status(uint16_t power);
#endif
