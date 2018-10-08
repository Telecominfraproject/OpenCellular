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
#include "inc/interfaces/usbcdcd.h"
#include "inc/common/global_header.h"
#include "inc/utils/util.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             MACROS DEFINITION
//*****************************************************************************
#define OCUSB_TX_TASK_PRIORITY 4
#define OCUSB_TX_TASK_STACK_SIZE 1024

#define OCUSB_RX_TASK_PRIORITY 5
#define OCUSB_RX_TASK_STACK_SIZE 1024

#define USB_FRAME_LENGTH OCMP_FRAME_TOTAL_LENGTH

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
//Semaphore
Semaphore_Handle semUSBRX;
Semaphore_Handle semUSBTX;

// Queue object
Queue_Struct usbRxMsg;
Queue_Struct usbtTxMsg;
Queue_Handle usbRxMsgQueue;
Queue_Handle usbTxMsgQueue;

/* Global Task Configuration Variables */
Task_Struct ocUSBTxTask;
Char ocUSBTxTaskStack[OCUSB_TX_TASK_STACK_SIZE];

Task_Struct ocUSBRxTask;
Char ocUSBRxTaskStack[OCUSB_RX_TASK_STACK_SIZE];

static uint8_t ui8TxBuf[USB_FRAME_LENGTH];
static uint8_t ui8RxBuf[USB_FRAME_LENGTH];

//*****************************************************************************
//                             FUNCTION DECLARATIONS
//*****************************************************************************
void usb_tx_createtask(void);
void usb_rx_createtask(void);
void usb_tx_taskinit(void);
void usb_rx_taskinit(void);

