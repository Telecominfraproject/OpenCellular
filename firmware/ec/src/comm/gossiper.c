/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/*****************************************************************************
 *                                HEADER FILES
 *****************************************************************************/
#include "comm/gossiper.h"

#include "common/inc/global/ocmp_frame.h"
#include "inc/common/bigbrother.h"
#include "inc/common/global_header.h"
#include "inc/interfaces/uartdma.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

/*****************************************************************************
 *                             HANDLES DEFINITION
 *****************************************************************************/
// Semaphore on which USB will listen.
extern Semaphore_Handle semUSBTX;

/*
 * usbTxMsgQueue - Message queue of USB interface to read.
 * Will be filled in by Gossiper.
 */
extern Queue_Handle usbTxMsgQueue;

extern Semaphore_Handle ethTxsem;
extern Queue_Handle ethTxMsgQueue;

/* Config message Queue */
/*
 * This is the semaphore posted by either Interface (UART/Ethernet) when it has
 * recieved from external world or Bigbrother task once frame is processed and
 * needs to be sent back.
 */
Semaphore_Handle semGossiperMsg;

// Queue object
static Queue_Struct gossiperRxMsg;
static Queue_Struct gossiperTxMsg;

/*
 * gossiperRxMsgQueue - Queue used by the Interface to
 * send data to Gossiper
 */
Queue_Handle gossiperRxMsgQueue;

/*
 * gossiperTxMsgQueue - Queue used by the Big brother to
 * forward data to Gossiper
 */
Queue_Handle gossiperTxMsgQueue;

/* Global Task Configuration Variables */
static Task_Struct gossiperTask;
static Char gossiperTaskStack[GOSSIPER_TASK_STACK_SIZE];

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
static void gossiper_init();
static void gossiper_taskfxn(UArg a0, UArg a1);
static ReturnStatus gossiper_process_rx_msg(uint8_t *pMsg);
static ReturnStatus gossiper_process_tx_msg(uint8_t *pMsg);
static ReturnStatus gossiper_uart_send_msg(uint8_t *pMsg);
static ReturnStatus gossiper_usb_send_msg(uint8_t *pMsg);
static ReturnStatus gossiper_ethernet_send_msg(uint8_t *pMsg);

/*****************************************************************************
 **    FUNCTION NAME   : gossiper_createtask
 **
 **    DESCRIPTION     : Creates task for Gossiper
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void gossiper_createtask(void)
{
    Task_Params taskParams;
    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = gossiperTaskStack;
    taskParams.stackSize = GOSSIPER_TASK_STACK_SIZE;
    taskParams.priority = GOSSIPER_TASK_PRIORITY;
    Task_construct(&gossiperTask, gossiper_taskfxn, &taskParams, NULL);
    LOGGER_DEBUG("GOSSIPER:INFO::Creating a Gossiper task.\n");
}

/*****************************************************************************
 **    FUNCTION NAME   : gossiper_init
 **
 **    DESCRIPTION     : Initializes the gossiper task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void gossiper_init(void)
{
    /*Creating Semaphore for RX Message Queue*/
    semGossiperMsg = Semaphore_create(0, NULL, NULL);
    if (semGossiperMsg == NULL) {
        LOGGER_ERROR(
                "GOSSIPER:ERROR::GOSSIPER RX Semaphore creation failed.\n");
    }

    /*Creating RX Message Queue*/
    gossiperRxMsgQueue = Util_constructQueue(&gossiperRxMsg);
    LOGGER_DEBUG(
            "GOSSIPER:INFO::Constructing message Queue 0x%x for RX Gossiper Messages.\n",
            gossiperRxMsgQueue);

    /*Creating TX Message Queue*/
    gossiperTxMsgQueue = Util_constructQueue(&gossiperTxMsg);
    LOGGER_DEBUG(
            "GOSSIPER:INFO::Constructing message Queue 0x%x for TX Gossiper Messages.\n",
            gossiperTxMsgQueue);
}

