/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_EEPROM_1_H
#define _TEST_EEPROM_1_H

#include "common/inc/global/ocmp_frame.h"
#include "drivers/GpioSX1509.h"
#include "drivers/OcGpio.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "inc/devices/eeprom.h"
#include "include/test_sx1509.h"
#include <stdio.h>
#include <string.h>
#include <ti/sysbios/knl/Task.h>
#include "unity.h"

#define EEPROM_ASCII_VAL_SA 0x4153
#define EEPROM_ASCII_VAL_17 0x3731
#define EEPROM_ASCII_VAL_18 0x3831
#define EEPROM_ASCII_VAL_LI 0x494c
#define EEPROM_ASCII_VAL_FE 0x4546
#define EEPROM_ASCII_VAL_3G 0x4733
#define EEPROM_ASCII_VAL_BC 0x4342
#define EEPROM_ASCII_VAL_00 0x3030
#define EEPROM_ASCII_VAL_41 0x3134
#define EEPROM_ASCII_VAL_3S 0x5333
#define EEPROM_ASCII_VAL_DR 0x5244
#define EEPROM_ASCII_VAL_32 0x3233
#define EEPROM_ASCII_VAL_3F 0x4633
#define EEPROM_ASCII_VAL_E0 0x3045
#define EEPROM_ASCII_VAL_05 0x0035
#define EEPROM_ASCII_VAL_C0 0x3043
#define EEPROM_ASCII_VAL_45 0x3534
#define EEPROM_ASCII_VAL_0A 0x4130
#define EEPROM_ASCII_VAL_10 0x3031
#define EEPROM_ASCII_VAL_04 0x3430
#define EEPROM_ASCII_VAL_11 0x3131
#define EEPROM_BIG_WRITE_SIZE 0xCA
#define EEPROM_BOARD_SIZE 36
#define EEPROM_DEFUALT_VALUE_NULL 0x0000
#define EEPROM_DEVICE_SIZE 10
#define EEPROM_DISABLE 0x00
#define EEPROM_DISABLE_WRITE 0xFF
#define EEPROM_ENABLE 0x01
#define EEPROM_ENABLE_WRITE 0xFB
#define EEPROM_FE_BOARD_INFO "SA1718LIFE3FE0005"
#define EEPROM_FE_DEVICE_INFO "SA1718LIFE3FE000580256"
#define EEPROM_GBC_BOARD_INFO "SA1718LIFE3GBC0041"
#define EEPROM_GBC_DEVICE_INFO "SA1718LIFE3GBC004180256"
#define EEPROM_SDR_BOARD_INFO "SA1718LIFE3SDR0032"
#define EEPROM_SDR_DEVICE_INFO "SA1718LIFE3SDR003280256"
#define EEPROM_SERIAL_INFO "SA1718C0450A100411"
#define EEPROM_READ_WRITE_VALUE 0x0505
#define EEPROM_WRITE_SIZE 0x0A

/* ======================== Constants & variables =========================== */
/* Enums are defined as per the code requirment */
typedef enum EEPROMRegs {
    EEPROM_REG_DEF_INIT = 0x000,
    EEPROM_REG_DEVICE_INFO_1 = 0x0A01,
    EEPROM_REG_DEVICE_INFO_2,
    EEPROM_REG_DEVICE_INFO_3,
    EEPROM_REG_DEVICE_INFO_4,
    EEPROM_REG_DEVICE_INFO_5,
    EEPROM_REG_DEVICE_INFO_6,
    EEPROM_REG_DEVICE_INFO_7,
    EEPROM_REG_DEVICE_INFO_8,
    EEPROM_REG_DEVICE_INFO_9,
    EEPROM_REG_BOARD_INFO_1 = 0xAC01,
    EEPROM_REG_BOARD_INFO_2,
    EEPROM_REG_BOARD_INFO_3,
    EEPROM_REG_BOARD_INFO_4,
    EEPROM_REG_BOARD_INFO_5,
    EEPROM_REG_BOARD_INFO_6,
    EEPROM_REG_BOARD_INFO_7,
    EEPROM_REG_BOARD_INFO_8,
    EEPROM_REG_BOARD_INFO_9,
    EEPROM_REG_BOARD_INFO_10,
    EEPROM_REG_SERIAL_INFO_1 = 0xC601,
    EEPROM_REG_SERIAL_INFO_2,
    EEPROM_REG_SERIAL_INFO_3,
    EEPROM_REG_SERIAL_INFO_4,
    EEPROM_REG_SERIAL_INFO_5,
    EEPROM_REG_SERIAL_INFO_6,
    EEPROM_REG_SERIAL_INFO_7,
    EEPROM_REG_SERIAL_INFO_8,
    EEPROM_REG_SERIAL_INFO_9,
    EEPROM_REG_SERIAL_INFO_10,
    EEPROM_REG_FFFF = 0xFFFF,
    EEPROM_REG_END = 0x20000,
} EEPROMRegs;
#endif
