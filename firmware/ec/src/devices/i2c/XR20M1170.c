/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "XR20M1170.h"
#include "XR20M1170_Registers.h"

#include "helpers/math.h"
#include "helpers/i2c.h"
#include "helpers/memory.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"
#include "threaded_int.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Log.h>

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define FIFO_SIZE 64
#define TX_FIFO_SIZE FIFO_SIZE
#define RX_FIFO_SIZE FIFO_SIZE

// TODO: figure out task priority for things, maybe part of hwAttrs?
// for some reason it crashes if priority is > 6

// Mapping from TI UART settings to Exar register settings
static const XrWordLen XR_WORD_LEN_MAP[] = {
    [UART_LEN_5] = XR_WORD_LEN_5,
    [UART_LEN_6] = XR_WORD_LEN_6,
    [UART_LEN_7] = XR_WORD_LEN_7,
    [UART_LEN_8] = XR_WORD_LEN_8,
};

static const XrStopBit XR_STOP_BIT_MAP[] = {
    [UART_STOP_ONE] = XR_STOP_BIT_ONE,
    [UART_STOP_TWO] = XR_STOP_BIT_TWO,
};

static const XrParity XR_PARITY_MAP[] = {
    [UART_PAR_NONE] = XR_PARITY_NONE, [UART_PAR_EVEN] = XR_PARITY_EVEN,
    [UART_PAR_ODD] = XR_PARITY_ODD,   [UART_PAR_ZERO] = XR_PARITY_ZERO,
    [UART_PAR_ONE] = XR_PARITY_ONE,
};

// TXLVL IRQ will automatically fire after reset & give us the actual level
static XrRegTxlvl s_txEmptyBytes = 0;

// UART function table for XR20M1170 implementation
void XR20M1170_close(UART_Handle handle);
int XR20M1170_control(UART_Handle handle, unsigned int cmd, void *arg);
void XR20M1170_init(UART_Handle handle);
UART_Handle XR20M1170_open(UART_Handle handle, UART_Params *params);
int XR20M1170_read(UART_Handle handle, void *buffer, size_t size);
int XR20M1170_readPolling(UART_Handle handle, void *buffer, size_t size);
void XR20M1170_readCancel(UART_Handle handle);
int XR20M1170_write(UART_Handle handle, const void *buffer, size_t size);
int XR20M1170_writePolling(UART_Handle handle, const void *buffer, size_t size);
void XR20M1170_writeCancel(UART_Handle handle);

const UART_FxnTable XR20M1170_fxnTable = {
    XR20M1170_close,      XR20M1170_control, XR20M1170_init,
    XR20M1170_open,       XR20M1170_read,    XR20M1170_readPolling,
    XR20M1170_readCancel, XR20M1170_write,   XR20M1170_writePolling,
    XR20M1170_writeCancel
};

static XrSubAddress getSubAddress(XrRegister reg)
{
    return (XrSubAddress){
        .channel = XR_CHANNEL_A, // The only supported option
        .reg = reg,
    };
}

static bool writeData(UART_Handle handle, XrRegister reg, const void *buffer,
                      size_t size)
{
    XR20M1170_Object *object = handle->object;
    const XR20M1170_HWAttrs *hwAttrs = handle->hwAttrs;

    if (!I2C_write(object->i2cHandle, getSubAddress(reg).byte,
                   hwAttrs->i2cSlaveAddress, buffer, size)) {
        DEBUG("XR: I2C Failure\n");
        return false;
    }
    return true;
}

static void readData(UART_Handle handle, XrRegister reg, void *buffer,
                     size_t size)
{
    XR20M1170_Object *object = handle->object;
    const XR20M1170_HWAttrs *hwAttrs = handle->hwAttrs;

    if (!I2C_read(object->i2cHandle, getSubAddress(reg).byte,
                  hwAttrs->i2cSlaveAddress, buffer, size)) {
        DEBUG("XR: I2C Read Failure\n");
    }
}

void XR20M1170_close(UART_Handle handle)
{
}

