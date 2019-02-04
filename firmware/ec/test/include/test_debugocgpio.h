/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_DEBUGOCGPIO_H
#define _TEST_DEBUGOCGPIO_H

#include "common/inc/global/OC_CONNECT1.h"
#include "common/inc/ocmp_wrappers/ocmp_debugocgpio.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "inc/devices/debug_ocgpio.h"
#include "include/test_PCA9557.h"
#include "include/test_sx1509.h"
#include "src/drivers/GpioSX1509.h"
#include "src/drivers/GpioPCA9557.h"
#include <string.h>
#include "ti/drivers/gpio/GPIOTiva.h"
#include <ti/sysbios/knl/Task.h>
#include "unity.h"

#define DEBUG_GPIO_DEFAULT_VALUE 0x00
#define DEBUG_GPIO_PCA9557_INVALID_PIN 8
#define DEBUG_GPIO_PIN_VALUE 1
#define DEBUG_GPIO_PIN_1 0x01
#define DEBUG_GPIO_PIN_2 0x02
#define DEBUG_GPIO_S_FAKE_PIN 2
#define DEBUG_GPIO_SX1509_DATA_A_VALUE 0xff
#define DEBUG_GPIO_SX1509_DATA_B_VALUE 0x02
#define DEBUG_GPIO_SX1509_INALID_PIN 16
#define GBC_IO_1_SLAVE_ADDR 0x45
#define SDR_FX3_IO_INVALID_SLAVE_ADDR 0xFF
#endif
