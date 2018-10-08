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
#include "comm/gossiper.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/interfaces/uartdma.h"
#include "inc/common/global_header.h"

#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/uart.h>
#include <driverlib/udma.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_uart.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdlib.h>
#include <string.h>

/*****************************************************************************
 *                             HANDLES DEFINITION
 *****************************************************************************/
/* Semaphore */
Semaphore_Handle semUART;
Semaphore_Handle semUARTTX;

/* Queue object */
Queue_Struct uartRxMsg;
Queue_Struct uartTxMsg;
Queue_Handle uartRxMsgQueue;
Queue_Handle uartTxMsgQueue;

/* Global Task Configuration Variables */
Task_Struct ocUARTDMATask;
Char ocUARTDMATaskStack[OCUARTDMA_TASK_STACK_SIZE];

Task_Struct ocUARTDMATxTask;
Char ocUARTDMATxTaskStack[OCUARTDMATX_TASK_STACK_SIZE];

/*****************************************************************************
 * The transmit and receive buffers used for the UART transfers.  There is one
 * transmit buffer and a pair of recieve ping-pong buffers.
 ******************************************************************************/
static uint8_t ui8TxBuf[UART_TXBUF_SIZE];
static uint8_t ui8RxBufA[UART_RXBUF_SIZE];
static uint8_t ui8RxBufB[UART_RXBUF_SIZE];
static uint8_t ui8uartdmaRxBuf[UART_RXBUF_SIZE];

/*****************************************************************************
 * The control table used by the uDMA controller.  This table must be aligned
 * to a 1024 byte boundary.
 *****************************************************************************/
#pragma DATA_ALIGN(pui8ControlTable, 1024)

uint8_t pui8ControlTable[1024];

/*****************************************************************************
 *
 * The interrupt handler for uDMA errors.  This interrupt will occur if the
 * uDMA encounters a bus error while trying to perform a transfer.  This
 * handler just increments a counter if an error occurs.
 *****************************************************************************/
void uDMAErrorHandler(void)
{
    uint32_t ui32Status;

    /* Check for uDMA error bit. */
    ui32Status = uDMAErrorStatusGet();

    /* If there is a uDMA error, then clear the error and increment the error
     * counter.*/
    if (ui32Status) {
        uDMAErrorStatusClear();
        LOGGER_WARNING("UARTDMACTR:WARNING::Something went bad in uDMA.\n");
    }
}

/*****************************************************************************
 *
 * The interrupt handler for UART4.
 *
 *****************************************************************************/
void UART4IntHandler(void)
{
    uint32_t ui32Status;
    uint32_t ui32Mode;
    ui32Status = UARTIntStatus(UART4_BASE, 1);

    /* Clear any pending status*/
    UARTIntClear(UART4_BASE, ui32Status);

    /*Primary Buffer*/
    ui32Mode = uDMAChannelModeGet(UDMA_CHANNEL_TMR0A | UDMA_PRI_SELECT);
    if (ui32Mode == UDMA_MODE_STOP) {
        uDMAChannelTransferSet(
                UDMA_CHANNEL_TMR0A | UDMA_PRI_SELECT, UDMA_MODE_PINGPONG,
                (void *)(UART4_BASE + UART_O_DR), ui8RxBufA, sizeof(ui8RxBufA));
        /*Preparing message to send to UART RX Queue*/
        memset(ui8uartdmaRxBuf, '\0', UART_RXBUF_SIZE);
        memcpy(ui8uartdmaRxBuf, ui8RxBufA, sizeof(ui8RxBufA));
        Semaphore_post(semUART);
    }

    /*Alternate Buffer*/
    ui32Mode = uDMAChannelModeGet(UDMA_CHANNEL_TMR0A | UDMA_ALT_SELECT);
    if (ui32Mode == UDMA_MODE_STOP) {
        uDMAChannelTransferSet(
                UDMA_CHANNEL_TMR0A | UDMA_ALT_SELECT, UDMA_MODE_PINGPONG,
                (void *)(UART4_BASE + UART_O_DR), ui8RxBufB, sizeof(ui8RxBufB));
        /*Preparing message to send to UART RX Queue*/
        memset(ui8uartdmaRxBuf, '\0', UART_RXBUF_SIZE);
        memcpy(ui8uartdmaRxBuf, ui8RxBufB, sizeof(ui8RxBufB));
        Semaphore_post(semUART);
    }
}