int XR20M1170_control(UART_Handle handle, unsigned int cmd, void *arg)
{
    XR20M1170_Object *object = handle->object;

    // Most of the commands require this info, might as well dedupe
    int rxLvl;
    uint8_t peekByte;
    IArg mutexKey = GateMutex_enter(object->ringBufMutex);
    {
        rxLvl = RingBuf_peek(&object->ringBuffer, &peekByte);
    }
    GateMutex_leave(object->ringBufMutex, mutexKey);

    switch (cmd) {
        case UART_CMD_ISAVAILABLE: {
            bool *available = arg;
            *available = (rxLvl > 0);
            return (UART_STATUS_SUCCESS);
        }
        case UART_CMD_GETRXCOUNT: {
            int *count = arg;
            *count = rxLvl;
            return (UART_STATUS_SUCCESS);
        }
        case UART_CMD_PEEK: {
            int *byte = (int *)arg;
            if (!rxLvl) {
                *byte = UART_ERROR;
                return UART_STATUS_ERROR;
            }
            // NOTE: make sure we treat arg as int, otherwise the upper bits
            // won't get cleared
            *byte = peekByte;
            return UART_STATUS_SUCCESS;
        }
        case UART_CMD_RXENABLE:
        case UART_CMD_RXDISABLE:
        default:
            return (UART_STATUS_UNDEFINEDCMD);
    }
}

// XR20M1170_init
void XR20M1170_init(UART_Handle handle)
{
    XR20M1170_Object *object = handle->object;

    *object = (XR20M1170_Object){
        .state.opened = false,
    };
}

static void processIrq(void *context)
{
    UART_Handle handle = context;
    XR20M1170_Object *object = handle->object;

    // Handle interrupt
    while (true) {
        XrRegIsr isr;
        readData(handle, XR_REG_ISR, &isr, sizeof(isr));

        switch (isr.source) {
            case ISR_SRC_RXRDY_TMOUT:
            case ISR_SRC_RXRDY: {
                // See how much data is available
                XrRegRxlvl rxLvl;
                readData(handle, XR_REG_RXLVL, &rxLvl, sizeof(rxLvl));

                int ringBufSpace;
                IArg mutexKey = GateMutex_enter(object->ringBufMutex);
                {
                    ringBufSpace = object->ringBuffer.length -
                                   RingBuf_getCount(&object->ringBuffer);
                }
                GateMutex_leave(object->ringBufMutex, mutexKey);

                // Read in all available data to our own buffer
                // This will be at most 64 bytes, so read into a contiguous
                // buffer for efficiency (one i2c transaction & out of mutex)

                // TODO: investigate consequences of not reading in all data
                // - if there's always a consumer of the data it's fine, but
                // if something is pushing data and nothing is listening, we'll
                // enter a nasty loop
                const int bytesToRead = MIN(rxLvl, ringBufSpace);
                unsigned char buf[bytesToRead];
                readData(handle, XR_REG_RHR, buf, sizeof(buf));

                mutexKey = GateMutex_enter(object->ringBufMutex);
                {
                    // TODO: I don't like having to copy one byte at a time,
                    // I can make a better ring buffer - one that supports bulk
                    // copy and maybe auto-locking
                    for (int i = 0; i < bytesToRead; ++i) {
                        RingBuf_put(&object->ringBuffer, buf[i]);
                        Semaphore_post(
                                object->readSem); // TODO: move out of mutex lock?
                    }
                }
                GateMutex_leave(object->ringBufMutex, mutexKey);
                break;
            }
            case ISR_SRC_TXRDY:
                readData(handle, XR_REG_TXLVL, &s_txEmptyBytes,
                         sizeof(s_txEmptyBytes));
                Semaphore_post(object->writeSem);
                break;
            case ISR_SRC_LSR:
            case ISR_SRC_MSR:
            case ISR_SRC_GPIO:
            case ISR_SRC_RXRDY_XOFF:
            case ISR_SRC_CTS_RTS:
                DEBUG("INT %d not yet handled\n", isr.source);
                break;
            case ISR_SRC_NONE:
                return;
        }
    }
}

static double calc_divisor(double xtal_freq, double prescaler, double baudRate,
                           double sampling_mode)
{
    return (xtal_freq / prescaler) / (baudRate * sampling_mode);
}

static bool reset_ic(UART_Handle handle)
{
    XrRegIOCtrl ioCtrl = {
        .uartReset = true,
    };
    if (!writeData(handle, XR_REG_IOCTRL, &ioCtrl, sizeof(ioCtrl))) {
        return false;
    }
    Task_sleep(20);
    return true;
}

