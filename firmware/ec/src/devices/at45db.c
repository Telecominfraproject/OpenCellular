/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*
*/

#include "inc/devices/at45db.h"
#include "inc/common/spibus.h"
#include "inc/common/global_header.h"
#include <inc/global/OC_CONNECT1.h>

#define AT45DB_MANFACTURE_ID            0x1F
#define AT45DB_DEVICE_ID                0x0028
#define AT45DB_READY                    0x80 /* AT45DB Ready Value */
#define AT45DB_STATUS_OPCODE            0xD7
#define AT45DB_PAGE_ERASE_OPCODE        0x81
#define AT45DB_PAGE_RD_OPCODE           0xD2
#define AT45DB_SRAM_BUFF2_WR_OPCODE     0x87
#define AT45DB_PAGE_WR_OPCODE           0x86
#define AT45DB_DEVID_RD_OPCODE          0x9F

#define AT45DB_STATUS_OPCODE_WR_COUNT   1
#define AT45DB_DEVID_OPCODE_WR_COUNT    1
#define AT45DB_ERASE_OPCODE_WR_COUNT    4
#define AT45DB_DATA_WR_OPCODE_WR_COUNT  4
#define AT45DB_DATA_RD_OPCODE_WR_COUNT  8
#define AT45DB_STATUS_RD_BYTES          1
#define AT45DB_DEVID_RD_BYTES           2

#define waitForReady(dev) \
    while (!(AT45DB_READY & at45db_readStatusRegister(dev)));

