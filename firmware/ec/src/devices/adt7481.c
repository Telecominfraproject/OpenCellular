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
#include "inc/devices/adt7481.h"

#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"

/*****************************************************************************
 *                          REGISTER DEFINITIONS
 *****************************************************************************/
#define ADT7481_REG_R_LOCAL_TEMP 0x00 /* Local Temperature Value */
#define ADT7481_REG_R_REMOTE1_TEMP_H \
    0x01 /* Remote 1 Temperature Value High Byte */
#define ADT7481_REG_R_STATUS1 0x02 /* Status Register 1 */
#define ADT7481_REG_R_CONFIG1 0x03 /* Configuration Register 1 */
#define ADT7481_REG_R_CONVERSION_RATE \
    0x04 /* Conversion Rate/Channel Selector */
#define ADT7481_REG_R_LOCAL_HIGHLIMIT 0x05 /* Local Temperature High Limit */
#define ADT7481_REG_R_LOCAL_LOWLIMIT 0x06 /* Local Temperature Low Limit */
#define ADT7481_REG_R_REMOTE1_HIGHLIMIT_H \
    0x07 /* Remote 1 Temp High Limit High Byte */
#define ADT7481_REG_R_REMOTE1_LOWLIMIT_H \
    0x08 /* Remote 1 Temp Low Limit High Byte */
#define ADT7481_REG_W_CONFIG1 0x09 /* Configuration Register */
#define ADT7481_REG_W_CONVERSION_RATE \
    0x0A /* Conversion Rate/Channel Selector */
#define ADT7481_REG_W_LOCAL_HIGHLIMIT 0x0B /* Local Temperature High Limit */
#define ADT7481_REG_W_LOCAL_LOWLIMIT 0x0C /* Local Temperature Low Limit */
#define ADT7481_REG_W_REMOTE1_HIGHLIMIT_H \
    0x0D /* Remote 1 Temp High Limit High Byte */
#define ADT7481_REG_W_REMOTE1_LOWLIMIT_H \
    0x0E /* Remote 1 Temp Low Limit High Byte */
#define ADT7481_REG_W_ONE_SHOT 0x0F /* One-Shot */
#define ADT7481_REG_R_REMOTE1_TEMP_L \
    0x10 /* Remote 1 Temperature Value Low Byte */
#define ADT7481_REG_R_REMOTE1_OFFSET_H \
    0x11 /* Remote 1 Temperature Offset High Byte */
#define ADT7481_REG_W_REMOTE1_OFFSET_H \
    0x11 /* Remote 1 Temperature Offset High Byte */
#define ADT7481_REG_R_REMOTE1_OFFSET_L \
    0x12 /* Remote 1 Temperature Offset Low Byte */
#define ADT7481_REG_W_REMOTE1_OFFSET_L \
    0x12 /* Remote 1 Temperature Offset Low Byte */
#define ADT7481_REG_R_REMOTE1_HIGHLIMIT_L \
    0x13 /* Remote 1 Temp High Limit Low Byte */
#define ADT7481_REG_W_REMOTE1_HIGHLIMIT_L \
    0x13 /* Remote 1 Temp High Limit Low Byte */
#define ADT7481_REG_R_REMOTE1_LOWLIMIT_L \
    0x14 /* Remote 1 Temp Low Limit Low Byte */
#define ADT7481_REG_W_REMOTE1_LOWLIMIT_L \
    0x14 /* Remote 1 Temp Low Limit Low Byte */
#define ADT7481_REG_R_REMOTE1_THERMLIMIT 0x19 /* Remote 1 THERM Limit */
#define ADT7481_REG_W_REMOTE1_THERMLIMIT 0x19 /* Remote 1 THERM Limit */
#define ADT7481_REG_R_LOCAL_THERMLIMIT 0x20 /* Local THERM Limit */
#define ADT7481_REG_W_LOCAL_THERMLIMIT 0x20 /* Local THERM Limit */
#define ADT7481_REG_R_THERM_HYST 0x21 /* THERM Hysteresis */
#define ADT7481_REG_W_THERM_HYST 0x21 /* THERM Hysteresis */
#define ADT7481_REG_R_CONECUTIVE_ALERT 0x22 /* Consecutive ALERT */
#define ADT7481_REG_W_CONECUTIVE_ALERT 0x22 /* Consecutive ALERT */
#define ADT7481_REG_R_STATUS2 0x23 /* Status Register 2 */
#define ADT7481_REG_R_CONFIG2 0x24 /* Configuration 2 Register */
#define ADT7481_REG_W_CONFIG2 0x24 /* Configuration 2 Register */
#define ADT7481_REG_R_REMOTE2_TEMP_H \
    0x30 /* Remote 2 Temperature Value High Byte */
