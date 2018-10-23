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
#include "inc/devices/eeprom.h"

#include "Board.h"
#include "inc/common/global_header.h"
#include "inc/common/byteorder.h"

#include <ti/drivers/I2C.h>
#ifndef UT_FRAMEWORK
#include <driverlib/emac.h> /* TODO: for htons - clean up this random include */
#endif
#include <string.h>

#define WP_ASSERT   1
#define WP_DEASSERT 0

extern Eeprom_Cfg eeprom_gbc_sid;
extern Eeprom_Cfg eeprom_gbc_inv;
extern Eeprom_Cfg eeprom_sdr_inv;
extern Eeprom_Cfg eeprom_fe_inv;

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
ReturnStatus eeprom_read(const Eeprom_Cfg *cfg,
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
    uint8_t txBuffer[numofBytes + 2];
    *(uint16_t *)txBuffer = htobe16(memAddress); /* Address is big-endian */

    /*Copy the values to the location after first two bytes*/
    memcpy((txBuffer + 2), value, numofBytes);
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
    uint16_t txBuffer = htobe16(memAddress); /* Address is big-endian */

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

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_read_oc_info
 **
 **    DESCRIPTION     : Read the info about OC connect1 box from the EEPROM register.
 **
 **    ARGUMENTS       : EEPROM (Slave) address, Register address and
 **                      pointer to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_read_oc_info(uint8_t * oc_serial)
{
    ReturnStatus status = RETURN_NOTOK;
    status = eeprom_read(&eeprom_gbc_sid, OC_CONNECT1_SERIAL_INFO,
                         oc_serial, OC_CONNECT1_SERIAL_SIZE);
    if (status != RETURN_OK) {
        LOGGER_ERROR("EEPROM:ERROR:: Failed to get I2C Bus for GBC serial ID EEPROM.\n");
    } else {
        LOGGER_ERROR("EEPROM:Info:: OC Connect1 %d.\n", *oc_serial);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_read_board_info
 **
 **    DESCRIPTION     : Read the info about various board from the EEPROM register.
 **
 **    ARGUMENTS       : EEPROM (Slave) address, Register address and
 **                      pointer to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_read_board_info(const Eeprom_Cfg *cfg, uint8_t * rom_info)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t info_size = 0x00;
    uint16_t eepromOffset = 0x0000;
    switch (cfg->ss) {
        case OC_SS_SYS:
        {
            info_size = OC_GBC_BOARD_INFO_SIZE;
            eepromOffset = OC_GBC_BOARD_INFO;
            break;
        }
        case OC_SS_SDR:
        {
            info_size = OC_SDR_BOARD_INFO_SIZE;
            eepromOffset = OC_SDR_BOARD_INFO;
            break;
        }
        case OC_SS_RF:
        {
            info_size = OC_RFFE_BOARD_INFO_SIZE;
            eepromOffset = OC_RFFE_BOARD_INFO;
            break;
        }
        default:
        {
            return status;
        }
    }
    status = eeprom_read(cfg, eepromOffset, rom_info, info_size);
    if (status != RETURN_OK) {
        LOGGER_ERROR("EEPROM:ERROR:: Failed to get I2C Bus for EEPROM device 0x%x.\n",
                     cfg->i2c_dev.slave_addr);
    } else {
        LOGGER_ERROR("EEPROM:Info:: OC Connect1 %s.\n", rom_info);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_read_device_info_record
 **
 **    DESCRIPTION     : Read the device info about subsystem from the EEPROM
 **                      register.
 **
 **    ARGUMENTS       : EEPROM (Slave) address, Register address and
 **                      pointer to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_read_device_info_record(const Eeprom_Cfg *cfg,
                                            uint8_t recordNo,
                                            char * device_info)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t info_size = OC_DEVICE_INFO_SIZE;
    uint16_t eepromOffset = 0x0000;
    switch (cfg->ss) {
        case OC_SS_SYS:
        {
            eepromOffset = OC_GBC_DEVICE_INFO + (recordNo * info_size);
            break;
        }
        case OC_SS_SDR:
        {
            eepromOffset = OC_SDR_DEVICE_INFO + (recordNo * info_size);
            break;
        }
        case OC_SS_RF:
        {
            eepromOffset = OC_RFFE_DEVICE_INFO + (recordNo * info_size);
            break;
        }
        default:
        {
            return status;
        }
    }
    status = eeprom_read(cfg, eepromOffset, device_info, info_size);
    if (status != RETURN_OK) {
        LOGGER_ERROR("EEPROM:ERROR:: Failed to get I2C Bus for EEPROM device 0x%x.\n",
                     cfg->i2c_dev.slave_addr);
    } else {
        LOGGER_ERROR("EEPROM:Info:: Record read for  0x%x.\n",
                     cfg->i2c_dev.slave_addr);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : eeprom_write_device_info_record
 **
 **    DESCRIPTION     : Write the device info about subsystem from the EEPROM
 **                      register.
 **
 **    ARGUMENTS       : EEPROM (Slave) address, Register address and
 **                      pointer to value read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus eeprom_write_device_info_record(Eeprom_Cfg *cfg,
                                             uint8_t recordNo,
                                             char * device_info)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t info_size = OC_DEVICE_INFO_SIZE;
    uint16_t eepromOffset = 0x0000;
    switch (cfg->ss) {
        case OC_SS_SYS:
        {
            eepromOffset = OC_GBC_DEVICE_INFO + (recordNo * info_size);
            break;
        }
        case OC_SS_SDR:
        {
            eepromOffset = OC_SDR_DEVICE_INFO + (recordNo * info_size);
            break;
        }
        case OC_SS_RF:
        {
            eepromOffset = OC_RFFE_DEVICE_INFO + (recordNo * info_size);
            break;
        }
        default:
        {
            return status;
        }
    }
    status = eeprom_write(cfg, eepromOffset, device_info, info_size);
    if (status != RETURN_OK) {
        LOGGER_ERROR("EEPROM:ERROR:: Failed to get I2C Bus for EEPROM device 0x%x.\n",
                     cfg->i2c_dev.slave_addr);
    } else {
        LOGGER_ERROR("EEPROM:Info:: Record written for  0x%x.\n",
                     cfg->i2c_dev.slave_addr);
    }
    return status;
}
