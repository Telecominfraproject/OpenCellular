/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "inc/devices/sx1509.h"

#include "inc/common/global_header.h"
#include "inc/common/byteorder.h"

/*****************************************************************************
 *                          REGISTER DEFINITIONS
 *****************************************************************************/
#define SX1509_REG_INPUT_DISABLE_B \
    0x00 /* Input buffer disable register I/O[15..8] (Bank B) */
#define SX1509_REG_INPUT_DISABLE_A \
    0x01 /* Input buffer disable register I/O[7..0] (Bank A) */
#define SX1509_REG_LONG_SLEW_B \
    0x02 /* Output buffer long slew register I/O[15..8] (Bank B) */
#define SX1509_REG_LONG_SLEW_A \
    0x03 /* Output buffer long slew register I/O[7..0] (Bank A) */
#define SX1509_REG_LOW_DRIVE_B \
    0x04 /* Output buffer low drive register I/O[15..8] (Bank B) */
#define SX1509_REG_LOW_DRIVE_A \
    0x05 /* Output buffer low drive register I/O[7..0] (Bank A) */
#define SX1509_REG_PULL_UP_B 0x06 /* Pull_up register I/O[15..8] (Bank B) */
#define SX1509_REG_PULL_UP_A 0x07 /* Pull_up register I/O[7..0] (Bank A) */
#define SX1509_REG_PULL_DOWN_B 0x08 /* Pull_down register I/O[15..8] (Bank B) */
#define SX1509_REG_PULL_DOWN_A 0x09 /* Pull_down register I/O[7..0] (Bank A) */
#define SX1509_REG_OPEN_DRAIN_B \
    0x0A /* Open drain register I/O[15..8] (Bank B) */
#define SX1509_REG_OPEN_DRAIN_A \
    0x0B /* Open drain register I/O[7..0] (Bank A) */
#define SX1509_REG_POLARITY_B 0x0C /* Polarity register I/O[15..8] (Bank B) */
#define SX1509_REG_POLARITY_A 0x0D /* Polarity register I/O[7..0] (Bank A) */
#define SX1509_REG_DIR_B 0x0E /* Direction register I/O[15..8] (Bank B) */
#define SX1509_REG_DIR_A 0x0F /* Direction register I/O[7..0] (Bank A) */
#define SX1509_REG_DATA_B 0x10 /* Data register I/O[15..8] (Bank B) */
#define SX1509_REG_DATA_A 0x11 /* Data register I/O[7..0] (Bank A) */
#define SX1509_REG_INTERRUPT_MASK_B \
    0x12 /* Interrupt mask register I/O[15..8] (Bank B) */
#define SX1509_REG_INTERRUPT_MASK_A \
    0x13 /* Interrupt mask register I/O[7..0] (Bank A) */
#define SX1509_REG_SENSE_HIGH_B \
    0x14 /* Sense register for I/O[15:12] (Bank B) */
#define SX1509_REG_SENSE_LOW_B 0x15 /* Sense register for I/O[11:8] (Bank B) */
#define SX1509_REG_SENSE_HIGH_A 0x16 /* Sense register for I/O[7:4] (Bank A) */
#define SX1509_REG_SENSE_LOW_A 0x17 /* Sense register for I/O[3:0] (Bank A) */
#define SX1509_REG_INTERRUPT_SOURCE_B \
    0x18 /* Interrupt source register I/O[15..8] (Bank B) */
#define SX1509_REG_INTERRUPT_SOURCE_A \
    0x19 /* Interrupt source register I/O[7..0] (Bank A) */
#define SX1509_REG_EVENT_STATUS_B \
    0x1A /* Event status register I/O[15..8] (Bank B) */
#define SX1509_REG_EVENT_STATUS_A \
    0x1B /* Event status register I/O[7..0] (Bank A) */
#define SX1509_REG_LEVEL_SHIFTER_1 0x1C /* Level shifter register 1 */
#define SX1509_REG_LEVEL_SHIFTER_2 0x1D /* Level shifter register 2 */
#define SX1509_REG_CLOCK 0x1E /* Clock management register */
#define SX1509_REG_MISC 0x1F /* Miscellaneous device settings register */
#define SX1509_REG_LED_DRIVER_ENABLE_B \
    0x20 /* LED driver enable register I/O[15..8] (Bank B) */
