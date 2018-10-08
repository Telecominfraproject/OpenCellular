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
#include "inc/devices/pca9557.h"

#include "inc/common/global_header.h"

/* Register Addresses */
#define PCA9557_INPUT_PORT_REG 0x00
#define PCA9557_OUTPUT_PORT_REG 0x01
#define PCA9557_POL_INV_REG 0x02
#define PCA9557_CONFIG_REG 0x03

/*****************************************************************************
 **    FUNCTION NAME   : PCA9557_regRead
 **
 **    DESCRIPTION     : Read the value from IO Expander register.
 **
 **    ARGUMENTS       : i2c device, Register address and value to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus PCA9557_regRead(const I2C_Dev *i2c_dev, uint8_t regAddress,
                                    uint8_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle pca9557_handle = i2c_get_handle(i2c_dev->bus);
    if (!pca9557_handle) {
        LOGGER_ERROR("IOEXP:ERROR:: Failed to get I2C Bus for IO Expander "
                     "0x%x on bus 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        uint16_t tmpValue = 0x0000;
        status = i2c_reg_read(pca9557_handle, i2c_dev->slave_addr, regAddress,
                              &tmpValue, 1);
        *regValue = (uint8_t)tmpValue;
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : PCA9557_regWrite
 **
 **    DESCRIPTION     : Write the value to IO Expander register.
 **
 **    ARGUMENTS       : i2c device, Register address and value to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus PCA9557_regWrite(const I2C_Dev *i2c_dev, uint8_t regAddress,
                                     uint8_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle pca9557_handle = i2c_get_handle(i2c_dev->bus);
    if (!pca9557_handle) {
        LOGGER_ERROR("IOEXP:ERROR:: Failed to get I2C Bus for IO Expander "
                     "0x%x on bus 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        status = i2c_reg_write(pca9557_handle, i2c_dev->slave_addr, regAddress,
                               regValue, 1);
    }
    return status;
}

/******************************************************************************
 * @fn          PCA9557_getInput
 *
 * @brief       Reading Input Port Register Value from IO Expander.
 *
 * @args        i2c device and pointer to the input port register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus PCA9557_getInput(const I2C_Dev *i2c_dev, uint8_t *inputRegValue)
{
    ReturnStatus status =
            PCA9557_regRead(i2c_dev, PCA9557_INPUT_PORT_REG, inputRegValue);
    if (status == RETURN_OK) {
        LOGGER_DEBUG("IOEXP:INFO:: IO Expander 0x%x on bus 0x%x is "
                     "reporting Input Port Reg value of 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus, *inputRegValue);
    }
    return status;
}

/******************************************************************************
 * @fn          PCA9557_getOutput
 *
 * @brief       Reading Output Port Register Value from IO Expander.
 *
 * @args        i2c device and pointer to the output port register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus PCA9557_getOutput(const I2C_Dev *i2c_dev, uint8_t *outputRegValue)
{
    ReturnStatus status =
            PCA9557_regRead(i2c_dev, PCA9557_OUTPUT_PORT_REG, outputRegValue);
    if (status == RETURN_OK) {
        LOGGER_DEBUG("IOEXP:INFO:: IO Expander 0x%x on bus 0x%x is "
                     "reporting Output Port Reg value of 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus, *outputRegValue);
    }
    return status;
}

/******************************************************************************
 * @fn          PCA9557_setOutput
 *
 * @brief       Write the value into Output port register of IO Expander.
 *
 * @args        i2c device and Output Port register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus PCA9557_setOutput(const I2C_Dev *i2c_dev, uint8_t outputRegValue)
{
    return PCA9557_regWrite(i2c_dev, PCA9557_OUTPUT_PORT_REG, outputRegValue);
}

/******************************************************************************
 * @fn          PCA9557_getConfig
 *
 * @brief       Read Configuration register of IO Expander.
 *
 * @args        i2c device and pointer to the configuration register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus PCA9557_getConfig(const I2C_Dev *i2c_dev, uint8_t *configValue)
{
    return PCA9557_regRead(i2c_dev, PCA9557_CONFIG_REG, configValue);
}

/******************************************************************************
 * @fn          PCA9557_setConfig
 *
 * @brief       Configure Input and Output direction of IO Expander.
 *
 * @args        i2c device and configuration register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus PCA9557_setConfig(const I2C_Dev *i2c_dev, uint8_t configValue)
{
    return PCA9557_regWrite(i2c_dev, PCA9557_CONFIG_REG, configValue);
}

/******************************************************************************
 * @fn          PCA9557_getPolarity
 *
 * @brief       Read Polaity Inversion register of IO Expander.
 *
 * @args        i2c device and pointer to the Polarity Inversion register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus PCA9557_getPolarity(const I2C_Dev *i2c_dev, uint8_t *polRegValue)
{
    return PCA9557_regRead(i2c_dev, PCA9557_POL_INV_REG, polRegValue);
}

/******************************************************************************
 * @fn          PCA9557_setPolarity
 *
 * @brief       Configure polarity of Input and Output pins of IO Expander.
 *
 * @args        i2c device and Polarity Inversion register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus PCA9557_setPolarity(const I2C_Dev *i2c_dev, uint8_t polRegValue)
{
    return PCA9557_regWrite(i2c_dev, PCA9557_POL_INV_REG, polRegValue);
}