// Sets up the XR20M1170 registers to match desired settings
static bool register_config(UART_Handle handle, UART_Params *params)
{
    const XR20M1170_HWAttrs *hwAttrs = handle->hwAttrs;

    // TODO: Idea: have a function for setting registers to make sure everything
    // is in the correct state?
    // - should probably at least do function wrappers to have safer typing

    /* TODO: handle i2c failures better */

    // Enable modifications to enhanced function registers and enable HW flow control (might as well)
    uint8_t bf = 0xBF; // TODO: hack - required to modify this register
    if (!writeData(handle, XR_REG_LCR, &bf, 1)) {
        return false;
    }

    XrRegEfr efr = {
        .enhancedFunc = true, // It's fine to keep this enabled
        .autoCts = hwAttrs->flowControl & XR20M1170_FLOWCONTROL_TX,
        .autoRts = hwAttrs->flowControl & XR20M1170_FLOWCONTROL_RX,
    };
    writeData(handle, XR_REG_EFR, &efr, sizeof(efr));

    // Switch to divisor config mode (LCR[7] = 1)
    XrRegLcr lcr = {
        .divisorEn = true,
    };
    writeData(handle, XR_REG_LCR, &lcr, sizeof(lcr));

    // Calculate divisor register values
    // TODO: how do I figure out what the sampling mode should be?
    int prescaler = 1;
    int samplingMode = 16;
    double divisor = calc_divisor(hwAttrs->xtal1_freq, prescaler,
                                  params->baudRate, samplingMode);

    // If the divisor is too big, introduce the prescaler
    if (divisor > UINT16_MAX) {
        prescaler = 4;
        divisor = calc_divisor(hwAttrs->xtal1_freq, prescaler, params->baudRate,
                               samplingMode);
    }

    // Split the divisor into its integer and fractional parts
    double intPart;
    double fracPart;
    fracPart = modf(divisor, &intPart);

    XrRegDlm dlm = HIBYTE((uint16_t)intPart);
    XrRegDll dll = LOBYTE((uint16_t)intPart);
    XrRegDld dld = {
        .fracDivisor = round(fracPart * 16.0),
        .mode8x = samplingMode == 8 ? true : false,
        .mode4x = samplingMode == 4 ? true : false,
    };
    writeData(handle, XR_REG_DLM, &dlm, sizeof(dlm));
    writeData(handle, XR_REG_DLL, &dll, sizeof(dll));
    writeData(handle, XR_REG_DLD, &dld, sizeof(dld));

    // Calculate data error rate
    double realDivisor = (dld.fracDivisor / 16.0) + trunc(divisor);
    double dataErrorRate = ((divisor - realDivisor) / divisor) * 100.0;
    if (dataErrorRate > 0.001) {
        Log_warning2("XR20M1170: Data error rate of %d%% for baud rate %d",
                     dataErrorRate, params->baudRate);
    }

    // Set up LCR
    lcr = (XrRegLcr){
        .wordLen = XR_WORD_LEN_MAP[params->dataLength],
        .stopBits = XR_STOP_BIT_MAP[params->stopBits],
        .parity = XR_PARITY_MAP[params->parityType],
        .txBreak = 0,
        .divisorEn = 0,
    };
    writeData(handle, XR_REG_LCR, &lcr, sizeof(lcr));

    // TODO: can do something with loopback for testing
    XrRegMcr mcr = {
        .dtr = true, // Assert DTR to enable Iridium serial interface
        .rts = true, // Required for auto flow control
        .op1 = true, /* Select access to TCR from MSR */
        .loopbackEn = false,
        .clkPrescaler =
                ((prescaler > 1) ? XR_CLK_PRESCALER_4x : XR_CLK_PRESCALER_1x),
    };
    writeData(handle, XR_REG_MCR, &mcr, sizeof(mcr));

    // Enable FIFOs
    XrRegFcr fcr = {
        .fifoEn = true,
    };
    writeData(handle, XR_REG_FCR, &fcr, sizeof(fcr));

    /* Set trigger levels - these override the levels set by FCR if nonzero */
    XrRegTlr tlr = {
        .rxTrigger = 32 / 4, /* 4-60, multiple of 4 */
        .txTrigger = 32 / 4, /* 4-60, multiple of 4 */
    };
    writeData(handle, XR_REG_TLR, &tlr, sizeof(tlr));

    /* Set halt/resume levels - these can be relatively low since data should
     * normally be cleared quite quickly */
    XrRegTcr tcr = {
        .rxHaltLvl = 40 / 4, /* 0-60, multiple of 4 */
        .rxResumeLvl = 12 / 4, /* 0-60, multiple of 4 */
    };
    writeData(handle, XR_REG_TCR, &tcr, sizeof(tcr));

    // Configure interrupts
    XrRegIer ier = {
        .rhrIntEn = true,
        .thrIntEn = true,
    };
    writeData(handle, XR_REG_IER, &ier, sizeof(ier));

    // Enable modem interface for GPIO pins
    XrRegIOCtrl ioCtrl = {
        .modemIf = true,
    };
    writeData(handle, XR_REG_IOCTRL, &ioCtrl, sizeof(ioCtrl));

    // Enable DTR as an output pin
    XrGpioPins ioDir = {
        .modem.dtr = true,
    };
    writeData(handle, XR_REG_IODIR, &ioDir, sizeof(ioDir));

    return true;
}