#define SX1509_REG_LED_DRIVER_ENABLE_A \
    0x21 /* LED driver enable register I/O[7..0] (Bank A) */
#define SX1509_REG_DEBOUNCE_CONFIG 0x22 /* Debounce configuration register */
#define SX1509_REG_DEBOUNCE_ENABLE_B \
    0x23 /* Debounce enable register I/O[15..8] (Bank B) */
#define SX1509_REG_DEBOUNCE_ENABLE_A \
    0x24 /* Debounce enable register I/O[7..0] (Bank A) */
#define SX1509_REG_T_ON_0 0x29 /* ON time register for I/O[0] */
#define SX1509_REG_I_ON_0 0x2A /* ON intensity register for I/O[0] */
#define SX1509_REG_OFF_0 0x2B /* OFF time/intensity register for I/O[0] */
#define SX1509_REG_T_ON_1 0x2C /* ON time register for I/O[1] */
#define SX1509_REG_I_ON_1 0x2D /* ON intensity register for I/O[1] */
#define SX1509_REG_OFF_1 0x2E /* OFF time/intensity register for I/O[1] */
#define SX1509_REG_T_ON_2 0x2F /* ON time register for I/O[2] */
#define SX1509_REG_I_ON_2 0x30 /* ON intensity register for I/O[2] */
#define SX1509_REG_OFF_2 0x31 /* OFF time/intensity register for I/O[2] */
#define SX1509_REG_T_ON_3 0x32 /* ON time register for I/O[3] */
#define SX1509_REG_I_ON_3 0x33 /* ON intensity register for I/O[3] */
#define SX1509_REG_OFF_3 0x34 /* OFF time/intensity register for I/O[3] */
#define SX1509_REG_T_ON_4 0x35 /* ON time register for I/O[4] */
#define SX1509_REG_I_ON_4 0x36 /* ON intensity register for I/O[4] */
#define SX1509_REG_OFF_4 0x37 /* OFF time/intensity register for I/O[4] */
#define SX1509_REG_T_RISE_4 0x38 /* Fade in register for I/O[4] */
#define SX1509_REG_T_FALL_4 0x39 /* Fade out register for I/O[4] */
#define SX1509_REG_T_ON_5 0x3A /* ON time register for I/O[5] */
#define SX1509_REG_I_ON_5 0x3B /* ON intensity register for I/O[5] */
#define SX1509_REG_OFF_5 0x3C /* OFF time/intensity register for I/O[5] */
#define SX1509_REG_T_RISE_5 0x3D /* Fade in register for I/O[5] */
#define SX1509_REG_T_FALL_5 0x3E /* Fade out register for I/O[5] */
#define SX1509_REG_T_ON_6 0x3F /* ON time register for I/O[6] */
#define SX1509_REG_I_ON_6 0x40 /* ON intensity register for I/O[6] */
#define SX1509_REG_OFF_6 0x41 /* OFF time/intensity register for I/O[6] */
#define SX1509_REG_T_RISE_6 0x42 /* Fade in register for I/O[6] */
#define SX1509_REG_T_FALL_6 0x43 /* Fade out register for I/O[6] */
#define SX1509_REG_T_ON_7 0x44 /* ON time register for I/O[7] */
#define SX1509_REG_I_ON_7 0x45 /* ON intensity register for I/O[7] */
#define SX1509_REG_OFF_7 0x46 /* OFF time/intensity register for I/O[7] */
#define SX1509_REG_T_RISE_7 0x47 /* Fade in register for I/O[7] */
#define SX1509_REG_T_FALL_7 0x48 /* Fade out register for I/O[7] */
#define SX1509_REG_T_ON_8 0x49 /* ON time register for I/O[8] */
#define SX1509_REG_I_ON_8 0x4A /* ON intensity register for I/O[8] */
#define SX1509_REG_OFF_8 0x4B /* OFF time/intensity register for I/O[8] */
#define SX1509_REG_T_ON_9 0x4C /* ON time register for I/O[9] */
#define SX1509_REG_I_ON_9 0x4D /* ON intensity register for I/O[9] */
#define SX1509_REG_OFF_9 0x4E /* OFF time/intensity register for I/O[9] */
#define SX1509_REG_T_ON_10 0x4F /* ON time register for I/O[10] */
#define SX1509_REG_I_ON_10 0x50 /* ON intensity register for I/O[10] */
#define SX1509_REG_OFF_10 0x51 /* OFF time/intensity register for I/O[10] */
#define SX1509_REG_T_ON_11 0x52 /* ON time register for I/O[11] */
#define SX1509_REG_I_ON_11 0x53 /* ON intensity register for I/O[11] */
#define SX1509_REG_OFF_11 0x54 /* OFF time/intensity register for I/O[11] */
#define SX1509_REG_T_ON_12 0x55 /* ON time register for I/O[12] */
#define SX1509_REG_I_ON_12 0x56 /* ON intensity register for I/O[12] */
#define SX1509_REG_OFF_12 0x57 /* OFF time/intensity register for I/O[12] */
#define SX1509_REG_T_RISE_12 0x58 /* Fade in register for I/O[12] */
#define SX1509_REG_T_FALL_12 0x59 /* Fade out register for I/O[12] */
#define SX1509_REG_T_ON_13 0x5A /* ON time register for I/O[13] */
#define SX1509_REG_I_ON_13 0x5B /* ON intensity register for I/O[13] */
#define SX1509_REG_OFF_13 0x5C /* OFF time/intensity register for I/O[13] */
#define SX1509_REG_T_RISE_13 0x5D /* Fade in register for I/O[13] */
#define SX1509_REG_T_FALL_13 0x5E /* Fade out register for I/O[13] */
#define SX1509_REG_T_ON_14 0x5F /* ON time register for I/O[14] */
#define SX1509_REG_I_ON_14 0x60 /* ON intensity register for I/O[14] */
#define SX1509_REG_OFF_14 0x61 /* OFF time/intensity register for I/O[14] */
#define SX1509_REG_T_RISE_14 0x62 /* Fade in register for I/O[14] */
#define SX1509_REG_T_FALL_14 0x63 /* Fade out register for I/O[14] */
#define SX1509_REG_T_ON_15 0x64 /* ON time register for I/O[15] */
#define SX1509_REG_I_ON_15 0x65 /* ON intensity register for I/O[15] */
#define SX1509_REG_OFF_15 0x66 /* OFF time/intensity register for I/O[15] */
#define SX1509_REG_T_RISE_15 0x67 /* Fade in register for I/O[15] */
#define SX1509_REG_T_FALL_15 0x68 /* Fade out register for I/O[15] */
#define SX1509_REG_HIGH_INPUT_B \
    0x69 /* High input enable register I/O[15..8] (Bank B) */
