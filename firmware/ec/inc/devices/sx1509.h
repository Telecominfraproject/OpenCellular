/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef SX1509_H_
#define SX1509_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
/* Oscillator frequency source */
#define SX1509_EXTERNAL_CLOCK 1
#define SX1509_INTERNAL_CLOCK_2MHZ 2

/* OSCIO pin function */
#define SX1509_CLOCK_OSC_IN 0
#define SX1509_CLOCK_OSC_OUT 1

/* IO pin definitions */
#define SX1509_IO_PIN_0 0x0001
#define SX1509_IO_PIN_1 0x0002
#define SX1509_IO_PIN_2 0x0004
#define SX1509_IO_PIN_3 0x0008
#define SX1509_IO_PIN_4 0x0010
#define SX1509_IO_PIN_5 0x0020
#define SX1509_IO_PIN_6 0x0040
#define SX1509_IO_PIN_7 0x0080
#define SX1509_IO_PIN_8 0x0001
#define SX1509_IO_PIN_9 0x0002
#define SX1509_IO_PIN_10 0x0004
#define SX1509_IO_PIN_11 0x0008
#define SX1509_IO_PIN_12 0x0010
#define SX1509_IO_PIN_13 0x0020
#define SX1509_IO_PIN_14 0x0040
#define SX1509_IO_PIN_15 0x0080

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Enumeration of SX1509 register types */
typedef enum { SX1509_REG_A = 0, SX1509_REG_B, SX1509_REG_AB } sx1509RegType;

typedef enum {
    SX1509_EDGE_SENSE_REG_LOW = 0,
    SX1509_EDGE_SENSE_REG_HIGH,
    SX1509_EDGE_SENSE_REG_LOW_HIGH
} sx1509EdgeSenseRegType;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus ioexp_led_get_data(const I2C_Dev *i2c_dev, sx1509RegType regType,
                                uint8_t *regValue);
ReturnStatus ioexp_led_set_data(const I2C_Dev *i2c_dev, sx1509RegType regType,
                                uint8_t regValue1, uint8_t regValue2);
ReturnStatus ioexp_led_set_on_time(const I2C_Dev *i2c_dev, uint8_t index,
                                   uint8_t tOnRegValue);
ReturnStatus ioexp_led_set_off_time(const I2C_Dev *i2c_dev, uint8_t index,
                                    uint8_t tOffRegValue);
ReturnStatus ioexp_led_software_reset(const I2C_Dev *i2c_dev);
ReturnStatus ioexp_led_config_inputbuffer(const I2C_Dev *i2c_dev,
                                          sx1509RegType regType,
                                          uint8_t inputBuffRegValue1,
                                          uint8_t inputBuffRegValue2);
ReturnStatus ioexp_led_config_pullup(const I2C_Dev *i2c_dev,
                                     sx1509RegType regType,
                                     uint8_t pullUpRegValue1,
                                     uint8_t pullUpRegValue2);
ReturnStatus ioexp_led_config_pulldown(const I2C_Dev *i2c_dev,
                                       sx1509RegType regType,
                                       uint8_t pullDownRegValue1,
                                       uint8_t pullDownRegValue2);
ReturnStatus ioexp_led_config_opendrain(const I2C_Dev *i2c_dev,
                                        sx1509RegType regType,
                                        uint8_t openDrainRegValue1,
                                        uint8_t openDrainRegValue2);
ReturnStatus ioexp_led_config_data_direction(const I2C_Dev *i2c_dev,
                                             sx1509RegType regType,
                                             uint8_t directionRegValue1,
                                             uint8_t directionRegValue2);
ReturnStatus ioexp_led_config_polarity(const I2C_Dev *i2c_dev,
                                       sx1509RegType regType,
                                       uint8_t polarityRegValue1,
                                       uint8_t polarityRegValue2);
ReturnStatus ioexp_led_config_clock(const I2C_Dev *i2c_dev, uint8_t oscSource,
                                    uint8_t oscPin);
ReturnStatus ioexp_led_config_misc(const I2C_Dev *i2c_dev, uint8_t regValue);
ReturnStatus ioexp_led_enable_leddriver(const I2C_Dev *i2c_dev,
                                        sx1509RegType regType,
                                        uint8_t ledEnableRegValue1,
                                        uint8_t ledEnableRegValue2);
ReturnStatus ioexp_led_read_testregister_1(const I2C_Dev *i2c_dev,
                                           uint8_t *regValue);
ReturnStatus ioexp_led_config_interrupt(const I2C_Dev *i2c_dev,
                                        sx1509RegType regType,
                                        uint8_t interruptMaskRegValue1,
                                        uint8_t interruptMaskRegValue2);
ReturnStatus ioexp_led_config_edge_sense_A(const I2C_Dev *i2c_dev,
                                           sx1509EdgeSenseRegType regType,
                                           uint8_t edgeSenseLowARegValue,
                                           uint8_t edgeSenseHighARegValue);
ReturnStatus ioexp_led_config_edge_sense_A(const I2C_Dev *i2c_dev,
                                           sx1509EdgeSenseRegType regType,
                                           uint8_t edgeSenseLowBRegValue,
                                           uint8_t edgeSenseHighBRegValue);
ReturnStatus ioexp_led_config_edge_sense_B(const I2C_Dev *i2c_dev,
                                           sx1509EdgeSenseRegType regType,
                                           uint8_t edgeSenseLowBRegValue,
                                           uint8_t edgeSenseHighBRegValue);
ReturnStatus ioexp_led_config_debounce_time(const I2C_Dev *i2c_dev,
                                            uint8_t debounceTime);
ReturnStatus ioexp_led_enable_debounce(const I2C_Dev *i2c_dev,
                                       sx1509RegType regType,
                                       uint8_t debounceEnableRegValue1,
                                       uint8_t debounceEnableRegValue2);
ReturnStatus ioexp_led_get_interrupt_source(const I2C_Dev *i2c_dev,
                                            uint16_t *intPins);
ReturnStatus ioexp_led_clear_interrupt_source(const I2C_Dev *i2c_dev);

#endif /* SX1509_H_ */