/*****************************************************************************
 * Reset and configure DMA and UART.
 *****************************************************************************/
void resetUARTDMA(void)
{
    LOGGER_WARNING("UARTDMACTR:WARNING::Configuring UART DMA again.....!!!\n");

    // Configure UART.*/
    ConfigureUART();

    /* Configure UART.*/
    uartdma_init();
    LOGGER("UARTDMACTR:INFO::Re-Configuring UART DMA again.....!!!\n");
}

/*****************************************************************************
 * Configure the UART and its pins.  This must be called before UARTprintf().
 *****************************************************************************/
void ConfigureUART(void)
{
    LOGGER_DEBUG(
            "UARTDMACTR:INFO::Configuring UART interface for communication.\n");

    /* Enable the GPIO Peripheral used by the UART.*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);

    /* Enable UART3 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART4);

    /* Configure GPIO Pins for UART mode.*/
    GPIOPinConfigure(GPIO_PK0_U4RX);
    GPIOPinConfigure(GPIO_PK1_U4TX);
    GPIOPinTypeUART(GPIO_PORTK_BASE, GPIO_PIN_0 | GPIO_PIN_1);
}

/****************************************************************************
 *
 * Initializes the UART3 peripheral and sets up the TX and RX uDMA channels.
 *****************************************************************************/
void InitUART4Transfer(void)
{
    LOGGER_DEBUG(
            "UARTDMACTR:INFO::Configuring UART interrupt and uDMA channel for communication to GPP.\n");
    uint_fast16_t ui16Idx;
    const uint32_t SysClock = 120000000;

    /* TX buffer init to 0.*/
    for (ui16Idx = 0; ui16Idx < UART_TXBUF_SIZE; ui16Idx++) {
        ui8TxBuf[ui16Idx] = 0;
    }

    /* Enable the UART peripheral, and configure it to operate even if the CPU
     * is in sleep.*/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART4);

    /* Configure the UART communication parameters.*/

    UARTConfigSetExpClk(UART4_BASE, SysClock, 115200,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                                UART_CONFIG_PAR_NONE);

    /* Set both the TX and RX trigger thresholds to 4.  */
    UARTFIFOLevelSet(UART4_BASE, UART_FIFO_TX4_8, UART_FIFO_RX4_8);

    /* Enable the UART for operation, and enable the uDMA interface for both TX
     * and RX channels.*/
    UARTEnable(UART4_BASE);
    UARTDMAEnable(UART4_BASE, UART_DMA_RX | UART_DMA_TX);

    uDMAChannelAttributeDisable(UDMA_CHANNEL_TMR0A,
                                UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
                                        UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);

    uDMAChannelControlSet(UDMA_CHANNEL_TMR0A | UDMA_PRI_SELECT,
                          UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                                  UDMA_ARB_4);

    uDMAChannelControlSet(UDMA_CHANNEL_TMR0A | UDMA_ALT_SELECT,
                          UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                                  UDMA_ARB_4);

    uDMAChannelTransferSet(UDMA_CHANNEL_TMR0A | UDMA_PRI_SELECT,
                           UDMA_MODE_PINGPONG, (void *)(UART4_BASE + UART_O_DR),
                           ui8RxBufA, sizeof(ui8RxBufA));

    uDMAChannelTransferSet(UDMA_CHANNEL_TMR0A | UDMA_ALT_SELECT,
                           UDMA_MODE_PINGPONG, (void *)(UART4_BASE + UART_O_DR),
                           ui8RxBufB, sizeof(ui8RxBufB));

    uDMAChannelAttributeDisable(UDMA_CHANNEL_TMR0B,
                                UDMA_ATTR_ALTSELECT | UDMA_ATTR_HIGH_PRIORITY |
                                        UDMA_ATTR_REQMASK);

    uDMAChannelAttributeEnable(UDMA_CHANNEL_TMR0B, UDMA_ATTR_USEBURST);

    uDMAChannelControlSet(UDMA_CHANNEL_TMR0B | UDMA_PRI_SELECT,
                          UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE |
                                  UDMA_ARB_4);

    uDMAChannelTransferSet(UDMA_CHANNEL_TMR0B | UDMA_PRI_SELECT,
                           UDMA_MODE_BASIC, ui8TxBuf,
                           (void *)(UART4_BASE + UART_O_DR), sizeof(ui8TxBuf));

    uDMAChannelAssign(UDMA_CH18_UART4RX);
    uDMAChannelAssign(UDMA_CH19_UART4TX);

    uDMAChannelEnable(UDMA_CHANNEL_TMR0A);
    uDMAChannelEnable(UDMA_CHANNEL_TMR0B);

    /* Enable the UART DMA TX/RX interrupts.*/
    UARTIntEnable(UART4_BASE, UART_INT_DMARX);

    /* Enable the UART peripheral interrupts.*/
    IntEnable(INT_UART4);
}