/*****************************************************************************
 **    FUNCTION NAME   : gossiper_taskfxn
 **
 **    DESCRIPTION     : Recieve and transmitts media depenedent messages.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void gossiper_taskfxn(UArg a0, UArg a1)
{
    gossiper_init();
    while (true) {
        if (Semaphore_pend(semGossiperMsg, BIOS_WAIT_FOREVER)) {
            /* Gossiper RX Messgaes */
            while (!Queue_empty(gossiperRxMsgQueue)) {
                uint8_t *pWrite =
                        (uint8_t *)Util_dequeueMsg(gossiperRxMsgQueue);
                if (pWrite) {
                    gossiper_process_rx_msg(pWrite);
                } else {
                    LOGGER_ERROR("GOSSIPER::ERROR:: No Valid Pointer.\n");
                }
            }

            /* Gossiper TX Messgaes */
            while (!Queue_empty(gossiperTxMsgQueue)) {
                uint8_t *pWrite =
                        (uint8_t *)Util_dequeueMsg(gossiperTxMsgQueue);
                if (pWrite) {
                    gossiper_process_tx_msg(pWrite);
                } else {
                    LOGGER_ERROR("GOSSIPER::ERROR:: No Valid Pointer.\n");
                }
            }
        }
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : gossiper_process_rx_msg
 **
 **    DESCRIPTION     : Processes the RX Gossiper messages
 **
 **    ARGUMENTS       : Pointer to GOSSIPERRXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus gossiper_process_rx_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("GOSSIPER:INFO:: Processing Gossiper RX Message.\n");

    OCMPMessageFrame *pOCMPMessageFrame = (OCMPMessageFrame *)pMsg;
    if (pOCMPMessageFrame != NULL) {
        LOGGER_DEBUG(
                "GOSSIPER:INFO:: RX Msg recieved with Length: 0x%x, Interface: 0x%x, Seq.No: 0x%x, TimeStamp: 0x%x.\n",
                pOCMPMessageFrame->header.ocmpFrameLen,
                pOCMPMessageFrame->header.ocmpInterface,
                pOCMPMessageFrame->header.ocmpSeqNumber,
                pOCMPMessageFrame->header.ocmpTimestamp);
        /*Update the Debug info required based on the debug jumper connected*/
        //status = CheckDebugEnabled()
        if (pOCMPMessageFrame->message.msgtype == OCMP_MSG_TYPE_DEBUG) {
#if 0
            if (!IN_DEBUGMODE()) {
                // If board is not set in debug mode then discard the message.
            } else {
                pOCMPMessageFrame->message.msgtype = UNSET_DEBUG_MODE(
                        pOCMPMessageFrame->message.msgtype);
            }
#endif
        }
        Util_enqueueMsg(bigBrotherRxMsgQueue, semBigBrotherMsg,
                        (uint8_t *)pMsg);
    } else {
        LOGGER_ERROR("GOSSIPER:ERROR:: Not valid pointer.\n");
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : gossiper_process_tx_msg
 **
 **    DESCRIPTION     : Processes the Gossiper TX Messages
 **
 **    ARGUMENTS       : Pointer to GOSSIPERTXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus gossiper_process_tx_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("GOSSIPER:INFO:: Processing Gossiper TX Message.\n");
    OCMPMessageFrame *pOCMPMessageFrame = (OCMPMessageFrame *)pMsg;
    if (pOCMPMessageFrame != NULL) {
        if (pOCMPMessageFrame->header.ocmpInterface == OCMP_COMM_IFACE_UART) {
            status = gossiper_uart_send_msg(pMsg);
        } else if (pOCMPMessageFrame->header.ocmpInterface ==
                   OCMP_COMM_IFACE_ETHERNET) {
            status = gossiper_ethernet_send_msg(pMsg);
        } else if (pOCMPMessageFrame->header.ocmpInterface ==
                   OCMP_COMM_IFACE_SBD) {
            // Will be added later.
        } else if (pOCMPMessageFrame->header.ocmpInterface ==
                   OCMP_COMM_IFACE_USB) {
            status = gossiper_usb_send_msg(pMsg);
        }
    } else {
        LOGGER_ERROR("BIGBROTHER:ERROR:: Not valid pointer.\n");
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : gossiper_ethernet_send_msg
 **
 **    DESCRIPTION     : transmitt TX Messages to Ethernet
 **
 **    ARGUMENTS       : Pointer to GOSSIPERTXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus gossiper_ethernet_send_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG(
            "GOSSIPER:INFO:: Forwarding TX message to the ETH Interface.\n");
    if (pMsg != NULL) {
        Util_enqueueMsg(ethTxMsgQueue, ethTxsem, (uint8_t *)pMsg);
    } else {
        LOGGER_ERROR("GOSSIPER::ERROR::No Valid Pointer.\n");
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : gossiper_uart_send_msg
 **
 **    DESCRIPTION     : transmitt TX Messages to UART
 **
 **    ARGUMENTS       : Pointer to GOSSIPERTXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus gossiper_uart_send_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG(
            "GOSSIPER:INFO:: Forwarding TX message to the UART Interface.\n");
    if (pMsg != NULL) {
        Util_enqueueMsg(uartTxMsgQueue, semUARTTX, (uint8_t *)pMsg);
    } else {
        LOGGER_ERROR("GOSSIPER::ERROR::No Valid Pointer.\n");
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : usbTXMessages
 **
 **    DESCRIPTION     : Transmitt TX Messages to USB.
 **
 **    ARGUMENTS       : Pointer to GOSSIPERTXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus gossiper_usb_send_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG(
            "GOSSIPER:INFO:: Forwarding TX message to the USB Interface.\n");
    if (pMsg != NULL) {
        Util_enqueueMsg(usbTxMsgQueue, semUSBTX, (uint8_t *)pMsg);
    } else {
        LOGGER_ERROR("GOSSIPER::ERROR::No Valid Pointer.\n");
    }
    return status;
}
