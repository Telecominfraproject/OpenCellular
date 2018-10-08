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
#include "Board.h"
#include "helpers/array.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"

/* TI-RTOS driver files */
#include <ti/drivers/I2C.h>

#include <string.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
I2C_Handle OC_I2C_Handle[Board_I2CCOUNT] = {};

/*****************************************************************************
 **    FUNCTION NAME   : i2c_open_bus
 **
 **    DESCRIPTION     : Initialize I2C Bus
 **
 **    ARGUMENTS       : I2C bus index
 **
 **    RETURN TYPE     : I2C_Handle (NULL on failure)
 **
 *****************************************************************************/
I2C_Handle i2c_open_bus(unsigned int index)
{
    if (index >= ARRAY_SIZE(OC_I2C_Handle)) {
        LOGGER_ERROR("I2CBUS:ERROR:: I2C bus %d not found\n", index);
        return NULL;
    }

    I2C_Params i2cParams;
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    if (!OC_I2C_Handle[index]) {
        OC_I2C_Handle[index] = I2C_open(index, &i2cParams);

        if (!OC_I2C_Handle[index]) {
            LOGGER_ERROR("I2CBUS:ERROR:: Failed Initializing I2C bus %d.\n",
                         index);
        }
    }
    return OC_I2C_Handle[index];
}

/*****************************************************************************
 **    FUNCTION NAME   : i2c_close_bus
 **
 **    DESCRIPTION     : Initialize I2C Bus
 **
 **    ARGUMENTS       : I2C bus index
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void i2c_close_bus(I2C_Handle *i2cHandle)
{
    I2C_close(*i2cHandle);
    i2cHandle = NULL;
}

/*****************************************************************************
 **    FUNCTION NAME   : i2c_reg_write
 **
 **    DESCRIPTION     : Writing device register over i2c bus.
 **
 **    ARGUMENTS       : I2C handle, device address, register address and value.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus i2c_reg_write(I2C_Handle i2cHandle, uint8_t deviceAddress,
                           uint8_t regAddress, uint16_t value,
                           uint8_t numofBytes)
{
    ReturnStatus status = RETURN_OK;
    uint8_t txBuffer[3];
    I2C_Transaction i2cTransaction;
    txBuffer[0] = regAddress;
    memcpy(&txBuffer[1], &value, numofBytes);
    i2cTransaction.slaveAddress = deviceAddress;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = numofBytes + 1;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;
    if (I2C_transfer(i2cHandle, &i2cTransaction)) {
        //LOGGER_DEBUG("I2CBUS:INFO:: I2C write success for device: 0x%x reg Addr: 0x%x value: 0x%x.\n",
        //              deviceAddress, regAddress, value);
        status = RETURN_OK;
    } else {
        LOGGER_ERROR(
                "I2CBUS:ERROR:: I2C write failed for for device: 0x%x reg Addr: 0x%x value: 0x%x.\n",
                deviceAddress, regAddress, value);
        status = RETURN_NOTOK;
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : i2c_reg_read
 **
 **    DESCRIPTION     : Reading device register over i2c bus.
 **
 **    ARGUMENTS       : I2C handle, device address, register address and value.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus i2c_reg_read(I2C_Handle i2cHandle, uint8_t deviceAddress,
                          uint8_t regAddress, uint16_t *value,
                          uint8_t numofBytes)
{
    ReturnStatus status = RETURN_OK;
    uint8_t txBuffer[1] = { 0 };
    uint8_t rxBuffer[2] = { 0 };
    txBuffer[0] = regAddress;
    I2C_Transaction i2cTransaction;
    i2cTransaction.slaveAddress = deviceAddress;
    i2cTransaction.writeBuf = &txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = numofBytes;
    if (I2C_transfer(i2cHandle, &i2cTransaction)) {
        memcpy(value, rxBuffer, numofBytes);
        //LOGGER_DEBUG("I2CBUS:INFO:: I2C read success for device: 0x%x reg Addr: 0x%x value : 0x%x.\n",
        //              deviceAddress, regAddress, *value);
        status = RETURN_OK;
    } else {
        LOGGER_ERROR(
                "I2CBUS:ERROR:: I2C read failed for for device: 0x%x reg Addr: 0x%x.\n",
                deviceAddress, regAddress);
        status = RETURN_NOTOK;
    }
    return status;
}
