/*
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== AT45DB.c ========
 */
#include "Board.h"
#include "inc/devices/AT45DB.h"

#include <stdint.h>
#include <stdbool.h>

/* TI-RTOS Kernel Header files */
#include <xdc/std.h>
#include <xdc/runtime/Assert.h>
#include <xdc/runtime/Diags.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/Memory.h>
#include <ti/sysbios/gates/GateMutex.h>

/* TI-RTOS Driver Header files */
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
//#include "src/filesystem/AT45DBFatFs.h"

#define PIN_LOW (0)
#define PIN_HIGH ~(0)

/* Default AT45DB parameters structure */
const AT45DB_Params AT45DB_defaultParams = {
    0, /* dummy */
};

unsigned char AT45DB_readStatusRegister(AT45DB_Handle handle);

/*
 *  ======== waitForReady ========
 */
#define waitForReady(handle)                                    \
    while (!(AT45DB_READY & AT45DB_readStatusRegister(handle))) \
        ;

/*
 *  ======== AT45DB_construct ========
 */
AT45DB_Handle AT45DB_construct(AT45DB_Object *obj, SPI_Handle spiHandle,
                               unsigned int gpioCSIndex, AT45DB_Params *params)
{
    /* Initialize the object's fields */
    obj->spiHandle = spiHandle;
    obj->gpioCS = gpioCSIndex;
    GateMutex_construct(&(obj->gate), NULL);

    Log_print1(Diags_USER1, "AT45DB constructed", NULL);
    return ((AT45DB_Handle)obj);
}

/*
 *  ======== AT45DB_create ========
 */
AT45DB_Handle AT45DB_create(SPI_Handle spiHandle, unsigned int gpioCSIndex,
                            AT45DB_Params *params)
{
    AT45DB_Handle handle;
    Error_Block eb;

    Error_init(&eb);
    handle = Memory_alloc(NULL, sizeof(AT45DB_Object), NULL, &eb);
    if (handle == NULL) {
        return (NULL);
    }
    handle = AT45DB_construct(handle, spiHandle, gpioCSIndex, params);
    return (handle);
}

/*
 *  ======== AT45DB_delete ========
 */
Void AT45DB_delete(AT45DB_Handle handle)
{
    AT45DB_destruct(handle);

    Memory_free(NULL, handle, sizeof(AT45DB_Object));
}

/*
 *  ======== AT45DB_destruct ========
 */
Void AT45DB_destruct(AT45DB_Handle handle)
{
    Assert_isTrue((handle != NULL), NULL);

    GateMutex_destruct(&(handle->gate));
}

/*
 *  ======== AT45DB_erasePage ========
 */
bool AT45DB_erasePage(AT45DB_Handle handle, uint32_t page)
{
    SPI_Transaction spiTransaction;
    unsigned char txBuffer[4];
    IArg key;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));
    waitForReady(handle);

    txBuffer[0] = 0x81;
    txBuffer[1] =
        (unsigned char)(page >>
                        7); // Page size is 15 bits 8 in tx1 and 7 in tx2.
    txBuffer[2] = (unsigned char)(page << 1);
    txBuffer[3] = 0x00;

    /* Initialize master SPI transaction structure */
    spiTransaction.count = 4;
    spiTransaction.txBuf = txBuffer;
    spiTransaction.rxBuf = NULL;

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }
    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (true);
}

/*
 *  ======== AT45DB_Params_init ========
 */
Void AT45DB_Params_init(AT45DB_Params *params)
{
    Assert_isTrue(params != NULL, NULL);

    *params = AT45DB_defaultParams;
}

/*
 *  ======== AT45DB_read ========
 */
bool AT45DB_read(AT45DB_Handle handle, AT45DB_Transaction *transaction,
                 uint32_t page)
{
    SPI_Transaction spiTransaction;
    unsigned char
        txBuffer[8]; /* last 4 bytes are needed, but have don't care values */
    IArg key;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    waitForReady(handle);

    txBuffer[0] = 0xD2;
    txBuffer[1] =
        (unsigned char)(page >>
                        7); // Page size is 15 bits 8 in tx1 and 7 in tx2.
    txBuffer[2] =
        (unsigned char)((page << 1)); //| (0x1 & (transaction->byte >> 8)));
    txBuffer[3] = (unsigned char)(0xFF & (transaction->byte));

    /* Initialize master SPI transaction structure */
    spiTransaction.count = 8;
    spiTransaction.txBuf = txBuffer;
    spiTransaction.rxBuf = NULL;

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    spiTransaction.count = transaction->data_size;
    spiTransaction.txBuf = NULL;
    spiTransaction.rxBuf = transaction->data;

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }
    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (true);
}

/*
 *  ======== AT45DB_readBuffer ========
 */
bool AT45DB_readBuffer(AT45DB_Handle handle, AT45DB_Transaction *transaction)
{
    SPI_Transaction spiTransaction;
    unsigned char txBuffer[4];
    IArg key;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    waitForReady(handle);

    /* Using SRAM Buffer 2 */
    txBuffer[0] = 0xD3;
    txBuffer[1] = 0x00;
    txBuffer[2] = (unsigned char)(0x3 & (transaction->byte >> 8));
    txBuffer[2] = (unsigned char)(0x1 & (transaction->byte >>
                                         8)); //( 9 bit buffer address.)
    txBuffer[3] = (unsigned char)(0xFF & transaction->byte);

    /* Initialize master SPI transaction structure */
    spiTransaction.count = 4;
    spiTransaction.txBuf = txBuffer;
    spiTransaction.rxBuf = NULL;

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);
    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    spiTransaction.count = transaction->data_size;
    spiTransaction.txBuf = NULL;
    spiTransaction.rxBuf = transaction->data;

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (true);
}

