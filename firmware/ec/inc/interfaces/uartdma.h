/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef UARTDMA_H_
#define UARTDMA_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/utils/util.h"

/*****************************************************************************
 *                             MACROS DEFINITION
 *****************************************************************************/
#define OCUARTDMA_TASK_PRIORITY 7
#define OCUARTDMA_TASK_STACK_SIZE 1024

#define OCUARTDMATX_TASK_PRIORITY 7
#define OCUARTDMATX_TASK_STACK_SIZE 1024

#define UART_TXBUF_SIZE OCMP_FRAME_TOTAL_LENGTH
#define UART_RXBUF_SIZE OCMP_FRAME_TOTAL_LENGTH

/*****************************************************************************
 *                             HANDLE DECLARATIONS
 *****************************************************************************/
extern Semaphore_Handle semUARTTX;
extern Queue_Handle uartTxMsgQueue;

/*****************************************************************************
 *                            FUNCTION DECLARATIONS
 *****************************************************************************/
void uDMAIntHandler(void);
void uDMAErrorHandler(void);
void UART3IntHandler(void);
void resetUARTDMA(void);
void ConfigureUART(void);
void InitUART3Transfer(void);
void dataTransfertoProc(char *buffer, int size);
void uartdma_init(void);
void uartDMAinterface_init(void);

void uartdma_tx_taskinit(void);
void uartdma_rx_createtask(void);
void uartdma_tx_createtask(void);

#endif /* UARTDMA_H_ */
