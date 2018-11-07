/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * This file is used as Device layer for AT45DB641E. Mainly it contains Data
 * read, Data write, Page erase, Status check functions, these functions are
 * called by littlefs filesystyem in order to perform read/write operation for
 * data using SPI interface. Also while post execution device and manufacturing
 * id's of AT45DB641E will be verified by probe function.
 */

#include "inc/devices/at45db.h"
#include "inc/common/spibus.h"
#include "inc/common/global_header.h"
#include "inc/global/OC_CONNECT1.h"

#define AT45DB_DATA_WR_OPCODE_WR_COUNT 4
#define AT45DB_DATA_RD_OPCODE_WR_COUNT 8
#define AT45DB_DEVICE_ID 0x0028
#define AT45DB_DEVID_RD_BYTES 2
#define AT45DB_DEVID_RD_OPCODE 0x9F
#define AT45DB_DEVID_OPCODE_WR_COUNT 1
#define AT45DB_ERASE_OPCODE_WR_COUNT 4
#define AT45DB_MANFACTURE_ID 0x1F
#define AT45DB_PAGE_ERASE_OPCODE 0x81
#define AT45DB_PAGE_RD_OPCODE 0xD2
#define AT45DB_PAGE_WR_OPCODE 0x86
#define AT45DB_READY 0x80 /* AT45DB Ready Value */
#define AT45DB_SRAM_BUFF2_WR_OPCODE 0x87
#define AT45DB_STATUS_OPCODE 0xD7
#define AT45DB_STATUS_OPCODE_WR_COUNT 1
#define AT45DB_STATUS_RD_BYTES 1

#define waitForReady(dev)                                    \
    while (!(AT45DB_READY & at45db_readStatusRegister(dev))) \
        ;

