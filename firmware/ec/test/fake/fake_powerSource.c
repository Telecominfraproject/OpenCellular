/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_powerSource.h"
#include <stdint.h>

extern const OcGpio_FnTable GpioSX1509_fnTable;

bool PWR_GpioPins[] = {
    [OC_EC_PWR_PRSNT_SOLAR_AUX] = PWR_STATE_ENABLE,
    [OC_EC_PWR_PRSNT_POE] = PWR_STATE_ENABLE,
};

uint32_t PWR_GpioConfig[] = {
    [OC_EC_PWR_PRSNT_SOLAR_AUX] = OCGPIO_CFG_INPUT,
    [OC_EC_PWR_PRSNT_POE] = OCGPIO_CFG_INPUT,
};

/* ============================= Boilerplate ================================ */
OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

OcGpio_Port gbc_io_0 = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
        &(SX1509_Cfg){
            .i2c_dev = { I2C_BUS, I2C_ADDR },
        },
    .object_data = &(SX1509_Obj){},
};