#define SX1509_REG_HIGH_INPUT_A \
    0x6A /* High input enable register I/O[7..0] (Bank A) */
#define SX1509_REG_RESET 0x7D /* Software reset register */
#define SX1509_REG_TEST_1 0x7E /* Test register 1 */
#define SX1509_REG_TEST_2 0x7F /* Test register 2 */

/* Values being used for Soft reset of SX1509 */
#define SX1509_SOFT_RESET_REG_VALUE_1 0x12
#define SX1509_SOFT_RESET_REG_VALUE_2 0x34

static uint8_t SX1509_REG_T_ON[16] = { SX1509_REG_T_ON_0,  SX1509_REG_T_ON_1,
                                       SX1509_REG_T_ON_2,  SX1509_REG_T_ON_3,
                                       SX1509_REG_T_ON_4,  SX1509_REG_T_ON_5,
                                       SX1509_REG_T_ON_6,  SX1509_REG_T_ON_7,
                                       SX1509_REG_T_ON_8,  SX1509_REG_T_ON_9,
                                       SX1509_REG_T_ON_10, SX1509_REG_T_ON_11,
                                       SX1509_REG_T_ON_12, SX1509_REG_T_ON_13,
                                       SX1509_REG_T_ON_14, SX1509_REG_T_ON_15 };

