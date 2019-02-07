/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_debugocgpio.h"

OcGpio_Port gbc_io_1 = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
        &(SX1509_Cfg){
            .i2c_dev = { OC_CONNECT1_I2C0, GBC_IO_1_SLAVE_ADDR },
            .pin_irq = NULL, /* This IO expander doesn't provide interrupts */
        },
    .object_data = &(SX1509_Obj){},
};

OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

OcGpio_Port sdr_fx3_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
        &(PCA9557_Cfg){
            .i2c_dev = { OC_CONNECT1_I2C3, SDR_FX3_IOEXP_ADDRESS },
        },
    .object_data = &(PCA9557_Obj){},
};

OcGpio_Port sdr_fx3_io_invalid = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
        &(PCA9557_Cfg){
            .i2c_dev = { OC_CONNECT1_I2C4, SDR_FX3_IO_INVALID_SLAVE_ADDR },
        },
    .object_data = &(PCA9557_Obj){},
};

S_OCGPIO_Cfg debug_sdr_ioexpanderx1E_invalid = {
    .port = &sdr_fx3_io_invalid,
    .group = 1,
};

bool DEBUG_GpioPins[] = {
    [DEBUG_GPIO_PIN_1] = OCGPIO_CFG_INPUT,
    [DEBUG_GPIO_PIN_2] = OCGPIO_CFG_INPUT,
};

uint32_t DEBUG_GpioConfig[] = {
    [DEBUG_GPIO_PIN_1] = OCGPIO_CFG_INPUT,
    [DEBUG_GPIO_PIN_2] = OCGPIO_CFG_INPUT,
};

S_OCGPIO s_fake_pin = {
    .pin = DEBUG_GPIO_S_FAKE_PIN,
};

S_OCGPIO s_sx1509_invalid_pin = {
    .pin = DEBUG_GPIO_SX1509_INALID_PIN,
};

S_OCGPIO s_pca9557_invalid_pin = {
    .pin = DEBUG_GPIO_PCA9557_INVALID_PIN,
};
