/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_ina226.h"

OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};
INA226_Dev ina226_invalid_dev = {
    .cfg =
        {
            .dev =
                {
                    .bus = 4,
                    .slave_addr = 0x02,
                },
        },
};

INA226_Dev ina226_invalid_bus = {
    .cfg =
        {
            .dev =
                {
                    .bus = 0xFF,
                    .slave_addr = 0x01,
                },
        },
};

uint16_t INA226_regs[] = {
    [INA226_CONFIG_REG] = 0x0000,      [INA226_SHUNT_VOLT_REG] = 0x0000,
    [INA226_BUS_VOLT_REG] = 0x0000,    [INA226_POWER_REG] = 0x0000,
    [INA226_CURRENT_REG] = 0x0000,     [INA226_CALIBRATION_REG] = 0x0000,
    [INA226_MASK_ENABLE_REG] = 0x0000, [INA226_ALERT_REG] = 0x0000,
    [INA226_MANF_ID_REG] = 0x0000,     [INA226_DIE_ID_REG] = 0x0000,
    [INA226_END_REG] = 0x0000,
};

bool INA226_GpioPins[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};

uint32_t INA226_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};