#define ADT7481_REG_R_REMOTE2_HIGHLIMIT_H \
    0x31 /* Remote 2 Temp High Limit High Byte */
#define ADT7481_REG_W_REMOTE2_HIGHLIMIT_H \
    0x31 /* Remote 2 Temp High Limit High Byte */
#define ADT7481_REG_R_REMOTE2_LOWLIMIT_H \
    0x32 /* Remote 2 Temp Low Limit High Byte */
#define ADT7481_REG_W_REMOTE2_LOWLIMIT_H \
    0x32 /* Remote 2 Temp Low Limit High Byte */
#define ADT7481_REG_R_REMOTE2_TEMP_L \
    0x33 /* Remote 2 Temperature Value Low Byte */
#define ADT7481_REG_R_REMOTE2_OFFSET_H \
    0x34 /* Remote 2 Temperature Offset High Byte */
#define ADT7481_REG_W_REMOTE2_OFFSET_H \
    0x34 /* Remote 2 Temperature Offset High Byte */
#define ADT7481_REG_R_REMOTE2_OFFSET_L \
    0x35 /* Remote 2 Temperature Offset Low Byte */
#define ADT7481_REG_W_REMOTE2_OFFSET_L \
    0x35 /* Remote 2 Temperature Offset Low Byte */
#define ADT7481_REG_R_REMOTE2_HIGHLIMIT_L \
    0x36 /* Remote 2 Temp High Limit Low Byte */
#define ADT7481_REG_W_REMOTE2_HIGHLIMIT_L \
    0x36 /* Remote 2 Temp High Limit Low Byte */
#define ADT7481_REG_R_REMOTE2_LOWLIMIT_L \
    0x37 /* Remote 2 Temp Low Limit Low Byte */
#define ADT7481_REG_W_REMOTE2_LOWLIMIT_L \
    0x37 /* Remote 2 Temp Low Limit Low Byte */
#define ADT7481_REG_R_REMOTE2_THERMLIMIT 0x39 /* Remote 2 THERM Limit */
#define ADT7481_REG_W_REMOTE2_THERMLIMIT 0x39 /* Remote 2 THERM Limit */
#define ADT7481_REG_R_CHIP_ID 0x3D /* Device ID */
#define ADT7481_REG_R_MAN_ID 0x3E /* Manufacturer ID */

/*
 * Macros to convert temperature values to register values and vice versa.
 * When the measurement range is in extended mode, an offset binary data format
 * is used for both local and remote results. Temperature values in the offset
 * binary data format are offset by +64.
 */
#ifdef ADT7481_EXTENDED_FLAG
#define TEMP_TO_REG_U8(x) (x + 64)
#define TEMP_TO_REG_U16(x) ((x + 64) << 8)
#define REG_U8_TO_TEMP(y) (y - 64)
#define REG_U16_TO_TEMP(y) (y - 64)
#else
#define TEMP_TO_REG_U8(x) (x)
#define TEMP_TO_REG_U16(x) (x << 8)
#define REG_U8_TO_TEMP(y) (y)
#define REG_U16_TO_TEMP(y) (y)
#endif

