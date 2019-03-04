/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * This file contains SPI driver's API within spi_get_handle, spi_reg_read and
 * spi_reg_write which ccan be called by device layer to communicate any SPI
 * device.
 */

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************

#include "Board.h"
#include "drivers/OcGpio.h"
#include "inc/common/spibus.h"
#include "inc/global/OC_CONNECT1.h"
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/gates/GateMutex.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>

#define PIN_LOW (0)
#define PIN_HIGH ~(0)

/*****************************************************************************
 **    FUNCTION NAME   : spi_get_handle
 **
 **    DESCRIPTION     : Initialize SPI Bus
 **
 **    ARGUMENTS       : SPI bus index
 **
 **    RETURN TYPE     : SPI_Handle (NULL on failure)
 **
 *****************************************************************************/
SPI_Handle spi_get_handle(uint32_t index)
{
    SPI_Params spiParams;
    SPI_Handle spiHandle;

    SPI_Params_init(&spiParams);
    spiHandle = SPI_open(index, &spiParams);
    if (spiHandle == NULL) {
        LOGGER_ERROR("SPI_open failed\n");
        return false;
    }
    return spiHandle;
}

/*****************************************************************************
 **    FUNCTION NAME   : spi_reg_read
 **
 **    DESCRIPTION     : Writing device register over SPI bus.
 **
 **    ARGUMENTS       : SPI handle, chip select, register address, data, data
 **
 **                      length, offset byte, numOfBytes for cmd write count
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus spi_reg_read(SPI_Handle spiHandle, OcGpio_Pin *chip_select,
                          void *regAddress, uint8_t *data, uint32_t data_size,
                          uint32_t byte, uint8_t numofBytes)
{
    ReturnStatus status = RETURN_OK;
    SPI_Transaction spiTransaction;

    spiTransaction.count =
        numofBytes; /* Initialize master SPI transaction structure */
    spiTransaction.txBuf = regAddress;
    spiTransaction.rxBuf = NULL;

    OcGpio_write(chip_select, PIN_LOW); /* Initiate SPI transfer */

    if (SPI_transfer(spiHandle, &spiTransaction)) {
        status = RETURN_OK;
    } else {
        LOGGER_ERROR("SPIBUS:ERROR:: SPI write failed");
        status = RETURN_NOTOK;
    }

    spiTransaction.count = data_size;
    spiTransaction.txBuf = NULL;
    spiTransaction.rxBuf = data;

    if (SPI_transfer(spiHandle, &spiTransaction)) {
        status = RETURN_OK;
    } else {
        LOGGER_ERROR("SPIBUS:ERROR:: SPI read failed");
        status = RETURN_NOTOK;
    }
    OcGpio_write(chip_select, PIN_HIGH);

    SPI_close(spiHandle);

    return (status);
}

/*****************************************************************************
 **    FUNCTION NAME   : spi_reg_write
 **
 **    DESCRIPTION     : Writing device register over SPI bus.
 **
 **    ARGUMENTS       : SPI handle, chip select, register address, data, data
 **
 **                      length, offset byte, numOfBytes for cmd write count
 **
 **    RETURN TYPE     : Success or failure
 **
 *****************************************************************************/
ReturnStatus spi_reg_write(SPI_Handle spiHandle, OcGpio_Pin *chip_select,
                           void *regAddress, uint8_t *data, uint32_t data_size,
                           uint32_t byte, uint8_t numofBytes)
{
    ReturnStatus status = RETURN_OK;
    SPI_Transaction spiTransaction;

    spiTransaction.count =
        numofBytes; /* Initialize master SPI transaction structure */
    spiTransaction.txBuf = regAddress;
    spiTransaction.rxBuf = NULL;

    OcGpio_write(chip_select, PIN_LOW); /* Initiate SPI transfer */

    if (SPI_transfer(spiHandle, &spiTransaction)) {
        status = RETURN_OK;
    } else {
        LOGGER_ERROR("SPIBUS:ERROR:: SPI write failed");
        status = RETURN_NOTOK;
    }

    spiTransaction.count = data_size;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
    /* NOTE: const is discarded by SPI_transfer in TI RTOS. */
    spiTransaction.txBuf = data;
#pragma GCC diagnostic pop
    spiTransaction.rxBuf = NULL;

    if (data_size > 0) {
        if (SPI_transfer(spiHandle, &spiTransaction)) {
            status = RETURN_OK;
        } else {
            LOGGER_ERROR("SPIBUS:ERROR:: SPI write failed");
            status = RETURN_NOTOK;
        }
    }

    OcGpio_write(chip_select, PIN_HIGH);

    SPI_close(spiHandle);

    return (status);
}
