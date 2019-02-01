/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_RFWATCHDOG_H
#define _TEST_RFWATCHDOG_H

#include "common/inc/global/Framework.h"
#include "common/inc/global/OC_CONNECT1.h"
#include "common/inc/ocmp_wrappers/ocmp_rfwatchdog.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "include/test_PCA9557.h"
#include "src/drivers/GpioPCA9557.h"
#include <string.h>
#include "unity.h"

#define RFWATCHDOG_CH1_DIR_CONFIG_VALUE 0x0C
#define RFWATCHDOG_CH2_DIR_CONFIG_VALUE 0x3C
#define RFWATCHDOG_DIR_CONFIG_DEFAULT_VALUE 0x00
#define RFWATCHDOG_PIN_1 0x01
#define RFWATCHDOG_PIN_2 0x02

#endif