/*****************************************************************************
 **    FUNCTION NAME   : adt7481_raw_read
 **
 **    DESCRIPTION     : Read the register value from Temperature sensor ADT7481.
 **
 **    ARGUMENTS       : OCSubsystem, Slave address, Register address and
 **                      pointer to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus adt7481_raw_read(const I2C_Dev *i2c_dev, uint8_t regAddress,
                                     uint8_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle adt7481_handle = i2c_get_handle(i2c_dev->bus);
    if (!adt7481_handle) {
        LOGGER_ERROR("ADT7481:ERROR:: Failed to get I2C Bus for Thermal sensor "
                     "0x%x on bus 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        status = i2c_reg_read(adt7481_handle, i2c_dev->slave_addr, regAddress,
                              (uint16_t *)regValue, 1);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : adt7481_raw_write
 **
 **    DESCRIPTION     : Write the register value into Temperature sensor ADT7481.
 **
 **    ARGUMENTS       : OCSubsystem, Slave address, Register address and value
 **                      to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus adt7481_raw_write(const I2C_Dev *i2c_dev,
                                      uint8_t regAddress, uint8_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle adt7481_handle = i2c_get_handle(i2c_dev->bus);
    if (!adt7481_handle) {
        LOGGER_ERROR("ADT7481:ERROR:: Failed to get I2C Bus for Thermal sensor "
                     "0x%x on bus 0x%x.\n",
                     i2c_dev->slave_addr, i2c_dev->bus);
    } else {
        status = i2c_reg_write(adt7481_handle, i2c_dev->slave_addr, regAddress,
                               regValue, 1);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : adt7481_get_dev_id
 **
 **    DESCRIPTION     : Read the Device ID from Temperature sensor ADT7481.
 **
 **    ARGUMENTS       : OCSubsystem, Slave address, and pointer to device Id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus adt7481_get_dev_id(const I2C_Dev *i2c_dev, uint8_t *devID)
{
    ReturnStatus status = RETURN_OK;

    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_CHIP_ID, devID);
    if (status != RETURN_OK) {
        devID = NULL;
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : adt7481_get_mfg_id
 **
 **    DESCRIPTION     : Read the Manufacturer ID from Temperature sensor ADT7481.
 **
 **    ARGUMENTS       : OCSubsystem, Slave address and pointer to manufacturing
 **                      id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus adt7481_get_mfg_id(const I2C_Dev *i2c_dev, uint8_t *mfgID)
{
    ReturnStatus status = RETURN_OK;
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_MAN_ID, mfgID);
    if (status != RETURN_OK) {
        mfgID = NULL;
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : adt7481_probe
 **
 **    DESCRIPTION     : Read the Manufacturer ID from Temperature sensor ADT7481.
 **
 **    ARGUMENTS       : I2C driver config and POST Data struct
 **                      id.
 **
 **    RETURN TYPE     : ePostCode
 **
 *****************************************************************************/
ePostCode adt7481_probe(const I2C_Dev *i2c_dev, POSTData *postData)
{
    ePostCode postcode = POST_DEV_MISSING;
    ReturnStatus status = RETURN_OK;
    uint8_t devId = 0x0000;
    uint8_t manfId = 0x0000;
    status = adt7481_get_dev_id(i2c_dev, &devId);
    status = adt7481_get_mfg_id(i2c_dev, &manfId);
    if (status != RETURN_OK) {
        postcode = POST_DEV_MISSING;
    } else if ((devId == TEMP_ADT7481_DEV_ID) &&
               (manfId == TEMP_ADT7481_MANF_ID)) {
        postcode = POST_DEV_FOUND;
    } else {
        postcode = POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, i2c_dev->bus, i2c_dev->slave_addr, manfId,
                         devId);
    return postcode;
}

