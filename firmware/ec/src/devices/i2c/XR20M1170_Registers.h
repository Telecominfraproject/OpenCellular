/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
// Register definitions for the XR20M1170

// Using bitfields aren't the most portable thing, but they make things much
// nicer for defining the registers - I'm not against changing them to macros
// if there's a strong case for it

#pragma once

#ifndef XR20M1170_REGISTERS_H_
#define XR20M1170_REGISTERS_H_

#include <stdbool.h>
#include <stdint.h>

#include "helpers/attribute.h"

typedef enum XrRegister {
    XR_REG_RHR = 0x00, // LCR[7] = 0 (read only)
    XR_REG_THR = 0x00, // LCR[7] = 0 (write only)

    XR_REG_DLL = 0x00, // LCR[7] = 1, LCR != 0xBF
    XR_REG_DLM = 0x01, // LCR[7] = 1, LCR != 0xBF
    XR_REG_DLD = 0x02, // LCR[7] = 1, LCR != 0xBF, EFR[4] = 1

    XR_REG_IER = 0x01, // LCR[7] = 0

    XR_REG_ISR = 0x02, // LCR[7] = 0 (read only)
    XR_REG_FCR = 0X02, // LCR[7] = 0 (write only)

    XR_REG_LCR = 0x03,

    XR_REG_MCR = 0x04, // LCR != 0xBF
    XR_REG_LSR = 0x05, // LCR != 0xBF

    XR_REG_MSR = 0x06, /* EFR[0] == 0 || MCR[2] == 0 */
    XR_REG_SPR = 0x07, /* EFR[0] == 0 || MCR[2] == 0 */
    XR_REG_TCR = 0x06, /* EFR[4] == 1 && MCR[2] == 1 */
    XR_REG_TLR = 0x07, /* EFR[4] == 1 && MCR[2] == 1 */

    XR_REG_TXLVL = 0x08, // LCR[7] = 0
    XR_REG_RXLVL = 0x09, // LCR[7] = 0
    XR_REG_IODIR = 0x0A, // LCR[7] = 0
    XR_REG_IOSTATE = 0x0B, // LCR[7] = 0
    XR_REG_IOINTENA = 0x0C, // LCR[7] = 0
    XR_REG_IOCTRL = 0x0E, // LCR[7] = 0
    XR_REG_EFCR = 0x0F, // LCR[7] = 0

    XR_REG_EFR = 0x02, // LCR = 0xBF
    XR_REG_XON1 = 0x04, // LCR = 0xBF
    XR_REG_XON2 = 0x05, // LCR = 0xBF
    XR_REG_XOFF1 = 0x06, // LCR = 0xBF
    XR_REG_XOFF2 = 0x07, // LCR = 0xBF
} XrRegister;

// TODO: this isn't used right now
typedef enum XrSamplingMode {
    XR_SAMPLING_MODE_4X = 0,
    XR_SAMPLING_MODE_8X,
    XR_SAMPLING_MODE_16X,

    XR_SAMPLING_MODE_COUNT,
} XRSamplingMode;

typedef enum XrChannel {
    XR_CHANNEL_A = 0x00,

    XR_CHANNEL_COUNT,
} XrChannel;

typedef struct PACKED XrSubAddress {
    union PACKED {
        struct PACKED {
            uint8_t reserve1 : 1;
            XrChannel channel : 2;
            uint8_t reg : 4;
            uint8_t reserve2 : 1;
        };
        uint8_t byte;
    };
} XrSubAddress;

typedef uint8_t XrRegTxlvl;
typedef uint8_t XrRegRxlvl;

typedef enum XrWordLen {
    XR_WORD_LEN_5 = 0x0, //!< Data length is 5 bits
    XR_WORD_LEN_6 = 0x1, //!< Data length is 6 bits
    XR_WORD_LEN_7 = 0x2, //!< Data length is 7 bits
    XR_WORD_LEN_8 = 0x3, //!< Data length is 8 bits
} XrWordLen;

typedef enum XrStopBit {
    XR_STOP_BIT_ONE = 0x0, //!< One stop bit
    XR_STOP_BIT_TWO = 0x1, //!< Two stop bits
} XrStopBit;

typedef enum XrParity {
    XR_PARITY_NONE = 0x0, //!< No parity
    XR_PARITY_ODD = 0x1, //!< Parity bit is odd
    XR_PARITY_EVEN = 0x3, //!< Parity bit is even
    XR_PARITY_ONE = 0x5, //!< Parity bit is always one
    XR_PARITY_ZERO = 0x7, //!< Parity bit is always zero
} XrParity;

// TODO: a lot of these should be enums
typedef struct PACKED XrRegLcr {
    XrWordLen wordLen : 2; // Word length to be transmitted or received
    XrStopBit stopBits : 1; // Length of stop bit
    XrParity parity : 3; // Parity format
    bool txBreak : 1; // Causes a break condition to be transmitted
    bool divisorEn : 1; // Baud rate generator divisor (DLL, DLM and DLD)
} XrRegLcr;