/*****************************************************************************
 **    FUNCTION NAME   : AT45DB_read_reg
 **
 **    DESCRIPTION     : Read a 8 bit value from at45db page or register.
 **
 **    ARGUMENTS       : spi device, Register address and value
 **                      to be read.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus AT45DB_read_reg(AT45DB_Dev *dev,
                                    void *cmdbuffer,    /* cmd or opcode buffer */
                                    uint8_t *regValue,
                                    uint32_t pageOffset,
                                    uint32_t NumOfbytes,
                                    uint8_t writeCount)
{
    ReturnStatus status = RETURN_NOTOK;

    SPI_Handle at45dbHandle = spi_get_handle(dev->cfg.dev.bus);
    if (!at45dbHandle) {
        LOGGER_ERROR("AT45DBFLASHMEMORY:ERROR:: Failed to get SPI Bus for at45db flash memory "
                     "0x%x on bus 0x%x.\n", dev->cfg.dev.chip_select,
                     dev->cfg.dev.bus);
    } else {
        status = spi_reg_read(at45dbHandle,
                              dev->cfg.dev.chip_select,
                              cmdbuffer,
                              regValue,
                              NumOfbytes,
                              pageOffset,
                              writeCount);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : AT45DB_write_reg
 **
 **    DESCRIPTION     : Write 8 bit value to at45db page or register.
 **
 **    ARGUMENTS       : spi device, Register address and value
 **                      to be written.
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
static ReturnStatus AT45DB_write_reg(AT45DB_Dev *dev,
                                     void *cmdbuffer, /* cmd or opcode buffer */
                                     uint8_t *regValue,
                                     uint32_t pageOffset,
                                     uint32_t NumOfbytes,
                                     uint8_t writeCount)
{
    ReturnStatus status = RETURN_NOTOK;
    SPI_Handle at45dbHandle = spi_get_handle(dev->cfg.dev.bus);
    if (!at45dbHandle) {
        LOGGER_ERROR("AT45DBFLASHMEMORY:ERROR:: Failed to get SPI Bus for at45db flash memory "
                     "0x%x on bus 0x%x.\n", dev->cfg.dev.chip_select,
                     dev->cfg.dev.bus);
    } else {
        status = spi_reg_write(at45dbHandle,
                               dev->cfg.dev.chip_select,
                               cmdbuffer,
                               regValue,
                               NumOfbytes,
                               pageOffset,
                               writeCount);
    }
    return status;
}

unsigned char at45db_readStatusRegister(AT45DB_Dev *dev)
{
    unsigned char txBuffer = AT45DB_STATUS_OPCODE; /* opcode for ready status of AT45DB */;
    unsigned char status;

    AT45DB_read_reg(dev, &txBuffer, &status, NULL, AT45DB_STATUS_RD_BYTES, AT45DB_STATUS_OPCODE_WR_COUNT);

    return (status);
}

ReturnStatus at45db_erasePage(AT45DB_Dev *dev, uint32_t page)
{
    ReturnStatus status = RETURN_NOTOK;
    unsigned char txBuffer[4];

    waitForReady(dev);

    txBuffer[0] = AT45DB_PAGE_ERASE_OPCODE; /* opcode to erase main memory page */
    txBuffer[1] = (unsigned char)(page >> 7);  /* Page size is 15 bits 8 in tx1 and 7 in tx2 */
    txBuffer[2] = (unsigned char)(page << 1);
    txBuffer[3] = 0x00;

    status = AT45DB_write_reg(dev, txBuffer, NULL, NULL, NULL, AT45DB_ERASE_OPCODE_WR_COUNT);

    return status;
}

ReturnStatus at45db_data_read(AT45DB_Dev *dev, uint8_t *data, uint32_t data_size, uint32_t byte, uint32_t page)
{
    ReturnStatus status = RETURN_NOTOK;
    unsigned char txBuffer[8]; /* last 4 bytes are needed, but have don't care values */

    waitForReady(dev);

    txBuffer[0] = AT45DB_PAGE_RD_OPCODE; /* opcode to read main memory page */
    txBuffer[1] = (unsigned char)(page >> 7);  /* Page size is 15 bits 8 in tx1 and 7 in tx2 */
    txBuffer[2] = (unsigned char)((page << 1));
    txBuffer[3] = (unsigned char)(0xFF & byte);

    status = AT45DB_read_reg(dev, &txBuffer, data, byte, data_size, AT45DB_DATA_RD_OPCODE_WR_COUNT);

    return status;
}

ReturnStatus at45db_data_write(AT45DB_Dev *dev, uint8_t *data, uint32_t data_size, uint32_t byte, uint32_t page)
{
    ReturnStatus status = RETURN_NOTOK;
    unsigned char txBuffer[4];

    waitForReady(dev);

    txBuffer[0] = AT45DB_SRAM_BUFF2_WR_OPCODE; /* opcode to write data to AT45DB SRAM Buffer2 */
    txBuffer[1] = 0x00;
    txBuffer[2] = (unsigned char)(0x1 & (byte >> 8)); /* 9 bit buffer address */
    txBuffer[3] = (unsigned char)(0xFF & byte);

    status = AT45DB_write_reg(dev, &txBuffer, data, byte, data_size, AT45DB_DATA_WR_OPCODE_WR_COUNT);

    if(status == RETURN_OK){
       waitForReady(dev);

       txBuffer[0] = AT45DB_PAGE_WR_OPCODE; /* opcode to Push the data from AT45DB SRAM Buffer2 to the page */
       txBuffer[1] = (unsigned char)(page >> 7);  /* Page size is 15 bits 8 in tx1 and 7 in tx2 */
       txBuffer[2] = (unsigned char)(page << 1);
       txBuffer[3] = 0x00;

       status = AT45DB_write_reg(dev, &txBuffer, data, byte, data_size, AT45DB_DATA_WR_OPCODE_WR_COUNT);
    }
    return status;
}

static ReturnStatus at45db_getDevID(AT45DB_Dev *dev, uint32_t *devID)
{
    unsigned char txBuffer = AT45DB_DEVID_RD_OPCODE; /* opcode to get device id */

    return AT45DB_read_reg(dev, &txBuffer, devID, NULL, AT45DB_DEVID_RD_BYTES, AT45DB_DEVID_OPCODE_WR_COUNT);
}

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

    post_update_POSTData(postData, dev->cfg.dev.bus, NULL,manfId, devId);

    return POST_DEV_FOUND;
}