/*****************************************************************************
 * Intialize UART uDMA for the data transfer. This will initialise both Tx and
 * Rx Channel associated with UART Tx and Rx
 ****************************************************************************/
void uartdma_init(void)
{
    LOGGER_DEBUG("UARTDMACTR:INFO::Starting uDMA intilaization.\n");
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UDMA);
    IntEnable(INT_UDMAERR);
    uDMAEnable();
    uDMAControlBaseSet(pui8ControlTable);
    InitUART4Transfer();
}

/*****************************************************************************
 * Initialize the UART with DMA interface.
 ****************************************************************************/
void uartDMAinterface_init(void)
{
    /* Configure UART */
    ConfigureUART();

    /* Initialize UART */
    uartdma_init();

    /*UART RX Semaphore */
    LOGGER_DEBUG("UARTDMACTR:INFO:: uartDMA interface intialization.\n");
    semUART = Semaphore_create(0, NULL, NULL);
    if (semUART == NULL) {
        LOGGER_ERROR("UARTDMACTR:ERROR::UART RX Semaphore creation failed.\n");
    }

    /*UART OCMP RX Message Queue*/
    uartRxMsgQueue = Util_constructQueue(&uartRxMsg);
    LOGGER_DEBUG(
            "UARTDMACTR:INFO::Constructing message Queue 0x%x for UART RX OCMP Messages.\n",
            uartRxMsgQueue);

    LOGGER_DEBUG("UARTDMACTR:INFO::Waiting for OCMP UART RX messgaes....!!!\n");
}

/*****************************************************************************
 * uartdma_rx_taskfxn -Handles the UART received data.
 ****************************************************************************/
static void uartdma_rx_taskfxn(UArg arg0, UArg arg1)
{
    // Initialize application
    uartDMAinterface_init();

    // Application main loop
    while (true) {
        if (Semaphore_pend(semUART, BIOS_WAIT_FOREVER)) {
            /* Reset Uart DMA if the SOF is not equal to 0X55 */
            if (ui8uartdmaRxBuf[0] != OCMP_MSG_SOF) {
                resetUARTDMA();
            } else {
                /* OCMP UART RX Messgaes */
                uint8_t *pWrite = NULL;
                pWrite = (uint8_t *)malloc(sizeof(OCMPMessageFrame) +
                                           OCMP_FRAME_MSG_LENGTH);
                if (pWrite != NULL) {
                    memset(pWrite, '\0', UART_RXBUF_SIZE);
                    memcpy(pWrite, ui8uartdmaRxBuf, UART_RXBUF_SIZE);
#if 0
                    uint8_t i = 0;
                    LOGGER_DEBUG("UARTDMACTR:INFO:: UART RX BUFFER:\n");
                    for( i = 0; i < UART_RXBUF_SIZE; i++)
                    {
                        LOGGER_DEBUG("0x%x  ",ui8uartdmaRxBuf[i]);
                    }
                    LOGGER_DEBUG("\n");
#endif
                    Util_enqueueMsg(gossiperRxMsgQueue, semGossiperMsg, pWrite);
                } else {
                    LOGGER_ERROR(
                            "UARTDMACTR:ERROR:: No memory left for Msg Length %d.\n",
                            UART_RXBUF_SIZE);
                }
            }
        }
    }
}

/*****************************************************************************
 * uartdma_tx_taskinit - Creating IPC objects
 *****************************************************************************/
