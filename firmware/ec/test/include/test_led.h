/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_LED_H
#define _TEST_LED_H

#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "inc/devices/led.h"
#include "inc/devices/sx1509.h"
#include "include/test_sx1509.h"
#include <string.h>
#include <ti/sysbios/knl/Task.h>
#include "unity.h"

#define LED_CLOCK_VALUE 0x40
#define LED_DEFAULT_VALUE 0x00
#define LED_DRIVER_ENABLE_B_VALUE 0xFF
#define LED_INVALID_PARAM 3
#define LED_POST_DATA_NULL 0x00
#define LED_POST_DEVID 0xFF
#define LED_POST_MANID 0xFF
#define LED_REG_INPUT_DISABLE_B_VALUE 0xFF
#define LED_REG_OPEN_DRAIN_B_VALUE 0xFF
#define LED_REG_PULL_UP_B_VALUE 0x00
#define LED_SYSTEM_BOOT_DATAB 0xD5
#define INPUT_BUFFER_DISABLE 0x01
#define OC_CONNECT1_I2C8 7
#define OC_EC_HCI_LED_RESET 89
#define SX1509_SOFT_RESET_REG_VALUE_2 0x34
#define REG_T_ON_14_15_VALUE 0x00
#endif