// XR20M1170_open
UART_Handle XR20M1170_open(UART_Handle handle, UART_Params *params)
{
    XR20M1170_Object *object = handle->object;
    const XR20M1170_HWAttrs *hwAttrs = handle->hwAttrs;

    if (object->state.opened == true) {
        //Log_warning1("UART:(%p) already in use.", hwAttrs->baseAddr);
        return NULL;
    }

    object->i2cHandle = i2c_open_bus(hwAttrs->i2cIndex);
    if (!object->i2cHandle) {
        return NULL;
    }

    /* Make sure the chip is reset */
    if (!reset_ic(handle)) {
        return false;
    }

    // Read/Write semaphores
    Semaphore_Params semParams;

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    object->writeSem = Semaphore_create(0, &semParams, NULL);
    if (!object->writeSem) {
        DEBUG("XR20M1170:ERROR::Can't create write semaphore\n");
    }

    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_COUNTING;
    object->readSem = Semaphore_create(0, &semParams, NULL);
    if (!object->readSem) {
        DEBUG("XR20M1170:ERROR::Can't create read semaphore\n");
    }

    // Initialize our ring buffer
    if (!hwAttrs->ringBufPtr || !hwAttrs->ringBufSize) {
        DEBUG("XR20M1170::FATAL - no ring buf\n");
    }
    RingBuf_construct(&object->ringBuffer, hwAttrs->ringBufPtr,
                      hwAttrs->ringBufSize);
    object->ringBufMutex = GateMutex_create(NULL, NULL);
    if (!object->ringBufMutex) {
        DEBUG("XR20M1170:ERROR::Can't ring buffer mutex\n");
    }

    OcGpio_configure(hwAttrs->pin_irq,
                     OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING);

    // Set up our threaded interrupt handler
    ThreadedInt_Init(hwAttrs->pin_irq, processIrq, handle);

    /* Configure XR20M1170 chip */
    if (!register_config(handle, params)) {
        return NULL;
    }

    // TODO: for this to fully implement TI interface, we should deal with these
    //    object->state.readMode       = params->readMode;
    //    object->state.writeMode      = params->writeMode;
    //    object->state.readReturnMode = params->readReturnMode;
    //    object->state.readDataMode   = params->readDataMode;
    //    object->state.writeDataMode  = params->writeDataMode;
    //    object->state.readEcho       = params->readEcho;
    object->readTimeout = params->readTimeout;
    object->writeTimeout = params->writeTimeout;
    //    object->readCallback         = params->readCallback;
    //    object->writeCallback        = params->writeCallback;

    object->state.opened = true;

    return (handle);
}

int XR20M1170_read(UART_Handle handle, void *buffer, size_t size)
{
    XR20M1170_Object *object = handle->object;
    uint8_t *char_buf = buffer;

    int bytesRead = 0;
    while (bytesRead < size) {
        /* Wait for there to be data available in the buffer */
        if (!Semaphore_pend(object->readSem, object->readTimeout)) {
            break;
        }

        /* Read in all the data we can */
        IArg mutexKey = GateMutex_enter(object->ringBufMutex);
        {
            do {
                RingBuf_get(&object->ringBuffer, char_buf);
                ++char_buf;
                ++bytesRead;
            } while ((bytesRead < size) &&
                     Semaphore_pend(object->readSem, BIOS_NO_WAIT));
        }
        GateMutex_leave(object->ringBufMutex, mutexKey);
    }
    return bytesRead;
}

int XR20M1170_write(UART_Handle handle, const void *buffer, size_t size)
{
    XR20M1170_Object *object = handle->object;
    int bytes_written = 0;

    while (size) {
        if (!Semaphore_pend(object->writeSem, object->writeTimeout)) {
            return bytes_written;
        }

        int bytesToWrite = MIN(size, s_txEmptyBytes);
        writeData(handle, XR_REG_THR, buffer, bytesToWrite);
        size -= bytesToWrite;
        bytes_written += bytesToWrite;
        buffer = (const uint8_t *)buffer + bytesToWrite;
    }
    return bytes_written;
}

int XR20M1170_readPolling(UART_Handle handle, void *buffer, size_t size)
{
    DEBUG("XR20M1170::readPolling not yet implemented");
    return 0;
}

void XR20M1170_readCancel(UART_Handle handle)
{
    DEBUG("XR20M1170::readCancel not yet implemented");
}

int XR20M1170_writePolling(UART_Handle handle, const void *buffer, size_t size)
{
    DEBUG("XR20M1170::writePolling not yet implemented");
    return 0;
}

void XR20M1170_writeCancel(UART_Handle handle)
{
    DEBUG("XR20M1170::writeCancel not yet implemented");
}