void uartdma_tx_taskinit(void)
{
    LOGGER_DEBUG("UARTDMACTR:INFO:: uartDMA interface intialization.\n");

    /*UART TX Semaphore */
    semUARTTX = Semaphore_create(0, NULL, NULL);
    if (semUARTTX == NULL) {
        LOGGER_ERROR("UARTDMACTR:ERROR::UART TX Semaphore creation failed.\n");
    }

    /*UART OCMP TX Message Queue*/
    uartTxMsgQueue = Util_constructQueue(&uartTxMsg);
    LOGGER_DEBUG(
            "UARTDMACTR:INFO::Constructing message Queue 0x%x for UART TX OCMP Messages.\n",
            uartTxMsgQueue);
}

/*****************************************************************************
 **    FUNCTION NAME   :  uartdma_process_tx_message
 **
 **    DESCRIPTION     : transmitt TX Messages to UART physical medium
 **
 **    ARGUMENTS       : Pointer to UARTDMATX_Evt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus uartdma_process_tx_message(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    if (!uDMAChannelIsEnabled(UDMA_CHANNEL_TMR0B)) {
        memset(ui8TxBuf, '\0', UART_TXBUF_SIZE);

        memcpy(ui8TxBuf, pMsg, UART_TXBUF_SIZE);
#if 0
        uint8_t i = 0;
        LOGGER_DEBUG("UARTDMACTR:INFO:: UART TX BUFFER:\n");
        for( i = 0; i < UART_TXBUF_SIZE; i++)
        {
            LOGGER_DEBUG("0x%x  ",ui8TxBuf[i]);
        }
        LOGGER_DEBUG("\n");
#endif
        uDMAChannelTransferSet(
                UDMA_CHANNEL_TMR0B | UDMA_PRI_SELECT, UDMA_MODE_BASIC, ui8TxBuf,
                (void *)(UART4_BASE + UART_O_DR), sizeof(ui8TxBuf));
        uDMAChannelEnable(UDMA_CHANNEL_TMR0B);
    } else {
        status = RETURN_NOTOK;
    }
    return status;
}

/*****************************************************************************
 * uartdma_tx_taskfxn - Handles the UART sent data.
 ****************************************************************************/
static void uartdma_tx_taskfxn(UArg arg0, UArg arg1)
{
    /* UARTDMA TX init*/
    uartdma_tx_taskinit();

    // Application main loop
    while (true) {
        if (Semaphore_pend(semUARTTX, BIOS_WAIT_FOREVER)) {
            /* OCMP UART TX Messgaes */
            while (!Queue_empty(uartTxMsgQueue)) {
                uint8_t *pWrite = (uint8_t *)Util_dequeueMsg(uartTxMsgQueue);
                if (pWrite) {
                    uartdma_process_tx_message(pWrite);
                }
                free(pWrite);
            }
        }
    }
}

/******************************************************************************
 **    FUNCTION NAME   : uartdma_rx_createtask
 **
 **    DESCRIPTION     : Task creation function for the UARTDMA
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
void uartdma_rx_createtask(void)
{
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = OCUARTDMA_TASK_STACK_SIZE;
    taskParams.stack = &ocUARTDMATaskStack;
    taskParams.instance->name = "UART_DMA_TASK";
    taskParams.priority = OCUARTDMA_TASK_PRIORITY;
    Task_construct(&ocUARTDMATask, (Task_FuncPtr)uartdma_rx_taskfxn,
                   &taskParams, NULL);
    LOGGER_DEBUG("UARTDMACTRl:INFO::Creating UART DMA task function.\n");
}

/******************************************************************************
 **    FUNCTION NAME   : uartdma_tx_createtask
 **
 **    DESCRIPTION     : Task creation function for the UARTDMA TX
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
void uartdma_tx_createtask(void)
{
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = OCUARTDMATX_TASK_STACK_SIZE;
    taskParams.stack = &ocUARTDMATxTaskStack;
    taskParams.instance->name = "UART_DMA_TX_TASK";
    taskParams.priority = OCUARTDMATX_TASK_PRIORITY;
    Task_construct(&ocUARTDMATxTask, (Task_FuncPtr)uartdma_tx_taskfxn,
                   &taskParams, NULL);
    LOGGER_DEBUG("UARTDMACTRl:INFO::Creating UART DMA TX task function.\n");
}
