/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_POWERSOURCE_H
#define _TEST_POWERSOURCE_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_powersource.h"
#include "drivers/GpioSX1509.h"
#include "drivers/OcGpio.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "inc/devices/powerSource.h"
#include "include/test_sx1509.h"
#include <string.h>
#include "unity.h"
/* ======================== Constants & variables =========================== */
#define I2C_ADDR 0x71
#define I2C_BUS 5
#define OC_EC_PWR_INVALID_IO 0x82
#define OC_EC_PWR_INVALID_PIN 0x57
#define OC_EC_PWR_PRSNT_POE 0x55
#define OC_EC_PWR_PRSNT_SOLAR_AUX 0x1E
#define PWR_EXT_BAT_ENABLE_FIRST_BYTE 0x08
#define PWR_EXT_BAT_ENABLE_SECOND_BYTE 0x00
#define PWR_INT_BAT_ENABLE_FIRST_BYTE 0x10
#define PWR_INT_BAT_ENABLE_SECOND_BYTE 0x00
#define PWR_INT_EXT_BAT_DISABLE_FIRST_BYTE 0x18
#define PWR_INT_EXT_BAT_DISABLE_SECOND_BYTE 0x00
#define PWR_STATE_DISABLE 0x01
#define PWR_STATE_ENABLE 0x00
#define PWR_STATE_INVALID_PARAM 0x09

typedef enum ePowerSourceStatus {
    PWR_SRC_NOT_AVAILABLE = 0,
    PWR_SRC_ACTIVE_AVAILABLE,
} ePowerSourceStatus;

#endif
