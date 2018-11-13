/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_LTC4015_H
#define _TEST_LTC4015_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"
#include "drivers/GpioSX1509.h"
#include "drivers/OcGpio.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "inc/devices/ltc4015.h"
#include <string.h>
#include <math.h>
#include "unity.h"

/* ======================== Constants & variables =========================== */

extern const OcGpio_FnTable GpioSX1509_fnTable;

typedef enum LTC4015Status {
    LTC4015_STATUS_BATTERY_VOLTAGE = 0,
    LTC4015_STATUS_BATTERY_CURRENT,
    LTC4015_STATUS_SYSTEM_VOLTAGE,
    LTC4015_STATUS_INPUT_VOLATGE,
    LTC4015_STATUS_INPUT_CURRENT,
    LTC4015_STATUS_DIE_TEMPERATURE,
    LTC4015_STATUS_ICHARGE_DAC
} LTC4015Status;

typedef enum LTC4015Config {
    LTC4015_CONFIG_BATTERY_VOLTAGE_LOW = 0,
    LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
    LTC4015_CONFIG_BATTERY_CURRENT_LOW,
    LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
    LTC4015_CONFIG_INPUT_CURRENT_HIGH,
    LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
    LTC4015_CONFIG_ICHARGE,
    LTC4015_CONFIG_VCHARGE,
    LTC4015_CONFIG_DIE_TEMPERATURE_HIGH,
} LTC4015Config;

static const LTC4015_Config fact_lithiumIon_cfg = {
    .batteryVoltageLow = 9500,
    .batteryVoltageHigh = 12600,
    .batteryCurrentLow = 100,
    .inputVoltageLow = 16200,
    .inputCurrentHigh = 5000,
    .inputCurrentLimit = 5570,
};

static const LTC4015_Config fact_leadAcid_cfg = {
    .batteryVoltageLow = 9500,
    .batteryVoltageHigh = 13800,
    .batteryCurrentLow = 100,
    .inputVoltageLow = 16200,
    .inputCurrentHigh = 17000,
    .inputCurrentLimit = 16500,
    .icharge = 10660,
    .vcharge = 12000,
};
#endif