static uint8_t SX1509_REG_OFF[16] = {
    SX1509_REG_OFF_0,  SX1509_REG_OFF_1,  SX1509_REG_OFF_2,  SX1509_REG_OFF_3,
    SX1509_REG_OFF_4,  SX1509_REG_OFF_5,  SX1509_REG_OFF_6,  SX1509_REG_OFF_7,
    SX1509_REG_OFF_8,  SX1509_REG_OFF_9,  SX1509_REG_OFF_10, SX1509_REG_OFF_11,
    SX1509_REG_OFF_12, SX1509_REG_OFF_13, SX1509_REG_OFF_14, SX1509_REG_OFF_15
};

#if 0
static uint8_t SX1509_REG_I_ON[16] = {
        SX1509_REG_I_ON_0, SX1509_REG_I_ON_1, SX1509_REG_I_ON_2, SX1509_REG_I_ON_3,
        SX1509_REG_I_ON_4, SX1509_REG_I_ON_5, SX1509_REG_I_ON_6, SX1509_REG_I_ON_7,
        SX1509_REG_I_ON_8, SX1509_REG_I_ON_9, SX1509_REG_I_ON_10, SX1509_REG_I_ON_11,
        SX1509_REG_I_ON_12, SX1509_REG_I_ON_13, SX1509_REG_I_ON_14, SX1509_REG_I_ON_15 };
