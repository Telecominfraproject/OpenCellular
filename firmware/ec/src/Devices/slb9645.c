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
#include "inc/devices/slb9645.h"
#include "devices/i2c/threaded_int.h"
#include "inc/common/byteorder.h"
#include "inc/common/global_header.h"
#include "helpers/memory.h"

#define SLB9645_DEV_ID 0x0001A15D1
#define SLB9645_DIEID_REG 0x0006
#define TPM_ACCESS 0x0000
#define TPM_STS 0x0001
#define TPM_DATA_FIFO 0x0005

/*****************************************************************************
 **    FUNCTION NAME   : read_slb9645_reg
 **
 **    DESCRIPTION     : Read a 16 bit value from slb9645 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus read_slb9645_reg(const SLB9645_Dev *dev, uint8_t regAddress,
                                     uint32_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle slbHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!slbHandle) {
        LOGGER_ERROR("TPM(slb9645):ERROR:: Failed to get I2C Bus for SLB9645 "
                     "0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        status = i2c_reg_read(slbHandle, dev->cfg.dev.slave_addr, regAddress,
                              regValue, 4);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : write_slb9645_reg
 **
 **    DESCRIPTION     : Write 16 bit value to slb9645 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus write_slb9645_reg(const SLB9645_Dev *dev,
                                      uint16_t regAddress, uint32_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle slbHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!slbHandle) {
        LOGGER_ERROR("TPM(slb9645):ERROR:: Failed to get I2C Bus for SLB9645 "
                     "0x%x on bus 0x%x.\n",
                     dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        regValue = htobe16(regValue);
        status = i2c_reg_write(slbHandle, dev->cfg.dev.slave_addr, regAddress,
                               regValue, 2);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : slb9645_get_dev_id
 **
 **    DESCRIPTION     : Read the device id of Current sensor.
 **
 **    ARGUMENTS       : i2c device and pointer to device Id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus slb9645_getDevId(SLB9645_Dev *dev, uint32_t *devID)
{
    return read_slb9645_reg(dev, SLB9645_DIEID_REG, devID);
}

ePostCode slb9645_probe(SLB9645_Dev *dev, POSTData *postData)
{
    uint32_t devId = 0x00;
    uint16_t manfId = 0x0000;
    if (slb9645_getDevId(dev, &devId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (devId != SLB9645_DEV_ID) {
        return POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, dev->cfg.dev.bus, dev->cfg.dev.slave_addr,
                         manfId, devId);
    return POST_DEV_FOUND;
}