/*****************************************************************************
 **    FUNCTION NAME   : usb_tx_taskinit
 **
 **    DESCRIPTION     : Initializes USB TX task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void usb_tx_taskinit(void)
{
    /* Create Semaphore for USB TX Control Message Queue */
    semUSBTX = Semaphore_create(0, NULL, NULL);
    if (semUSBTX == NULL) {
        LOGGER_DEBUG("USBTX:ERROR:: Failed in Creating USB TX Semaphore.\n");
    }

    /* Create USB TX Message Queue for TX Messages */
    usbTxMsgQueue = Util_constructQueue(&usbtTxMsg);
    if (usbTxMsgQueue == NULL) {
        LOGGER_DEBUG(
                "USBTX:ERROR:: Failed in Constructing USB TX Message Queue for TX Messages.\n");
    } else {
        LOGGER_DEBUG(
                "USBTX:INFO:: Constructing message Queue for 0x%x USB TX Messages.\n",
                usbTxMsgQueue);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : usb_rx_taskinit
 **
 **    DESCRIPTION     : Initializes USB RX task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void usb_rx_taskinit(void)
{
    /* Create Semaphore for USB TX Control Message Queue */
    semUSBRX = Semaphore_create(0, NULL, NULL);
    if (semUSBRX == NULL) {
        LOGGER_DEBUG("USBRX:ERROR:: Failed in Creating USB RX Semaphore.\n");
    }

    /* Create USB RX Message Queue for RX Messages */
    usbRxMsgQueue = Util_constructQueue(&usbRxMsg);
    if (usbRxMsgQueue == NULL) {
        LOGGER_DEBUG(
                "USBRX:ERROR:: Failed in Constructing USB RX Message Queue for RX Messages.\n");
    } else {
        LOGGER_DEBUG(
                "USBRX:INFO:: Constructing message Queue for 0x%x USB RX Messages.\n",
                usbRxMsgQueue);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   :  usb_tx_taskfxn
 **
 **    DESCRIPTION     :  Handles USB TX Messages. This task sends data to the
 **                       USB host once it's connected.
 **
 **    ARGUMENTS       :  a0, a1 - not used
 **
 **    RETURN TYPE     :  None
 **
 *****************************************************************************/
void usb_tx_taskfxn(UArg arg0, UArg arg1)
{
    /* USB TX Task init*/
    usb_tx_taskinit();

    while (true) {
        /* Block while the device is NOT connected to the USB */
        USBCDCD_waitForConnect(BIOS_WAIT_FOREVER);
        if (Semaphore_pend(semUSBTX, BIOS_WAIT_FOREVER)) {
            /* OCMP UART TX Messgaes */
            while (!Queue_empty(usbTxMsgQueue)) {
                uint8_t *pWrite = (uint8_t *)Util_dequeueMsg(usbTxMsgQueue);
                if (pWrite) {
                    memset(ui8TxBuf, '\0', USB_FRAME_LENGTH);
                    memcpy(ui8TxBuf, pWrite, USB_FRAME_LENGTH);
#if 0
                    uint8_t i = 0;
                    LOGGER_DEBUG("USBTX:INFO:: USB TX BUFFER:\n");
                    for( i = 0; i < USB_FRAME_LENGTH; i++)
                    {
                        LOGGER_DEBUG("0x%x  ",ui8TxBuf[i]);
                    }
                    LOGGER_DEBUG("\n");
#endif
                    USBCDCD_sendData(pWrite, USB_FRAME_LENGTH,
                                     BIOS_WAIT_FOREVER);
                }
                free(pWrite);
            }
        }
    }
}

/*****************************************************************************
 **    FUNCTION NAME   :  usb_rx_taskfxn
 **
 **    DESCRIPTION     :  Handles USB RX Messages.This task will receive data
 **                       when data is available and block while the  device is
 **                       not connected to the USB host or if no data was
 **                       received.
 **
 **    ARGUMENTS       :  a0, a1 - not used
 **
 **    RETURN TYPE     :  None
 **
 *****************************************************************************/
void usb_rx_taskfxn(UArg arg0, UArg arg1)
{
    unsigned int received;

    USBCDCD_init();

    /* USB RX Task init*/
    usb_rx_taskinit();

    while (true) {
        /* Block while the device is NOT connected to the USB */
        USBCDCD_waitForConnect(BIOS_WAIT_FOREVER);

        received = USBCDCD_receiveData(ui8RxBuf, USB_FRAME_LENGTH,
                                       BIOS_WAIT_FOREVER);
        ui8RxBuf[received] = '\0';
        if (received && (ui8RxBuf[0] == 0x55)) {
            /* OCMP USB RX Messgaes */
            uint8_t *pWrite = NULL;
            pWrite = (uint8_t *)malloc(sizeof(OCMPMessageFrame) +
                                       OCMP_FRAME_MSG_LENGTH);
            if (pWrite != NULL) {
                memset(pWrite, '\0', USB_FRAME_LENGTH);
                memcpy(pWrite, ui8RxBuf, USB_FRAME_LENGTH);
#if 0
                uint8_t i = 0;
                LOGGER_DEBUG("USBRX:INFO:: USB RX BUFFER:\n");
                for( i = 0; i < USB_FRAME_LENGTH; i++)
                {
                    LOGGER_DEBUG("0x%x  ",ui8RxBuf[i]);
                }
                LOGGER_DEBUG("\n");
#endif
                Util_enqueueMsg(gossiperRxMsgQueue, semGossiperMsg, pWrite);
            } else {
                LOGGER_ERROR(
                        "USBRX:ERROR:: No memory left for Msg Length %d.\n",
                        USB_FRAME_LENGTH);
            }
        }
    }
}

/***************************************************************
 **    FUNCTION NAME   : usb_tx_createtask
 **
 **    DESCRIPTION     : Task creation function for the USB Tx.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 ****************************************************************/
void usb_tx_createtask(void)
{
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = OCUSB_TX_TASK_STACK_SIZE;
    taskParams.stack = &ocUSBTxTaskStack;
    taskParams.instance->name = "USB_TX_TASK";
    taskParams.priority = OCUSB_TX_TASK_PRIORITY;
    Task_construct(&ocUSBTxTask, (Task_FuncPtr)usb_tx_taskfxn, &taskParams,
                   NULL);
    LOGGER_DEBUG("USBTX:INFO:: Creating USB TX task function.\n");
}

/***************************************************************
 **    FUNCTION NAME   : usb_rx_createtask
 **
 **    DESCRIPTION     : Task creation function for the USB Rx.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 ****************************************************************/
void usb_rx_createtask(void)
{
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = OCUSB_RX_TASK_STACK_SIZE;
    taskParams.stack = &ocUSBRxTaskStack;
    taskParams.instance->name = "USB_RX_TASK";
    taskParams.priority = OCUSB_RX_TASK_PRIORITY;
    Task_construct(&ocUSBRxTask, (Task_FuncPtr)usb_rx_taskfxn, &taskParams,
                   NULL);
    LOGGER_DEBUG("USBRX:INFO:: Creating USB RX task function.\n");
}
