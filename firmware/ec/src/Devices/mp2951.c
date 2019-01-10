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
#include "inc/devices/mp2951.h"

#include "devices/i2c/threaded_int.h"
#include "inc/common/byteorder.h"
#include "inc/common/global_header.h"
#include "helpers/memory.h"

#define MP2951_PRODUCT_ID 0x51
#define MP2951_VENDOR_ID 0x25
#define MP2951_VENDOR_ID_REG 0xEF
#define MP2951_PRODUCT_ID_REG 0xF0

#define MP2951_INPUT_VOL_ON_REG 0x35
#define MP2951_INPUT_VOL_OFF_REG 0x36
#define BIT_MASK 0x007F /*bit mask for 7-bit value */

/*****************************************************************************
 **    FUNCTION NAME   : read_mp2951_reg
 **
 **    DESCRIPTION     : Read a 16 bit value from mp2951 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus read_mp2951_reg(const MP2951_Dev *dev, uint8_t regAddress,
                                    uint16_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle mpHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!mpHandle) {
        LOGGER_ERROR("MP2951:ERROR:: Failed to get I2C Bus for MP2951 "
                     "0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        status = i2c_reg_read(mpHandle, dev->cfg.dev.slave_addr, regAddress,
                              regValue, 1);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : write_mp2951_reg
 **
 **    DESCRIPTION     : Write 16 bit value to mp2951 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus write_mp2951_reg(const MP2951_Dev *dev, uint8_t regAddress,
                                     uint16_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle mpHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!mpHandle) {
        LOGGER_ERROR("MP2951:ERROR:: Failed to get I2C Bus for MP2951 "
                     "0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        status = i2c_reg_write(mpHandle, dev->cfg.dev.slave_addr, regAddress,
                               regValue, 2);
    }
    return status;
}

ReturnStatus MP2951_setInputVolOnLim(MP2951_Dev *dev, uint16_t voltLim)
{
    return write_mp2951_reg(dev, MP2951_INPUT_VOL_ON_REG, voltLim);
}

ReturnStatus MP2951_setInputVolOffLim(MP2951_Dev *dev, uint16_t voltLim)
{
    return write_mp2951_reg(dev, MP2951_INPUT_VOL_OFF_REG, voltLim);
}

ReturnStatus MP2951_readInputVolOnLim(MP2951_Dev *dev, uint16_t *voltLim)
{
    uint16_t regValue = 0x0000;
    ReturnStatus status =
        read_mp2951_reg(dev, MP2951_INPUT_VOL_ON_REG, &regValue);
    if (status == RETURN_OK) {
        *voltLim = regValue & BIT_MASK;
        LOGGER_DEBUG("MP2951:INFO:: MP2951 0x%x on bus 0x%x is "
                     "reporting input voltage ON limit  of %d V.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus, voltLim);
    }
    return status;
}

ReturnStatus MP2951_readInputVolOffLim(MP2951_Dev *dev, uint16_t *voltLim)
{
    uint16_t regValue = 0x0000;
    ReturnStatus status =
        read_mp2951_reg(dev, MP2951_INPUT_VOL_OFF_REG, &regValue);
    if (status == RETURN_OK) {
        *voltLim = regValue & BIT_MASK;
        LOGGER_DEBUG("MP2951:INFO:: MP2951 0x%x on bus 0x%x is "
                     "reporting input voltage OFF limit  of %d V.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus, voltLim);
    }
    return status;
}

ReturnStatus mp2951_getVendId(MP2951_Dev *dev, uint8_t *vId)
{
    return read_mp2951_reg(dev, MP2951_VENDOR_ID_REG, vId);
}
/*****************************************************************************
 **    FUNCTION NAME   : mp2951_get_dev_id
 **
 **    DESCRIPTION     : Read the device id of mp2951.
 **
 **    ARGUMENTS       : i2c device and pointer to device Id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus mp2951_getDevId(MP2951_Dev *dev, uint8_t *devID)
{
    return read_mp2951_reg(dev, MP2951_PRODUCT_ID_REG, devID);
}

ePostCode mp2951_probe(MP2951_Dev *dev, POSTData *postData)
{
    uint8_t pId = 0x00;   /*product id */
    uint8_t vId = 0x0000; /*vendor id */
    if (mp2951_getDevId(dev, &pId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (pId != MP2951_PRODUCT_ID) {
        return POST_DEV_ID_MISMATCH;
    }
    if (mp2951_getVendId(dev, &vId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (vId != MP2951_VENDOR_ID) {
        return POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, dev->cfg.dev.bus, dev->cfg.dev.slave_addr,
                         vId, pId);
    return POST_DEV_FOUND;
}