/******************************************************************************
 * @fn          adt7481_get_config1
 *
 * @brief       Read configuration 1 register value of temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to the configuation value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_config1(const I2C_Dev *i2c_dev, uint8_t *configValue)
{
    ReturnStatus status = RETURN_OK;
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_CONFIG1, configValue);
    if (status != RETURN_OK) {
        configValue = NULL;
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_config1
 *
 * @brief       Configure configuration 1 register of temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and configuration register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_config1(const I2C_Dev *i2c_dev, uint8_t configValue)
{
    ReturnStatus status = RETURN_OK;
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_CONFIG1, configValue);
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_conv_rate
 *
 * @brief       Read Conversion Rate/ Channel Selector register value of
 *              temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to the conversion rate
 *              value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_conv_rate(const I2C_Dev *i2c_dev,
                                   uint8_t *convRateValue)
{
    ReturnStatus status = RETURN_OK;
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_CONVERSION_RATE,
                              convRateValue);
    if (status != RETURN_OK) {
        convRateValue = NULL;
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_conv_rate
 *
 * @brief       Configure  Conversion Rate/ Channel Selector register of
 *              temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and conversion rate register value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_conv_rate(const I2C_Dev *i2c_dev,
                                   uint8_t convRateValue)
{
    ReturnStatus status = RETURN_OK;
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_CONVERSION_RATE,
                               convRateValue);
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_status1
 *
 * @brief       Read status 1 register value of temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to the configuation value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_status1(const I2C_Dev *i2c_dev, uint8_t *statusValue)
{
    ReturnStatus status = RETURN_OK;
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_STATUS1, statusValue);
    if (status != RETURN_OK) {
        statusValue = NULL;
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_status2
 *
 * @brief       Read status 2 register value of temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to the configuation value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_status2(const I2C_Dev *i2c_dev, uint8_t *statusValue)
{
    ReturnStatus status = RETURN_OK;
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_STATUS2, statusValue);
    if (status != RETURN_OK) {
        statusValue = NULL;
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_local_temp_val
 *
 * @brief       Read Local temperature value of temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to the temp value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_local_temp_val(const I2C_Dev *i2c_dev,
                                        int16_t *tempValue)
{
    ReturnStatus status = RETURN_OK;
    uint8_t regValue = 0x00;
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_LOCAL_TEMP, &regValue);
    if (status != RETURN_OK) {
        tempValue = NULL;
    } else {
        *tempValue = (int8_t)REG_U8_TO_TEMP(regValue);
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote1_temp_val
 *
 * @brief       Read Remote 1 temperature value of temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to the temp value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote1_temp_val(const I2C_Dev *i2c_dev,
                                          int16_t *tempValue)
{
    ReturnStatus status = RETURN_OK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;
    status =
            adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_TEMP_L, &lRegValue);
    if (status != RETURN_OK) {
        tempValue = NULL;
    } else {
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_TEMP_H,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempValue = NULL;
        } else {
            *tempValue = (int16_t)REG_U16_TO_TEMP(hRegValue);
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote2_temp_val
 *
 * @brief       Read Remote 2 temperature value of temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to the temp value.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote2_temp_val(const I2C_Dev *i2c_dev,
                                          int8_t *tempValue)
{
    ReturnStatus status = RETURN_OK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;
    status =
            adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_TEMP_L, &lRegValue);
    if (status != RETURN_OK) {
        tempValue = NULL;
    } else {
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_TEMP_H,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempValue = NULL;
        } else {
            *tempValue = (int8_t)REG_U16_TO_TEMP(hRegValue);
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_local_temp_limit
 *
 * @brief       Read local temperature alert limits of the temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, limit to read, Slave address and pointer to
 *              limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus
adt7481_get_local_temp_limit(const I2C_Dev *i2c_dev,
                             eTempSensorADT7481ConfigParamsId limitToConfig,
                             int16_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;
    uint8_t regAddress = 0x0000;

    switch (limitToConfig) {
        case CONF_TEMP_ADT7481_LOW_LIMIT_REG: {
            regAddress = ADT7481_REG_R_LOCAL_LOWLIMIT;
            break;
        }
        case CONF_TEMP_ADT7481_HIGH_LIMIT_REG: {
            regAddress = ADT7481_REG_R_LOCAL_HIGHLIMIT;
            break;
        }
        case CONF_TEMP_ADT7481_THERM_LIMIT_REG: {
            regAddress = ADT7481_REG_R_LOCAL_THERMLIMIT;
            break;
        }
        default: {
            return status;
        }
    }

    status = adt7481_raw_read(i2c_dev, regAddress, &regValue);
    if (status != RETURN_OK) {
        tempLimitValue = NULL;
    } else {
        *tempLimitValue = (int8_t)REG_U8_TO_TEMP(regValue);
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_local_temp_limit
 *
 * @brief       Configure Local Temperature alert limits of temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, limit to configure, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus
adt7481_set_local_temp_limit(const I2C_Dev *i2c_dev,
                             eTempSensorADT7481ConfigParamsId limitToConfig,
                             int16_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regAddress = 0x00;
    uint8_t regValue = 0x0000;

    switch (limitToConfig) {
        case CONF_TEMP_ADT7481_LOW_LIMIT_REG: {
            regAddress = ADT7481_REG_W_LOCAL_LOWLIMIT;
            break;
        }
        case CONF_TEMP_ADT7481_HIGH_LIMIT_REG: {
            regAddress = ADT7481_REG_W_LOCAL_HIGHLIMIT;
            break;
        }
        case CONF_TEMP_ADT7481_THERM_LIMIT_REG: {
            regAddress = ADT7481_REG_W_LOCAL_THERMLIMIT;
            break;
        }
        default: {
            return status;
        }
    }

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U8(tempLimitValue);

    status = adt7481_raw_write(i2c_dev, regAddress, regValue);
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote1_temp_low_limit
 *
 * @brief       Read Remote 1 temperature Low alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote1_temp_low_limit(const I2C_Dev *i2c_dev,
                                                int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;

    /* Read LSB data */
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_LOWLIMIT_L,
                              &lRegValue);
    if (status != RETURN_OK) {
        tempLimitValue = NULL;
    } else {
        /* Read MSB data */
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_LOWLIMIT_H,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempLimitValue = NULL;
        } else {
            *tempLimitValue = (int8_t)REG_U16_TO_TEMP(hRegValue);
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote1_temp_high_limit
 *
 * @brief       Read Remote 1 temperature High alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote1_temp_high_limit(const I2C_Dev *i2c_dev,
                                                 int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;

    /* Read LSB data */
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_HIGHLIMIT_L,
                              &lRegValue);
    if (status != RETURN_OK) {
        tempLimitValue = NULL;
    } else {
        /* Read MSB data */
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_HIGHLIMIT_H,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempLimitValue = NULL;
        } else {
            *tempLimitValue = (int8_t)REG_U16_TO_TEMP(hRegValue);
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote1_temp_therm_limit
 *
 * @brief       Read Remote 1 temperature Therm limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote1_temp_therm_limit(const I2C_Dev *i2c_dev,
                                                  int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;

    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_THERMLIMIT,
                              &regValue);
    if (status != RETURN_OK) {
        tempLimitValue = NULL;
    } else {
        *tempLimitValue = (int8_t)REG_U8_TO_TEMP(regValue);
    }

    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote1_temp_limit
 *
 * @brief       Read Remote 1 temperature alert limits of the temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, limit to read, Slave address and pointer to
 *              limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus
adt7481_get_remote1_temp_limit(const I2C_Dev *i2c_dev,
                               eTempSensorADT7481ConfigParamsId limitToConfig,
                               int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;

    switch (limitToConfig) {
        case CONF_TEMP_ADT7481_LOW_LIMIT_REG: {
            status =
                    adt7481_get_remote1_temp_low_limit(i2c_dev, tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_HIGH_LIMIT_REG: {
            status = adt7481_get_remote1_temp_high_limit(i2c_dev,
                                                         tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_THERM_LIMIT_REG: {
            status = adt7481_get_remote1_temp_therm_limit(i2c_dev,
                                                          tempLimitValue);
            break;
        }
        default: {
            return status;
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote1_temp_low_limit
 *
 * @brief       Write Remote 1 temperature Low alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote1_temp_low_limit(const I2C_Dev *i2c_dev,
                                                int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint16_t regValue = 0x0000;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U16(tempLimitValue);

    /* Write LSB data */
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE1_LOWLIMIT_L,
                               (uint8_t)regValue);
    if (status == RETURN_OK) {
        /* Write MSB data */
        status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE1_LOWLIMIT_H,
                                   (uint8_t)(regValue >> 8));
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote1_temp_high_limit
 *
 * @brief       Write Remote 1 temperature High alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote1_temp_high_limit(const I2C_Dev *i2c_dev,
                                                 int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint16_t regValue = 0x0000;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U16(tempLimitValue);

    /* Write LSB data */
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE1_HIGHLIMIT_L,
                               (uint8_t)regValue);
    if (status == RETURN_OK) {
        /* Write MSB data */
        status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE1_HIGHLIMIT_H,
                                   (uint8_t)(regValue >> 8));
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote1_temp_therm_limit
 *
 * @brief       Write Remote 1 temperature Therm limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote1_temp_therm_limit(const I2C_Dev *i2c_dev,
                                                  int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U8(tempLimitValue);

    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE1_THERMLIMIT,
                               regValue);
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote1_temp_limit
 *
 * @brief       Configure Remote 1 Temperature alert limits of temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, limit to configure, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus
adt7481_set_remote1_temp_limit(const I2C_Dev *i2c_dev,
                               eTempSensorADT7481ConfigParamsId limitToConfig,
                               int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;

    switch (limitToConfig) {
        case CONF_TEMP_ADT7481_LOW_LIMIT_REG: {
            status =
                    adt7481_set_remote1_temp_low_limit(i2c_dev, tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_HIGH_LIMIT_REG: {
            status = adt7481_set_remote1_temp_high_limit(i2c_dev,
                                                         tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_THERM_LIMIT_REG: {
            status = adt7481_set_remote1_temp_therm_limit(i2c_dev,
                                                          tempLimitValue);
            break;
        }
        default: {
            return status;
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote2_temp_low_limit
 *
 * @brief       Read Remote 2 temperature Low alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote2_temp_low_limit(const I2C_Dev *i2c_dev,
                                                int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;

    /* Read LSB data */
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_LOWLIMIT_L,
                              &lRegValue);
    if (status != RETURN_OK) {
        tempLimitValue = NULL;
    } else {
        /* Read MSB data */
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_LOWLIMIT_H,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempLimitValue = NULL;
        } else {
            *tempLimitValue = (int8_t)REG_U16_TO_TEMP(hRegValue);
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote2_temp_high_limit
 *
 * @brief       Read Remote 2 temperature High alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote2_temp_high_limit(const I2C_Dev *i2c_dev,
                                                 int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;

    /* Read LSB data */
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_HIGHLIMIT_L,
                              &lRegValue);
    if (status != RETURN_OK) {
        tempLimitValue = NULL;
    } else {
        /* Read MSB data */
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_HIGHLIMIT_H,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempLimitValue = NULL;
        } else {
            *tempLimitValue = (int8_t)REG_U16_TO_TEMP(hRegValue);
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote2_temp_therm_limit
 *
 * @brief       Read Remote 2 temperature Therm limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote2_temp_therm_limit(const I2C_Dev *i2c_dev,
                                                  int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;

    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_THERMLIMIT,
                              &regValue);
    if (status != RETURN_OK) {
        tempLimitValue = NULL;
    } else {
        *tempLimitValue = (int8_t)REG_U8_TO_TEMP(regValue);
    }

    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote2_temp_limit
 *
 * @brief       Read Remote 2 temperature alert limits of the temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, limit to read, Slave address and pointer to
 *              limit set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus
adt7481_get_remote2_temp_limit(const I2C_Dev *i2c_dev,
                               eTempSensorADT7481ConfigParamsId limitToConfig,
                               int8_t *tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;

    switch (limitToConfig) {
        case CONF_TEMP_ADT7481_LOW_LIMIT_REG: {
            status =
                    adt7481_get_remote2_temp_low_limit(i2c_dev, tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_HIGH_LIMIT_REG: {
            status = adt7481_get_remote2_temp_high_limit(i2c_dev,
                                                         tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_THERM_LIMIT_REG: {
            status = adt7481_get_remote2_temp_therm_limit(i2c_dev,
                                                          tempLimitValue);
            break;
        }
        default: {
            return status;
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote2_temp_low_limit
 *
 * @brief       Write Remote 2 temperature Low alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote2_temp_low_limit(const I2C_Dev *i2c_dev,
                                                int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint16_t regValue = 0x0000;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U16(tempLimitValue);

    /* Write LSB data */
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE2_LOWLIMIT_L,
                               (uint8_t)regValue);
    if (status == RETURN_OK) {
        /* Write MSB data */
        status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE2_LOWLIMIT_H,
                                   (uint8_t)(regValue >> 8));
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote2_temp_high_limit
 *
 * @brief       Write Remote 2 temperature High alert limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote2_temp_high_limit(const I2C_Dev *i2c_dev,
                                                 int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint16_t regValue = 0x0000;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U16(tempLimitValue);

    /* Write LSB data */
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE2_HIGHLIMIT_L,
                               (uint8_t)regValue);
    if (status == RETURN_OK) {
        /* Write MSB data */
        status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE2_HIGHLIMIT_H,
                                   (uint8_t)(regValue >> 8));
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote2_temp_therm_limit
 *
 * @brief       Write Remote 2 temperature Therm limit of the temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote2_temp_therm_limit(const I2C_Dev *i2c_dev,
                                                  int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U8(tempLimitValue);

    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE2_THERMLIMIT,
                               regValue);
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote2_temp_limit
 *
 * @brief       Configure Remote 2 Temperature alert limits of temperature
 *              sensor ADT7481.
 *
 * @args        OCSubsystem, limit to configure, Slave address and limit to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus
adt7481_set_remote2_temp_limit(const I2C_Dev *i2c_dev,
                               eTempSensorADT7481ConfigParamsId limitToConfig,
                               int8_t tempLimitValue)
{
    ReturnStatus status = RETURN_NOTOK;

    switch (limitToConfig) {
        case CONF_TEMP_ADT7481_LOW_LIMIT_REG: {
            status =
                    adt7481_set_remote2_temp_low_limit(i2c_dev, tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_HIGH_LIMIT_REG: {
            status = adt7481_set_remote2_temp_high_limit(i2c_dev,
                                                         tempLimitValue);
            break;
        }
        case CONF_TEMP_ADT7481_THERM_LIMIT_REG: {
            status = adt7481_set_remote2_temp_therm_limit(i2c_dev,
                                                          tempLimitValue);
            break;
        }
        default: {
            return status;
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote1_temp_offset
 *
 * @brief       Read Remote 1 temperature offset value of the temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to offset set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote1_temp_offset(const I2C_Dev *i2c_dev,
                                             int16_t *tempOffsetValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;

    /* Read LSB data */
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_OFFSET_L,
                              &lRegValue);
    if (status != RETURN_OK) {
        tempOffsetValue = NULL;
    } else {
        /* Read MSB data */
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE1_OFFSET_H,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempOffsetValue = NULL;
        } else {
            *tempOffsetValue = (int8_t)REG_U16_TO_TEMP(hRegValue);
        }
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote1_temp_offset
 *
 * @brief       Configure Remote 1 Temperature offset of temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, Slave address and offset to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote1_temp_offset(const I2C_Dev *i2c_dev,
                                             int16_t tempOffsetValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x0000;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U16(tempOffsetValue);

    /* Write LSB data */
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE1_OFFSET_L,
                               (uint8_t)regValue);
    if (status == RETURN_OK) {
        /* Write MSB data */
        status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE1_OFFSET_L,
                                   (uint8_t)(regValue >> 8));
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_remote2_temp_offset
 *
 * @brief       Read Remote 2 temperature offset value of the temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to offset set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_remote2_temp_offset(const I2C_Dev *i2c_dev,
                                             int16_t *tempOffsetValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t lRegValue = 0x00;
    uint8_t hRegValue = 0x00;

    /* Read LSB data */
    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_OFFSET_L,
                              &lRegValue);
    if (status != RETURN_OK) {
        tempOffsetValue = NULL;
    } else {
        /* Read MSB data */
        status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_REMOTE2_OFFSET_L,
                                  &hRegValue);
        if (status != RETURN_OK) {
            tempOffsetValue = NULL;
        } else {
            *tempOffsetValue = (int8_t)REG_U16_TO_TEMP(hRegValue);
        }
    }

    return status;
}

/******************************************************************************
 * @fn          adt7481_set_remote2_temp_offset
 *
 * @brief       Configure Remote 2 Temperature offset of temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, Slave address and offset to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_remote2_temp_offset(const I2C_Dev *i2c_dev,
                                             int8_t tempOffsetValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x0000;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U16(tempOffsetValue);

    /* Write LSB data */
    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE2_OFFSET_L,
                               (uint8_t)regValue);
    if (status == RETURN_OK) {
        /* Write MSB data */
        status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_REMOTE2_OFFSET_H,
                                   (uint8_t)(regValue >> 8));
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_get_therm_hysteresis
 *
 * @brief       Read THERM Hysteresis value of the temperature sensor
 *              ADT7481.
 *
 * @args        OCSubsystem, Slave address and pointer to therm hysteresis.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_get_therm_hysteresis(const I2C_Dev *i2c_dev,
                                          int8_t *tempHysteresisValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;

    status = adt7481_raw_read(i2c_dev, ADT7481_REG_R_THERM_HYST, &regValue);
    if (status != RETURN_OK) {
        tempHysteresisValue = NULL;
    } else {
        *tempHysteresisValue = (int8_t)REG_U8_TO_TEMP(regValue);
    }
    return status;
}

/******************************************************************************
 * @fn          adt7481_set_therm_hysteresis
 *
 * @brief       Write THERM Hysteresis of the temperature sensor ADT7481.
 *
 * @args        OCSubsystem, Slave address and therm hysteresis to set.
 *
 * @return      ReturnStatus
 *****************************************************************************/
ReturnStatus adt7481_set_therm_hysteresis(const I2C_Dev *i2c_dev,
                                          int8_t tempHysteresisValue)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t regValue = 0x00;

    /* Converting Temp limit into the register value */
    regValue = TEMP_TO_REG_U8(tempHysteresisValue);

    status = adt7481_raw_write(i2c_dev, ADT7481_REG_W_THERM_HYST, regValue);
    return status;
}
