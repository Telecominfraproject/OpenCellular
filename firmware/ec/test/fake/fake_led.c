/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_led.h"

const I2C_Dev s_sx1509_left_dev = {
    .bus = OC_CONNECT1_I2C8,
    .slave_addr = LED_SX1509_LEFT_ADDRESS,
};

const I2C_Dev s_sx1509_right_dev = {
    .bus = OC_CONNECT1_I2C8,
    .slave_addr = LED_SX1509_RIGHT_ADDRESS,
};

OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

uint8_t SX1509_right_regs[] = {
    [SX1509_REG_TEST_2] = 0x00,
};

uint8_t LED_GpioPins[] = {
    [INPUT_BUFFER_DISABLE] = 0x00,
    [OC_EC_HCI_LED_RESET] = 0x00,
};

uint32_t LED_GpioConfig[] = {
    [INPUT_BUFFER_DISABLE] = 0x00,
    [OC_EC_HCI_LED_RESET] = 0x00,
};
