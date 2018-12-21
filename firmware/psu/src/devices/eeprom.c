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
#include "inc/common/global_header.h"
#include "inc/common/byteorder.h"
#include "inc/devices/eeprom.h"

#include <ti/drivers/I2C.h>
#ifndef UT_FRAMEWORK
#include <driverlib/emac.h> /* TODO: for htons - clean up this random include */
#endif
#include <string.h>

#define WP_ASSERT   1
#define WP_DEASSERT 0

extern Eeprom_Cfg eeprom_psu_sid;
extern Eeprom_Cfg eeprom_psu_inv;

static ReturnStatus i2c_eeprom_write(I2C_Handle i2cHandle,
                                     uint8_t deviceAddress,
                                     uint16_t regAddress,
                                     const void *value,
                                     size_t numofBytes);

static ReturnStatus i2c_eeprom_read(I2C_Handle i2cHandle,
                                    uint16_t deviceAddress,
                                    uint16_t regAddress,
                                    void *value,
                                    size_t numofbytes);

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_init
 **
 **    DESCRIPTION     : Initialize an EEPROM device (WP gpio / test read)
 **
 **    ARGUMENTS       : EEPROM config
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
bool eeprom_init(Eeprom_Cfg *cfg) {
    /* Configure our WP pin (if any) and set to be low (protected) by default */
    if (cfg->pin_wp) {
        OcGpio_configure(cfg->pin_wp,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    }

    /* Test communication to the EEPROM */
    uint8_t test_byte;
    if (eeprom_read(cfg, 0x00, &test_byte, sizeof(test_byte)) != RETURN_OK) {
        return false;
    }

    return true;
}

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_read
 **
 **    DESCRIPTION     : Read the values from the EEPROM register.
 **
 **    ARGUMENTS       : EEPROM (Slave) address, Register address and
 **                      pointer to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_read(Eeprom_Cfg *cfg,
                         uint16_t address,
                         void *buffer,
                         size_t size)
{
    ReturnStatus status = RETURN_OK;
    I2C_Handle eepromHandle = i2c_get_handle(cfg->i2c_dev.bus);
    if (!eepromHandle) {
        LOGGER_ERROR("EEPROM:ERROR:: Failed to get I2C Bus for "
                     "EEPROM device 0x%x.\n", cfg->i2c_dev.slave_addr);
    } else {
        /* TODO: if we're concerned about hogging the bus, we could always
         * page reads, but this doesn't seem necessary right now
         */
        /* TODO: check for out-of-bounds addresses (some EEPROM wrap around
         * when reading after the end, so this could lead to confusion)
         */
        status = i2c_eeprom_read(eepromHandle, cfg->i2c_dev.slave_addr,
                                 address, buffer, size);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_write
 **
 **    DESCRIPTION     : Write the value to EEPROM register.
 **
 **    ARGUMENTS       :  EEPROM (Slave) address, Register address and value
 **                      to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_write(const Eeprom_Cfg *cfg,
                          uint16_t address,
                          const void *buffer,
                          size_t size)
{
    ReturnStatus status = RETURN_OK;
    I2C_Handle eepromHandle = i2c_get_handle(cfg->i2c_dev.bus);
    if (!eepromHandle) {
        LOGGER_ERROR("EEPROM:ERROR:: Failed to get I2C Bus for "
                     "EEPROM device 0x%x.\n", cfg->i2c_dev.slave_addr);
    } else {
        /* Respect EEPROM page size */
        const size_t page_size = cfg->type.page_size;
        if (page_size) {
            while (size > page_size) {
                status = i2c_eeprom_write(eepromHandle, cfg->i2c_dev.slave_addr,
                                          address, buffer, page_size);

                size -= page_size;
                address += page_size;
                buffer = (const uint8_t *)buffer + page_size;
            }
        }
        status = i2c_eeprom_write(eepromHandle, cfg->i2c_dev.slave_addr,
                                  address, buffer, size);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : i2c_eeprom_write
 **
 **    DESCRIPTION     : Writing device register over i2c bus.
 **
 **    ARGUMENTS       : I2C handle, device address, register address and value.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus i2c_eeprom_write(I2C_Handle i2cHandle,
                                     uint8_t slaveAddress,
                                     uint16_t memAddress,
                                     const void *value,
                                     size_t numofBytes)
{
    ReturnStatus status = RETURN_OK;
    uint8_t txBuffer[numofBytes + 1];

    /*TODO: This approach needs to be looked into when same
     * codebase is used for PSU and GBC

    uint8_t txBuffer[numofBytes + 2];
    *(uint16_t *)txBuffer = htobe16(memAddress);
    memcpy((txBuffer + 2), value, numofBytes);
    */

    *txBuffer = (memAddress & 0xFF);
    slaveAddress |= ((memAddress >> 8 ) & 0x01);
    memcpy((txBuffer + 1), value, numofBytes);
    I2C_Transaction i2cTransaction;
    i2cTransaction.slaveAddress = slaveAddress;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = sizeof(txBuffer);
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;
    if (I2C_transfer(i2cHandle, &i2cTransaction)) {
        LOGGER_DEBUG("EEPROM:INFO:: I2C write success for device: 0x%x reg Addr: 0x%x\n",
                     slaveAddress, memAddress);
        status = RETURN_OK;
    } else {
        LOGGER_ERROR("EEPROM:ERROR:: I2C write failed for for device: 0x%x reg Addr: 0x%x\n",
                     slaveAddress, memAddress);
        status = RETURN_NOTOK;
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : i2c_eeprom_read
 **
 **    DESCRIPTION     : Reading device register over i2c bus.
 **
 **    ARGUMENTS       : I2C handle, device address, register address and value.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus i2c_eeprom_read(I2C_Handle i2cHandle,
                                    uint16_t slaveAddress,
                                    uint16_t memAddress,
                                    void *value,
                                    size_t numofbytes)
{
    ReturnStatus status = RETURN_OK;
//    uint16_t txBuffer = htobe16(memAddress); /* Address is big-endian */
    uint8_t txBuffer = (memAddress & 0xFF); /* Address is big-endian */
    slaveAddress |= ((memAddress >> 8 )& 0x01);

    I2C_Transaction i2cTransaction;
    i2cTransaction.slaveAddress = slaveAddress;
    i2cTransaction.writeBuf = &txBuffer;
    i2cTransaction.writeCount = sizeof(txBuffer);
    i2cTransaction.readBuf = value;
    i2cTransaction.readCount = numofbytes;
    if (I2C_transfer(i2cHandle, &i2cTransaction)) {
        LOGGER_DEBUG("EEPROM:INFO:: I2C read success for device: 0x%x reg Addr: 0x%x\n",
                     slaveAddress, memAddress);
        status = RETURN_OK;
    } else {
        LOGGER_ERROR("EEPROM:ERROR:: I2C write failed for for device: 0x%x reg Addr: 0x%x\n",
                     slaveAddress, memAddress);
        status = RETURN_NOTOK;
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_disable_write
 **
 **    DESCRIPTION     : Read the values from the EEPROM register.
 **
 **    ARGUMENTS       : EEPROM handle.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_disable_write(Eeprom_Cfg *cfg)
{
    if (cfg->pin_wp) {
        OcGpio_write(cfg->pin_wp, WP_ASSERT);
    }

    /* TODO: error detection */
    return RETURN_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_enable_write
 **
 **    DESCRIPTION     : Enable eeprom write operation.
 **
 **    ARGUMENTS       : EEPROM handle.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_enable_write(Eeprom_Cfg *cfg)
{
    if (cfg->pin_wp) {
        OcGpio_write(cfg->pin_wp, WP_DEASSERT);
    }

    /* TODO: error detection */
    return RETURN_OK;
}

#if 0
/*
 * The SKU method of turning on powere lines is currently not needed , we will keep it in codebase
 * if similar implementation is needed in future
 */

void eeprom_init_powerLines(Eeprom_Cfg *eeprom_cfg )
{
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_24v,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_5v0,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_3v3,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_gbcv2_on,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_12v_bb,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_12v_fe,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_20v_fe,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&eeprom_cfg->power_cfg->pin_1v8,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
}

ePostCode eeprom_handle_sku_id(Eeprom_Cfg *eeprom_cfg , Eeprom_Skucfg *sku_id)
{
    eeprom_init_powerLines(eeprom_cfg);
    OcGpio_write(&eeprom_cfg->power_cfg->pin_12v_bb, 1);

    switch (sku_id->band_nbr) {
        case BAND3:
            OcGpio_write(&eeprom_cfg->power_cfg->pin_5v0, 1);
            OcGpio_write(&eeprom_cfg->power_cfg->pin_3v3, 1);
            OcGpio_write(&eeprom_cfg->power_cfg->pin_1v8, 1);
            OcGpio_write(&eeprom_cfg->power_cfg->pin_20v_fe, 1);
            break;

        case BAND5:
            OcGpio_write(&eeprom_cfg->power_cfg->pin_12v_fe, 1);
            break;

        case BAND28:
            OcGpio_write(&eeprom_cfg->power_cfg->pin_12v_fe, 1);
            break;

        default:
            /* Do Nothing */
    }

    if(sku_id->gbc_presence) {
        OcGpio_write(&eeprom_cfg->power_cfg->pin_gbcv2_on, 1);
    }
    return POST_DEV_FOUND;
}
#endif
