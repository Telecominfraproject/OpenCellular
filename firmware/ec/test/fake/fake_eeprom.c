/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_eeprom.h"
#include <stdint.h>

uint16_t EEPROM_regs[] = {
    [EEPROM_REG_DEF_INIT] = 0x0000,      [EEPROM_REG_DEVICE_INFO_1] = 0x0000,
    [EEPROM_REG_DEVICE_INFO_2] = 0x0000, [EEPROM_REG_DEVICE_INFO_3] = 0x0000,
    [EEPROM_REG_DEVICE_INFO_4] = 0x0000, [EEPROM_REG_DEVICE_INFO_5] = 0x0000,
    [EEPROM_REG_DEVICE_INFO_6] = 0x0000, [EEPROM_REG_DEVICE_INFO_7] = 0x0000,
    [EEPROM_REG_DEVICE_INFO_8] = 0x0000, [EEPROM_REG_DEVICE_INFO_9] = 0x0000,
    [EEPROM_REG_BOARD_INFO_1] = 0x0000,  [EEPROM_REG_BOARD_INFO_2] = 0x0000,
    [EEPROM_REG_BOARD_INFO_3] = 0x0000,  [EEPROM_REG_BOARD_INFO_4] = 0x0000,
    [EEPROM_REG_BOARD_INFO_5] = 0x0000,  [EEPROM_REG_BOARD_INFO_6] = 0x0000,
    [EEPROM_REG_BOARD_INFO_7] = 0x0000,  [EEPROM_REG_BOARD_INFO_8] = 0x0000,
    [EEPROM_REG_BOARD_INFO_9] = 0x0000,  [EEPROM_REG_BOARD_INFO_10] = 0x0000,
    [EEPROM_REG_SERIAL_INFO_1] = 0x0000, [EEPROM_REG_SERIAL_INFO_2] = 0x0000,
    [EEPROM_REG_SERIAL_INFO_3] = 0x0000, [EEPROM_REG_SERIAL_INFO_4] = 0x0000,
    [EEPROM_REG_SERIAL_INFO_5] = 0x0000, [EEPROM_REG_SERIAL_INFO_6] = 0x0000,
    [EEPROM_REG_SERIAL_INFO_7] = 0x0000, [EEPROM_REG_SERIAL_INFO_8] = 0x0000,
    [EEPROM_REG_SERIAL_INFO_9] = 0x0000, [EEPROM_REG_SERIAL_INFO_10] = 0x0000,
    [EEPROM_REG_FFFF] = 0x0000,          [EEPROM_REG_END] = 0x0000,
};

Eeprom_Cfg e_invalid_dev = {
    .i2c_dev =
        {
            .bus = 6,
            .slave_addr = 0xFF,
        },
};

Eeprom_Cfg e_invalid_bus = {
    .i2c_dev =
        {
            .bus = 0xFF,
            .slave_addr = 0x50,
        },
};

Eeprom_Cfg *e_invalid_cfg = NULL;

bool Eeprom_GpioPins[] = {
    [0x01] = OCGPIO_CFG_INPUT, /* Pin = 1 */
    [0x02] = OCGPIO_CFG_INPUT, /* Pin = 2 */
};

uint32_t Eeprom_GpioConfig[] = {
    [0x01] = OCGPIO_CFG_INPUT,
    [0x02] = OCGPIO_CFG_INPUT,
};

OcGpio_Port gbc_io_0 = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
        &(SX1509_Cfg){
            .i2c_dev = { 6, 0x45 },
            .pin_irq = NULL,
        },
    .object_data = &(SX1509_Obj){},
};

OcGpio_Port fe_ch1_lna_io = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
        &(SX1509_Cfg){
            .i2c_dev = { 6, 0x45 },
            .pin_irq = NULL,
        },
    .object_data = &(SX1509_Obj){},
};

static OcGpio_Pin pin_inven_eeprom_wp = { &gbc_io_0, 2, 32 };

Eeprom_Cfg enable_dev = {
    .i2c_dev = { 6, 0x45 },
    .pin_wp = &pin_inven_eeprom_wp,
    .type = { .page_size = 64, .mem_size = (256 / 8) },
    .ss = OC_SS_SYS,
};

Eeprom_Cfg e_invalid_ss_cfg = {
    .i2c_dev = { 4, 0x50 },
    .pin_wp = &(OcGpio_Pin){ &gbc_io_0, 5 },
    .type = { .page_size = 64, .mem_size = (256 / 8) },
    .ss = OC_SS_PWR,
};