/*****************************************************************************
 **    FUNCTION NAME   : AT45DB_read_reg
 **
 **    DESCRIPTION     : Reads 8 bit values from at45db page or register.
 **
 **    ARGUMENTS       : spi device configuration, cmd buffer, register value,
 **                      page offset, numOfBytes to be read, cmd write count.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus AT45DB_read_reg(AT45DB_Dev *dev,
                                    void *cmdbuffer, /* cmd or opcode buffer */
                                    uint8_t *regValue, uint32_t pageOffset,
                                    uint32_t NumOfbytes, uint8_t writeCount)
{
    ReturnStatus status = RETURN_NOTOK;

    SPI_Handle at45dbHandle = spi_get_handle(dev->cfg.dev.bus);
    if (!at45dbHandle) {
        LOGGER_ERROR(
            "AT45DBFLASHMEMORY:ERROR:: Failed to get SPI Bus for at45db flash memory "
            "0x%x on bus 0x%x.\n",
            dev->cfg.dev.chip_select, dev->cfg.dev.bus);
    } else {
        status = spi_reg_read(at45dbHandle, dev->cfg.dev.chip_select, cmdbuffer,
                              regValue, NumOfbytes, pageOffset, writeCount);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : AT45DB_write_reg
 **
 **    DESCRIPTION     : Write 8 bit value to at45db page or register.
 **
 **    ARGUMENTS       : spi device configuration, cmd buffer, register value,
 **                      page offset, numOfBytes to be written, cmd write count.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus AT45DB_write_reg(AT45DB_Dev *dev,
                                     void *cmdbuffer, /* cmd or opcode buffer */
                                     uint8_t *regValue, uint32_t pageOffset,
                                     uint32_t NumOfbytes, uint8_t writeCount)
{
    ReturnStatus status = RETURN_NOTOK;
    SPI_Handle at45dbHandle = spi_get_handle(dev->cfg.dev.bus);
    if (!at45dbHandle) {
        LOGGER_ERROR(
            "AT45DBFLASHMEMORY:ERROR:: Failed to get SPI Bus for at45db flash memory "
            "0x%x on bus 0x%x.\n",
            dev->cfg.dev.chip_select, dev->cfg.dev.bus);
    } else {
        status =
            spi_reg_write(at45dbHandle, dev->cfg.dev.chip_select, cmdbuffer,
                          regValue, NumOfbytes, pageOffset, writeCount);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : at45db_readStatusRegister
 **
 **    DESCRIPTION     : Reads status of at45db device whether it is ready for
 **
 **                      r/w operation
 **
 **    ARGUMENTS       : spi device configuration
 **
 **    RETURN TYPE     : 8-bit status code
 **
 *****************************************************************************/
uint8_t at45db_readStatusRegister(AT45DB_Dev *dev)
{
    uint8_t txBuffer =
        AT45DB_STATUS_OPCODE; /* opcode for ready status of AT45DB */
    ;
    uint8_t status;

    AT45DB_read_reg(dev, &txBuffer, &status, NULL, AT45DB_STATUS_RD_BYTES,
                    AT45DB_STATUS_OPCODE_WR_COUNT);

    return (status);
}

/*****************************************************************************
 **    FUNCTION NAME   : at45db_erasePage
 **
 **    DESCRIPTION     : Erases at45db memory page before writing data to it
 **
 **    ARGUMENTS       : spi device configuration, page number to be erased
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus at45db_erasePage(AT45DB_Dev *dev, uint32_t page)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t txBuffer[4];

    waitForReady(dev);

    txBuffer[0] =
        AT45DB_PAGE_ERASE_OPCODE; /* opcode to erase main memory page */
    txBuffer[1] =
        (uint8_t)(page >> 7); /* Page size is 15 bits 8 in tx1 and 7 in tx2 */
    txBuffer[2] = (uint8_t)(page << 1);
    txBuffer[3] = 0x00;

    status = AT45DB_write_reg(dev, txBuffer, NULL, NULL, NULL,
                              AT45DB_ERASE_OPCODE_WR_COUNT);

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : at45db_data_read
 **
 **    DESCRIPTION     : Reads data from at45db memory page
 **
 **    ARGUMENTS       : spi device configuration, data pointer, data size,
 **
 **                      page offset, page number
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus at45db_data_read(AT45DB_Dev *dev, uint8_t *data,
                              uint32_t data_size, uint32_t byte, uint32_t page)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t
        txBuffer[8]; /* last 4 bytes are needed, but have don't care values */

    waitForReady(dev);

    txBuffer[0] = AT45DB_PAGE_RD_OPCODE; /* opcode to read main memory page */
    txBuffer[1] =
        (uint8_t)(page >> 7); /* Page size is 15 bits 8 in tx1 and 7 in tx2 */
    txBuffer[2] = (uint8_t)((page << 1));
    txBuffer[3] = (uint8_t)(0xFF & byte);

    status = AT45DB_read_reg(dev, &txBuffer, data, byte, data_size,
                             AT45DB_DATA_RD_OPCODE_WR_COUNT);

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : at45db_data_write
 **
 **    DESCRIPTION     : Writes data to at45db memory page
 **
 **    ARGUMENTS       : spi device configuration, data pointer, data size,
 **
 **                      page offset, page number
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus at45db_data_write(AT45DB_Dev *dev, uint8_t *data,
                               uint32_t data_size, uint32_t byte, uint32_t page)
{
    ReturnStatus status = RETURN_NOTOK;
    uint8_t txBuffer[4];

    waitForReady(dev);

    txBuffer[0] = AT45DB_SRAM_BUFF2_WR_OPCODE; /* opcode to write data to AT45DB
                                                  SRAM Buffer2 */
    txBuffer[1] = 0x00;
    txBuffer[2] = (uint8_t)(0x1 & (byte >> 8)); /* 9 bit buffer address */
    txBuffer[3] = (uint8_t)(0xFF & byte);

    status = AT45DB_write_reg(dev, &txBuffer, data, byte, data_size,
                              AT45DB_DATA_WR_OPCODE_WR_COUNT);

    if (status == RETURN_OK) {
        waitForReady(dev);

        txBuffer[0] =
            AT45DB_PAGE_WR_OPCODE; /* opcode to Push the data from AT45DB SRAM
                                      Buffer2 to the page */
        txBuffer[1] = (uint8_t)(
            page >> 7); /* Page size is 15 bits 8 in tx1 and 7 in tx2 */
        txBuffer[2] = (uint8_t)(page << 1);
        txBuffer[3] = 0x00;

        status = AT45DB_write_reg(dev, &txBuffer, data, byte, data_size,
                                  AT45DB_DATA_WR_OPCODE_WR_COUNT);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : at45db_getDevID
 **
 **    DESCRIPTION     : Reads Device id and manufacturing id of at45db device
 **
 **    ARGUMENTS       : spi device configuration, data pointer
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus at45db_getDevID(AT45DB_Dev *dev, uint32_t *devID)
{
    uint8_t txBuffer = AT45DB_DEVID_RD_OPCODE; /* opcode to get device id */

    return AT45DB_read_reg(dev, &txBuffer, devID, NULL, AT45DB_DEVID_RD_BYTES,
                           AT45DB_DEVID_OPCODE_WR_COUNT);
}

/*****************************************************************************
 **    FUNCTION NAME   : at45db_probe
 **
 **    DESCRIPTION     : Compares device and manufacturing id's for post
 **
 **    ARGUMENTS       : spi device configuration, post data pointer
 **
 **    RETURN TYPE     : ePostCode type status, can be found in post_frame.h
 **
 *****************************************************************************/
ePostCode at45db_probe(AT45DB_Dev *dev, POSTData *postData)
{
    uint32_t value = 0;
    uint16_t devId = 0;
    uint8_t manfId = 0;

    if (at45db_getDevID(dev, &value) != RETURN_OK) {
        return POST_DEV_MISSING;
    }

    devId = (value >> 8) & 0xFFFF;

    if (devId != AT45DB_DEVICE_ID) {
        return POST_DEV_ID_MISMATCH;
    }

    manfId = value & 0xFF;

    if (manfId != AT45DB_MANFACTURE_ID) {
        return POST_DEV_ID_MISMATCH;
    }

    post_update_POSTData(postData, dev->cfg.dev.bus, NULL, manfId, devId);

    return POST_DEV_FOUND;
}
