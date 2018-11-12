/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _GPIOSX1509_H_
#define _GPIOSX1509_H_

#include "common/inc/global/ocmp_frame.h"
#include "inc/devices/sx1509.h"
#include "OcGpio.h"

#include <ti/sysbios/gates/GateMutex.h>

#include <stdint.h>

#define SX1509_NUM_BANKS 2
#define SX1509_PINS_PER_BANK 8
#define SX1509_PIN_COUNT (SX1509_NUM_BANKS * SX1509_PINS_PER_BANK)

extern const OcGpio_FnTable GpioSX1509_fnTable;

typedef struct SX1509_Cfg {
    I2C_Dev i2c_dev;
    OcGpio_Pin *pin_irq;
} SX1509_Cfg;

/* Private SX1509 driver data */
typedef struct SX1509_Registers {
    uint8_t data;
    uint8_t input_buf_disable;
    uint8_t pull_up;
    uint8_t pull_down;
    uint8_t open_drain;
    uint8_t polarity;
    uint8_t direction;
    uint8_t int_mask;
    uint16_t edge_sense; /*!< Could be split into high and low if needed */
} SX1509_Registers;

/* TODO: possibly dedupe with GpioNative */
typedef struct SX1509CallbackData {
    const OcGpio_Pin *pin;
    OcGpio_CallbackFn callback;
    void *context;
    struct SX1509CallbackData *next; /*!< Pointer to next pin subscriber */
} SX1509CallbackData;

typedef struct SX1509_Obj {
    GateMutex_Handle mutex; /*!< Prevent simultaneous editing of registers */
    SX1509_Registers regs[SX1509_NUM_BANKS];

    SX1509CallbackData *cb_data[SX1509_PIN_COUNT];
} SX1509_Obj;

#endif /* _GPIOSX1509_H_ */
