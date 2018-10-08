/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

// Driver for the XR20M1170 i2c to UART converter
#pragma once

#ifndef XR20M1170_H_
#define XR20M1170_H_

#include "drivers/OcGpio.h"

#include <ti/drivers/I2C.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/utils/RingBuf.h>
#include <ti/sysbios/gates/GateMutex.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <stdbool.h>

typedef enum XR20M1170_FlowControl {
    XR20M1170_FLOWCONTROL_TX = 0x01, // Enable auto CTS
    XR20M1170_FLOWCONTROL_RX = 0x02, // Enable auto RTS
    XR20M1170_FLOWCONTROL_NONE = 0x00,
} XR20M1170_FlowControl;

/* UART function table pointer */
extern const UART_FxnTable XR20M1170_fxnTable;

// TODO: reorganize struct for efficiency
typedef struct XR20M1170_HWAttrs {
    unsigned int i2cIndex;
    unsigned char i2cSlaveAddress;

    unsigned int xtal1_freq;
    OcGpio_Pin *pin_irq; /*!< XR20M IRQ# pin */
    /*! UART Peripheral's interrupt priority */
    // TODO: might be able to do something with this
    unsigned int intPriority;
    /*! Hardware flow control setting defined by driverlib */
    XR20M1170_FlowControl flowControl;

    /*! Pointer to a application ring buffer */
    unsigned char *ringBufPtr;
    /*! Size of ringBufPtr */
    size_t ringBufSize;
} XR20M1170_HWAttrs;

/*!
 *  @brief      XR20M1170 Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct XR20M1170_Object {
    /* UART state variable */
    struct {
        bool opened : 1; /* Has the obj been opened */
        UART_Mode readMode : 1; /* Mode for all read calls */
        UART_Mode writeMode : 1; /* Mode for all write calls */
        UART_DataMode readDataMode : 1; /* Type of data being read */
        UART_DataMode writeDataMode : 1; /* Type of data being written */
        UART_ReturnMode readReturnMode : 1; /* Receive return mode */
        UART_Echo readEcho : 1; /* Echo received data back */
        /*
         * Flag to determine if a timeout has occurred when the user called
         * UART_read(). This flag is set by the timeoutClk clock object.
         */
        bool bufTimeout : 1;
        /*
         * Flag to determine when an ISR needs to perform a callback; in both
         * UART_MODE_BLOCKING or UART_MODE_CALLBACK
         */
        bool callCallback : 1;
        /*
         * Flag to determine if the ISR is in control draining the ring buffer
         * when in UART_MODE_CALLBACK
         */
        bool drainByISR : 1;
        /* Flag to keep the state of the read ring buffer */
        bool rxEnabled : 1;
    } state;

    I2C_Handle i2cHandle;

    //  TODO: these are from UART_Tiva - need to revise the struct members
    //    Clock_Struct         timeoutClk;       /* Clock object to for timeouts */
    //    uint32_t             baudRate;         /* Baud rate for UART */
    //    UART_LEN             dataLength;       /* Data length for UART */
    //    UART_STOP            stopBits;         /* Stop bits for UART */
    //    UART_PAR             parityType;       /* Parity bit type for UART */
    //
    //    /* UART read variables */
    RingBuf_Object ringBuffer; /* local circular buffer object */
    GateMutex_Handle ringBufMutex; // Mutex for accessing ring buffer
    //    /* A complement pair of read functions for both the ISR and UART_read() */
    //    UARTTiva_FxnSet      readFxns;
    //    unsigned char       *readBuf;          /* Buffer data pointer */
    //    size_t               readSize;         /* Desired number of bytes to read */
    //    size_t               readCount;        /* Number of bytes left to read */

    Semaphore_Handle readSem; /* UART read semaphore */
    unsigned int readTimeout; /* Timeout for read semaphore */
    //    UART_Callback        readCallback;     /* Pointer to read callback */
    //
    //    /* UART write variables */
    //    const unsigned char *writeBuf;         /* Buffer data pointer */
    //    size_t               writeSize;        /* Desired number of bytes to write*/
    //    size_t               writeCount;       /* Number of bytes left to write */
    Semaphore_Handle writeSem; /* UART write semaphore*/
    unsigned int writeTimeout; /* Timeout for write semaphore */
    //    UART_Callback        writeCallback;    /* Pointer to write callback */
    //
    //    ti_sysbios_family_arm_m3_Hwi_Struct hwi; /* Hwi object */
} XR20M1170_Object, *XR20M1170_Handle;

#endif