/*
 *  ======== AT45DB_readStatusRegister ========
 */
unsigned char AT45DB_readStatusRegister(AT45DB_Handle handle)
{
    SPI_Transaction spiTransaction;
    unsigned char txBuffer = 0xD7;
    volatile unsigned char status;
    IArg key;

    /* Initialize master SPI transaction structure */
    spiTransaction.count = 1;
    spiTransaction.txBuf = &txBuffer;
    spiTransaction.rxBuf = NULL;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI opcode transfer", NULL);
    }

    /* Get status */
    spiTransaction.count = 1;
    spiTransaction.txBuf = NULL;
    spiTransaction.rxBuf = (Ptr)&status;

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI data transfer", NULL);
    }

    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (status);
}

/*
 *  ======== AT45DB_write ========
 */
bool AT45DB_write(AT45DB_Handle handle, AT45DB_Transaction *transaction,
                  uint32_t page)
{
    IArg key;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    /* Write to the AT45DB Buffer */
    if (!AT45DB_writeBuffer(handle, transaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    /* Push the AT45DB Buffer to the flash page */
    if (!AT45DB_writeBufferToPage(handle, page)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (true);
}

/*
 *  ======== AT45DB_writeBuffer ========
 */
bool AT45DB_writeBuffer(AT45DB_Handle handle, AT45DB_Transaction *transaction)
{
    SPI_Transaction spiTransaction;
    unsigned char txBuffer[4];
    IArg key;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    waitForReady(handle);

    /* Using SRAM Buffer 2 */
    txBuffer[0] = 0x87; // write to buffer 2
    txBuffer[1] = 0x00;
    txBuffer[2] = (unsigned char)(0x1 & (transaction->byte >>
                                         8)); //( 9 bit buffer address.
    txBuffer[3] = (unsigned char)(0xFF & transaction->byte);

    /* Initialize master SPI transaction structure */
    spiTransaction.count = 4;
    spiTransaction.txBuf = txBuffer;
    spiTransaction.rxBuf = NULL;

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    spiTransaction.count = transaction->data_size;
    spiTransaction.txBuf = transaction->data;
    spiTransaction.rxBuf = NULL;

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (true);
}

bool AT45DB_pageSelection(AT45DB_Handle handle, AT45DB_Transaction *transaction)
{
    SPI_Transaction spiTransaction;
    unsigned char txBuffer[4];
    IArg key;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    // waitForReady(handle);

    txBuffer[0] = 0x3D;
    txBuffer[1] = 0x2A;
    txBuffer[2] = 0x80;
    txBuffer[3] = 0xA7;

    /*txBuffer[0] = 0xF0;
    txBuffer[1] = 0x00;
    txBuffer[2] = 0x00;
    txBuffer[3] = 0x00;*/
    // F0h, 00h, 00h, and 00h
    /* Initialize master SPI transaction structure */
    spiTransaction.count = 4;
    spiTransaction.txBuf = txBuffer;
    spiTransaction.rxBuf = NULL;

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }

    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (true);
}

/*
 *  ======== AT45DB_writeBufferToPage ========
 */
bool AT45DB_writeBufferToPage(AT45DB_Handle handle, uint32_t page)
{
    SPI_Transaction spiTransaction;
    unsigned char txBuffer[4];
    IArg key;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    waitForReady(handle);

    /* Using SRAM Buffer 2 w/ built in erase of page before write */
    txBuffer[0] = 0x86; // write to main memory for buffer
    txBuffer[1] =
        (unsigned char)(page >>
                        7); // Page size is 15 bits 8 in tx1 and 7 in tx2.
    txBuffer[2] = (unsigned char)(page << 1);
    txBuffer[3] = 0x00;

    /* Initialize master SPI transaction structure */
    spiTransaction.count = 4;
    spiTransaction.txBuf = txBuffer;
    spiTransaction.rxBuf = NULL;

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
        return (false);
    }
    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return (true);
}

/*
 *  ======== AT45DB_readStatusRegister ========
 */
bool AT45DB_readDeviceRegister(AT45DB_Handle handle, unsigned char *deviceInfo)
{
    bool ret = true;
    SPI_Transaction spiTransaction;
    unsigned char txBuffer = 0x9F;
    // volatile unsigned char status[5];
    IArg key;

    /* Initialize master SPI transaction structure */
    spiTransaction.count = 1;
    spiTransaction.txBuf = &txBuffer;
    spiTransaction.rxBuf = NULL;

    key = GateMutex_enter(GateMutex_handle(&(handle->gate)));

    /* Initiate SPI transfer */
    GPIO_write(handle->gpioCS, PIN_LOW);

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI opcode transfer", NULL);
        return false;
    }

    /* Get status */
    spiTransaction.count = 5;
    spiTransaction.txBuf = NULL;
    spiTransaction.rxBuf = deviceInfo;

    if (!SPI_transfer(handle->spiHandle, &spiTransaction)) {
        Log_print1(Diags_USER1, "Unsuccessful SPI data transfer", NULL);
        ret = false;
    }

    GPIO_write(handle->gpioCS, PIN_HIGH);

    GateMutex_leave(GateMutex_handle(&(handle->gate)), key);

    return ret;
    ;
}

bool flash_info(AT45DB_Handle handle, unsigned char *deviceInfo)
{
    bool ret = true;
    if (handle) {
        /* Write data */
        if (!AT45DB_readDeviceRegister(handle, deviceInfo)) {
            /* Print contents of slave receive buffer */
            Log_print1(Diags_USER1, "Unsuccessful SPI transfer", NULL);
            ret = false;
        }
    }
    return ret;
}
