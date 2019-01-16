/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "include/test_ltc4274.h"

OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

bool LTC4274_GpioPins[] = {
    [OC_EC_PWR_PSE_RESET] = OCGPIO_CFG_OUTPUT,
    [OC_EC_GBC_PSE_ALERT] = OCGPIO_CFG_OUTPUT,
};

uint32_t LTC7274_GpioConfig[] = {
    [OC_EC_PWR_PSE_RESET] = OCGPIO_CFG_OUTPUT,
    [OC_EC_GBC_PSE_ALERT] = OCGPIO_CFG_OUTPUT,
};

LTC4274_Dev l_invalid_dev = {
    .cfg =
        {
            .i2c_dev =
                {
                    .bus = 7,
                    .slave_addr = 0x52,
                },
        },
};

LTC4274_Dev l_invalid_bus = {
    .cfg =
        {
            .i2c_dev =
                {
                    .bus = 3,
                    .slave_addr = 0x2F,
                },
        },
};

uint8_t LTC4274_regs[] = {
    [LTC4274_REG_INTERRUPT_STATUS] = 0x00,
    [LTC4274_REG_INTERRUPT_MASK] = 0x00,
    [LTC4274_REG_POWER_EVENT] = 0x00,
    [LTC4274_REG_POWER_EVENT_COR] = 0x00,
    [LTC4274_REG_DETECT_EVENT] = 0x00,
    [LTC4274_REG_DETECT_EVENT_COR] = 0x00,
    [LTC4274_REG_FAULT_EVENT] = 0x00,
    [LTC4274_REG_FAULT_EVENT_COR] = 0x00,
    [LTC4274_REG_START_EVENT] = 0x00,
    [LTC4274_REG_START_EVENT_COR] = 0x00,
    [LTC4274_REG_SUPPLY_EVENT] = 0x00,
    [LTC4274_REG_SUPPLY_EVENT_COR] = 0x00,
    [LTC4274_REG_STATUS] = 0x00,
    [LTC4274_REG_POWER_STATUS] = 0x00,
    [LTC4274_REG_PNI_STATUS] = 0x00,
    [LTC4274_REG_OPERATION_MODE] = 0x00,
    [LTC4274_REG_ENABLE_DUSCONNECT] = 0x00,
    [LTC4274_REG_DETECT_CLASS_ENABLE] = 0x00,
    [LTC4274_REG_MIDSPAN] = 0x00,
    [LTC4274_REG_MCONF] = 0x00,
    [LTC4274_REG_DETPB] = 0x00,
    [LTC4274_REG_PWRPB] = 0x00,
    [LTC4274_REG_RSTPB] = 0x00,
    [LTC4274_REG_ID] = 0x00,
    [LTC4274_REG_TLIMIT] = 0x00,
    [LTC4274_REG_IP1LSB] = 0x00,
    [LTC4274_REG_IP1MSB] = 0x00,
    [LTC4274_REG_VP1LSB] = 0x00,
    [LTC4274_REG_VP1MSB] = 0x00,
    [LTC4274_REG_FIRMWARE] = 0x00,
    [LTC4274_REG_WDOG] = 0x00,
    [LTC4274_REG_DEVID] = 0x00,
    [LTC4274_REG_HP_ENABLE] = 0x00,
    [LTC4274_REG_HP_MODE] = 0x00,
    [LTC4274_REG_CUT1] = 0x00,
    [LTC4274_REG_LIM1] = 0x00,
    [LTC4274_REG_IHP_STATUS] = 0x00,
};