#endif

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_raw_read
 **
 **    DESCRIPTION     : Read the register value from IO Expander with LED
 **                      driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register address and pointer
 **                      to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus ioexp_led_raw_read(const I2C_Dev *i2c_dev,
                                       uint8_t regAddress, uint8_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle sx1509_handle = i2c_get_handle(i2c_dev->bus);
    uint16_t value = 0x0000;
    if (!sx1509_handle) {
        LOGGER_ERROR("SX1509:ERROR:: Failed to get I2C Bus for SX1509 0x%02x "
                     "on bus 0x%02x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        status = i2c_reg_read(sx1509_handle, i2c_dev->slave_addr, regAddress,
                              &value, 1);
        if (status == RETURN_OK) {
            *regValue = (uint8_t)(value);
        }
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_raw_write
 **
 **    DESCRIPTION     : Write the register value(s) to IO Expander with LED
 **                      driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register address, value 1
 **                      & value 2 to be written and No of bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus ioexp_led_raw_write(const I2C_Dev *i2c_dev,
                                        uint8_t regAddress, uint8_t regValue1,
                                        uint8_t regValue2, uint8_t noOfBytes)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle sx1509_handle = i2c_get_handle(i2c_dev->bus);
    uint16_t value = 0x00;
    if (noOfBytes == 2) {
        value = (regValue2 << 8) | (regValue1);
        value = htobe16(value);
    } else {
        value = regValue1;
    }
    if (!sx1509_handle) {
        LOGGER_ERROR("SX1509:ERROR:: Failed to get I2C Bus for SX1509 0x%02x "
                     "on bus 0x%02x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        status = i2c_reg_write(sx1509_handle, i2c_dev->slave_addr, regAddress,
                               value, noOfBytes);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_get_data
 **
 **    DESCRIPTION     : Read the Data Register Value from IO Expander with LED
 **                      driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type and pointer
 **                      to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_get_data(const I2C_Dev *i2c_dev, sx1509RegType regType,
                                uint8_t *regValue)
{
    ReturnStatus status = RETURN_OK;
    uint8_t regAddress = (regType == SX1509_REG_A) ? (SX1509_REG_DATA_A) :
                                                     (SX1509_REG_DATA_B);
    status = ioexp_led_raw_read(i2c_dev, regAddress, regValue);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_set_data
 **
 **    DESCRIPTION     : Write the Data Register Value(s) into IO Expander with
 **                      LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_set_data(const I2C_Dev *i2c_dev, sx1509RegType regType,
                                uint8_t regValue1, uint8_t regValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ? (SX1509_REG_DATA_A) :
                                                     (SX1509_REG_DATA_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, regValue1, regValue2,
                                 noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_set_on_time
 **
 **    DESCRIPTION     : Write the ON Time Register Value into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, index to On time Register
 **                      and value to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_set_on_time(const I2C_Dev *i2c_dev, uint8_t index,
                                   uint8_t tOnRegValue)
{
    ReturnStatus status = RETURN_OK;
    status = ioexp_led_raw_write(i2c_dev, SX1509_REG_T_ON[index], tOnRegValue,
                                 0, 1);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_set_off_time
 **
 **    DESCRIPTION     : Write the OFF Time Register Value into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, index to Off time Register
 **                      and value to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_set_off_time(const I2C_Dev *i2c_dev, uint8_t index,
                                    uint8_t tOffRegValue)
{
    ReturnStatus status = RETURN_OK;
    status = ioexp_led_raw_write(i2c_dev, SX1509_REG_OFF[index], tOffRegValue,
                                 0, 1);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_software_reset
 **
 **    DESCRIPTION     : Do LED SX159 Soft Reset by writing 0x12 followed by
 **                      0x34 on software reset register.
 **
 **    ARGUMENTS       : Subsystem and Slave address.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_software_reset(const I2C_Dev *i2c_dev)
{
    ReturnStatus status = RETURN_OK;

    status = ioexp_led_raw_write(i2c_dev, SX1509_REG_RESET,
                                 SX1509_SOFT_RESET_REG_VALUE_1, 0, 1);
    if (status == RETURN_OK) {
        status = ioexp_led_raw_write(i2c_dev, SX1509_REG_RESET,
                                     SX1509_SOFT_RESET_REG_VALUE_2, 0, 1);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_inputbuffer
 **
 **    DESCRIPTION     : Write the Input Diable Input Buffer Register Value(s)
 **                      into IO Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_inputbuffer(const I2C_Dev *i2c_dev,
                                          sx1509RegType regType,
                                          uint8_t inputBuffRegValue1,
                                          uint8_t inputBuffRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ?
                                 (SX1509_REG_INPUT_DISABLE_A) :
                                 (SX1509_REG_INPUT_DISABLE_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, inputBuffRegValue1,
                                 inputBuffRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_pullup
 **
 **    DESCRIPTION     : Write the Pull_up Register Value(s) into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_pullup(const I2C_Dev *i2c_dev,
                                     sx1509RegType regType,
                                     uint8_t pullUpRegValue1,
                                     uint8_t pullUpRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ? (SX1509_REG_PULL_UP_A) :
                                                     (SX1509_REG_PULL_UP_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, pullUpRegValue1,
                                 pullUpRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_pulldown
 **
 **    DESCRIPTION     : Write the Pull Down Register Value(s) into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_pulldown(const I2C_Dev *i2c_dev,
                                       sx1509RegType regType,
                                       uint8_t pullDownRegValue1,
                                       uint8_t pullDownRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ? (SX1509_REG_PULL_DOWN_A) :
                                                     (SX1509_REG_PULL_DOWN_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, pullDownRegValue1,
                                 pullDownRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_opendrain
 **
 **    DESCRIPTION     : Write the Open drain Register Value(s) into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_opendrain(const I2C_Dev *i2c_dev,
                                        sx1509RegType regType,
                                        uint8_t openDrainRegValue1,
                                        uint8_t openDrainRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ? (SX1509_REG_OPEN_DRAIN_A) :
                                                     (SX1509_REG_OPEN_DRAIN_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, openDrainRegValue1,
                                 openDrainRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_data_direction
 **
 **    DESCRIPTION     : Write the Direction Register Value(s) into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_data_direction(const I2C_Dev *i2c_dev,
                                             sx1509RegType regType,
                                             uint8_t directionRegValue1,
                                             uint8_t directionRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress =
            (regType == SX1509_REG_A) ? (SX1509_REG_DIR_A) : (SX1509_REG_DIR_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, directionRegValue1,
                                 directionRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_polarity
 **
 **    DESCRIPTION     : Write the Polarity Register Value(s) into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_polarity(const I2C_Dev *i2c_dev,
                                       sx1509RegType regType,
                                       uint8_t polarityRegValue1,
                                       uint8_t polarityRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ? (SX1509_REG_POLARITY_A) :
                                                     (SX1509_REG_POLARITY_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, polarityRegValue1,
                                 polarityRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_clock
 **
 **    DESCRIPTION     : Write the Clock management Register Value into IO
 **                      Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address and value to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
/* RegClock:
 * 6:5 - Oscillator frequency souce
 *        00: off, 01: external input, 10: internal 2MHz, 11: reserved
 *   4 - OSCIO pin function
 *         0: input, 1 ouptut
 * 3:0 - Frequency of oscout pin
 *         0: LOW, 0xF: high, else fOSCOUT = FoSC/(2^(RegClock[3:0]-1))
 */
ReturnStatus ioexp_led_config_clock(const I2C_Dev *i2c_dev, uint8_t oscSource,
                                    uint8_t oscPin)
{
    ReturnStatus status = RETURN_OK;
    uint8_t regValue = 0;

    regValue = oscSource << 5;
    regValue |= oscPin << 4;

    status = ioexp_led_raw_write(i2c_dev, SX1509_REG_CLOCK, regValue, 0, 1);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_misc
 **
 **    DESCRIPTION     : Write the Miscellaneous device settings Register Value
 **                      into IO Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address and value to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_misc(const I2C_Dev *i2c_dev, uint8_t regValue)
{
    ReturnStatus status = RETURN_OK;
    status = ioexp_led_raw_write(i2c_dev, SX1509_REG_MISC, regValue, 0, 1);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_enable_leddriver
 **
 **    DESCRIPTION     : Write the LED driver enable Value(s) into IO Expander
 **                      with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_enable_leddriver(const I2C_Dev *i2c_dev,
                                        sx1509RegType regType,
                                        uint8_t ledEnableRegValue1,
                                        uint8_t ledEnableRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ?
                                 (SX1509_REG_LED_DRIVER_ENABLE_A) :
                                 (SX1509_REG_LED_DRIVER_ENABLE_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, ledEnableRegValue1,
                                 ledEnableRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_read_testregister_1
 **
 **    DESCRIPTION     : Read the Test Register Value from IO Expander with LED
 **                      driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address and pointer to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_read_testregister_1(const I2C_Dev *i2c_dev,
                                           uint8_t *regValue)
{
    ReturnStatus status = RETURN_OK;
    status = ioexp_led_raw_read(i2c_dev, SX1509_REG_TEST_1, regValue);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_interrupt
 **
 **    DESCRIPTION     : Write the Interrupt Mask Register Value(s) into IO
 **                      Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_interrupt(const I2C_Dev *i2c_dev,
                                        sx1509RegType regType,
                                        uint8_t interruptMaskRegValue1,
                                        uint8_t interruptMaskRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ?
                                 (SX1509_REG_INTERRUPT_MASK_A) :
                                 (SX1509_REG_INTERRUPT_MASK_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, interruptMaskRegValue1,
                                 interruptMaskRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_edge_sense_A
 **
 **    DESCRIPTION     : Write the Edge Sense Register A(I/O[7:0]) Value(s)
 **                      into IO Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, Low byte value
 **                      & High byte values of A to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
/*
 * 00 : None
 * 01 : Rising
 * 10 : Falling
 * 11 : Both
 */
ReturnStatus ioexp_led_config_edge_sense_A(const I2C_Dev *i2c_dev,
                                           sx1509EdgeSenseRegType regType,
                                           uint8_t edgeSenseLowARegValue,
                                           uint8_t edgeSenseHighARegValue)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_EDGE_SENSE_REG_LOW) ?
                                 (SX1509_REG_SENSE_LOW_A) :
                                 (SX1509_REG_SENSE_HIGH_A);
    if (regType == SX1509_EDGE_SENSE_REG_LOW_HIGH) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, edgeSenseLowARegValue,
                                 edgeSenseHighARegValue, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_edge_sense_B
 **
 **    DESCRIPTION     : Write the Edge Sense Register B(I/O[15:8]) Value(s)
 **                      into IO Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, Low byte value
 **                      & High byte values of B to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_config_edge_sense_B(const I2C_Dev *i2c_dev,
                                           sx1509EdgeSenseRegType regType,
                                           uint8_t edgeSenseLowBRegValue,
                                           uint8_t edgeSenseHighBRegValue)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_EDGE_SENSE_REG_LOW) ?
                                 (SX1509_REG_SENSE_LOW_B) :
                                 (SX1509_REG_SENSE_HIGH_B);
    if (regType == SX1509_EDGE_SENSE_REG_LOW_HIGH) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, edgeSenseLowBRegValue,
                                 edgeSenseHighBRegValue, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_config_debounce
 **
 **    DESCRIPTION     : Write the Debounce time Value into IO Expander with
 **                      LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address and debounce Time(ms) to be
 **                      written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
/* Debounce time-to-byte map: (assuming fOsc = 2MHz; 2^(n-1))
 * 0: 0.5ms     1: 1ms
 * 2: 2ms       3: 4ms
 * 4: 8ms       5: 16ms
 * 6: 32ms      7: 64ms
 */
ReturnStatus ioexp_led_config_debounce_time(const I2C_Dev *i2c_dev,
                                            uint8_t debounceTime)
{
    ReturnStatus status = RETURN_OK;
    uint8_t index = 0;
    uint8_t regValue = 0;

    for (index = 0; index < 8; index++) {
        if (debounceTime & (1 << index)) {
            regValue = index + 1;
            break;
        }
    }

    status = ioexp_led_raw_write(i2c_dev, SX1509_REG_DEBOUNCE_CONFIG, regValue,
                                 0, 1);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_enable_debounce
 **
 **    DESCRIPTION     : Write the Debounce enable Register Value into
 **                      IO Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type, value 1 &
 **                      value 2 to be written and No of Bytes.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_enable_debounce(const I2C_Dev *i2c_dev,
                                       sx1509RegType regType,
                                       uint8_t debounceEnableRegValue1,
                                       uint8_t debounceEnableRegValue2)
{
    ReturnStatus status = RETURN_OK;
    uint8_t noOfBytes = 1;
    uint8_t regAddress = (regType == SX1509_REG_A) ?
                                 (SX1509_REG_DEBOUNCE_ENABLE_A) :
                                 (SX1509_REG_DEBOUNCE_ENABLE_B);
    if (regType == SX1509_REG_AB) {
        noOfBytes = 2;
    }

    status = ioexp_led_raw_write(i2c_dev, regAddress, debounceEnableRegValue1,
                                 debounceEnableRegValue2, noOfBytes);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_read_interrupt_source
 **
 **    DESCRIPTION     : Read the Interrupt Source Register Value from IO
 **                      Expander with LED driver SX1509.
 **
 **    ARGUMENTS       : Subsystem, Slave address, Register Type and pointer
 **                      to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_get_interrupt_source(const I2C_Dev *i2c_dev,
                                            uint16_t *intPins)
{
    ReturnStatus status = RETURN_OK;
    uint8_t regValueA = 0;
    uint8_t regValueB = 0;

    status = ioexp_led_raw_read(i2c_dev, SX1509_REG_INTERRUPT_SOURCE_A,
                                &regValueA);
    if (status != RETURN_OK) {
        return status;
    }
    status = ioexp_led_raw_read(i2c_dev, SX1509_REG_INTERRUPT_SOURCE_B,
                                &regValueB);
    *intPins = (uint16_t)((regValueB << 8) | regValueA);
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : ioexp_led_clear_interrupt_source
 **
 **    DESCRIPTION     : Clear the Interrupt status.
 **
 **    ARGUMENTS       : Subsystem and Slave address.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus ioexp_led_clear_interrupt_source(const I2C_Dev *i2c_dev)
{
    ReturnStatus status = RETURN_OK;
    status = ioexp_led_raw_write(i2c_dev, SX1509_REG_INTERRUPT_SOURCE_B, 0xFF,
                                 0xFF, 2);
    return status;
}