typedef struct PACKED XrRegDld {
    uint8_t fracDivisor : 4;
    bool mode8x : 1;
    bool mode4x : 1;
    uint8_t reserved : 2;
} XrRegDld;

typedef struct PACKED XrRegEfr {
    uint8_t swFlowCtl : 4;
    bool enhancedFunc : 1;
    bool specialCharDetect : 1;
    bool autoRts : 1;
    bool autoCts : 1;
} XrRegEfr;

typedef uint8_t XrRegDlm;
typedef uint8_t XrRegDll;

typedef enum XrClkPrescaler {
    XR_CLK_PRESCALER_1x = 0x0,
    XR_CLK_PRESCALER_4x = 0x1,
} XrClkPrescaler;

typedef struct PACKED XrRegMcr {
    bool dtr : 1;
    bool rts : 1;
    bool op1 : 1;
    bool op2 : 1;
    bool loopbackEn : 1;
    bool xonAnyEn : 1;
    bool irModeEn : 1;
    XrClkPrescaler clkPrescaler : 1;
} XrRegMcr;

typedef enum RxTriggerLevel {
    RX_TRIGGER_LEVEL_8 = 0x00,
    RX_TRIGGER_LEVEL_16 = 0x01,
    RX_TRIGGER_LEVEL_56 = 0x02,
    RX_TRIGGER_LEVEL_60 = 0x03,
} RxTriggerLevel;

typedef enum TxTriggerLevel {
    TX_TRIGGER_LEVEL_8 = 0x00,
    TX_TRIGGER_LEVEL_16 = 0x01,
    TX_TRIGGER_LEVEL_32 = 0x02,
    TX_TRIGGER_LEVEL_56 = 0x03,
} TxTriggerLevel;

typedef struct PACKED XrRegFcr {
    bool fifoEn : 1;
    bool rxRst : 1;
    bool txRst : 1;
    bool reserved : 1;
    TxTriggerLevel txTrigger : 2; /*!< Overwridden if TLR value set */
    RxTriggerLevel rxTrigger : 2; /*!< Overwridden if TLR value set */
} XrRegFcr;

/* Transmission Control Register (TCR) */
typedef struct PACKED XrRegTcr {
    uint8_t rxHaltLvl : 4; /*!< x4, 0-60 - RTS goes high after this level */
    uint8_t rxResumeLvl : 4; /*!< x4, 0-60 - RTS returns low below this level */
} XrRegTcr;

/* Trigger Level Register (TLR) */
typedef struct PACKED XrRegTlr {
    uint8_t txTrigger : 4; /*!< x4, 4-60, If 0 (default), FCR value used */
    uint8_t rxTrigger : 4; /*!< x4, 4-60, If 0 (default), FCR value used */
} XrRegTlr;

typedef struct PACKED XrRegIOCtrl {
    bool ioLatch : 1;
    bool modemIf : 1;
    uint8_t res1 : 1;
    bool uartReset : 1;
    uint8_t res2 : 4;
} XrRegIOCtrl;

// LCR[7] = 0
typedef struct PACKED XrRegIer {
    bool rhrIntEn : 1;
    bool thrIntEn : 1;
    bool lsrIntEn : 1;
    bool msrIntEn : 1;
    bool sleepModeEn : 1; //!< EFR[4] = 1 to modify
    bool xoffIntEn : 1; //!< EFR[4] = 1 to modify
    bool rtsIntEn : 1; //!< EFR[4] = 1 to modify
    bool ctsIntEn : 1; //!< EFR[4] = 1 to modify
} XrRegIer;

// Note: in order of priority
typedef enum ISR_SRC {
    ISR_SRC_LSR = 0x06,
    ISR_SRC_RXRDY_TMOUT = 0x0C, // TODO: upper case hex?
    ISR_SRC_RXRDY = 0x04,
    ISR_SRC_TXRDY = 0x02,
    ISR_SRC_MSR = 0x00,
    ISR_SRC_GPIO = 0x30,
    ISR_SRC_RXRDY_XOFF = 0x10,
    ISR_SRC_CTS_RTS = 0x20,

    ISR_SRC_NONE = 0x01,
} ISR_SRC;

typedef struct PACKED XrRegIsr {
    ISR_SRC source : 6;
    uint8_t fifo_en : 2; // TODO: not sure why there's 2...datasheet doesn't elaborate - possibly TX vs RX, but it doesn't say
} XrRegIsr;

// General struct for configuring gpio pins
typedef struct PACKED XrGpioPins {
    union {
        struct PACKED {
            bool p0 : 1;
            bool p2 : 1;
            bool p3 : 1;

            bool p4 : 1;
            bool p5 : 1;
            bool p6 : 1;
            bool p7 : 1;
        } gpio;

        struct PACKED {
            char res : 4;

            bool dsr : 1;
            bool dtr : 1;
            bool cd : 1;
            bool ri : 1;
        } modem;
    };
} XrGpioPins;

#endif
