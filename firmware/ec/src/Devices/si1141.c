/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "inc/devices/si1141.h"

#include "devices/i2c/threaded_int.h"
#include "inc/common/byteorder.h"
#include "inc/common/global_header.h"
#include "helpers/memory.h"
#include "inc/global/post_frame.h"

#define SI1141_PART_ID_REG 0x00
#define SI1141_REV_ID_REG 0x01
#define SI1141_SEQ_ID_REG 0x02
#define SI1141_PART_ID 0x41
#define SI1141_REV_ID 0x00
#define SI1141_SEQ_ID 0x08

/*****************************************************************************
 **    FUNCTION NAME   : read_si1141_reg
 **
 **    DESCRIPTION     : Read a 16 bit value from si1141 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus read_si1141_reg(const SI1141_Dev *dev, uint8_t regAddress,
                                    uint16_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle siHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!siHandle) {
        LOGGER_ERROR(
            "SI1141SENSOR:ERROR:: Failed to get I2C Bus for SI1141 sensor "
            "0x%x on bus 0x%x.\n",
            dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        status = i2c_reg_read(siHandle, dev->cfg.dev.slave_addr, regAddress,
                              regValue, 1);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : read_si1141_reg
 **
 **    DESCRIPTION     : Read a 16 bit value from si1141 register.
 **
 **    ARGUMENTS       : i2c device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus write_si1141_reg(const SI1141_Dev *dev, uint8_t regAddress,
                                     uint16_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle siHandle = i2c_get_handle(dev->cfg.dev.bus);
    if (!siHandle) {
        LOGGER_ERROR(
            "SI1141SENSOR:ERROR:: Failed to get I2C Bus for SI1141 sensor "
            "0x%x on bus 0x%x.\n",
            dev->cfg.dev.slave_addr, dev->cfg.dev.bus);
    } else {
        regValue = htobe16(regValue);
        status = i2c_reg_write(siHandle, dev->cfg.dev.slave_addr, regAddress,
                               regValue, 2);
    }
    return status;
}

ReturnStatus si1141_init(SI1141_Dev *dev)
{
    ReturnStatus status;
    status = write_si1141_reg(dev, 0x07, 0x17);
    write_si1141_reg(dev, 0x08, 0xB9); /*  MEAS_RATE 100ms*/
    write_si1141_reg(dev, 0x0A, 0x08); /*PS_RATE @ 0x0A*/
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_rev_id
 **
 **    DESCRIPTION     : Read the device id of proximity sensor.
 **
 **    ARGUMENTS       : i2c device and pointer to device Id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus si1141_getSeqId(SI1141_Dev *dev, uint16_t *seqId)
{
    return read_si1141_reg(dev, SI1141_SEQ_ID_REG, seqId);
}

/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_rev_id
 **
 **    DESCRIPTION     : Read the device id of proximity sensor.
 **
 **    ARGUMENTS       : i2c device and pointer to device Id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus si1141_getRevId(SI1141_Dev *dev, uint16_t *revId)
{
    return read_si1141_reg(dev, SI1141_REV_ID_REG, revId);
}
/*****************************************************************************
 **    FUNCTION NAME   : curr_sens_get_part_id
 **
 **    DESCRIPTION     : Read the device id of proximity sensor.
 **
 **    ARGUMENTS       : i2c device and pointer to device Id.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus si1141_getPartId(SI1141_Dev *dev, uint16_t *partId)
{
    return read_si1141_reg(dev, SI1141_PART_ID_REG, partId);
}

ePostCode si1141_probe(SI1141_Dev *dev, POSTData *postData)
{
    uint16_t partId = 0x00;
    uint16_t revId = 0x0000;
    if (si1141_getPartId(dev, &partId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (partId != SI1141_PART_ID) {
        return POST_DEV_ID_MISMATCH;
    }
    if (si1141_getRevId(dev, &revId) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (revId != SI1141_REV_ID) {
        return POST_DEV_ID_MISMATCH;
    }
    post_update_POSTData(postData, dev->cfg.dev.bus, dev->cfg.dev.slave_addr,
                         revId, partId);
    return POST_DEV_FOUND;
}
