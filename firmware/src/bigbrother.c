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
#include "inc/common/bigbrother.h"

#include "Board.h"
#include "comm/gossiper.h"
#include "common/inc/global/ocmp_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/post.h"
#include "inc/common/system_states.h"
#include "inc/subsystem/hci/hci_buzzer.h"
#include "inc/utils/ocmp_util.h"
#include "registry/SSRegistry.h"

#include <ti/sysbios/BIOS.h>

#include <stdlib.h>
extern OcGpio_Port pwr_io;
extern OcGpio_Port  ec_io;

OcGpio_Pin pin_24v   = { &pwr_io, 3};
OcGpio_Pin pin_5v0   = { &pwr_io, 4};
OcGpio_Pin pin_3v3   = { &pwr_io, 5};
OcGpio_Pin pin_gbcv2_on   = { &pwr_io, 6};
OcGpio_Pin pin_12v_bb   = { &pwr_io, 7};
OcGpio_Pin pin_12v_fe   = { &pwr_io, 8};
OcGpio_Pin pin_20v_fe   = { &pwr_io, 9};
OcGpio_Pin pin_1v8   = { &pwr_io, 10};


/* Global Task Configuration Variables */
Task_Struct bigBrotherTask;
Char bigBrotherTaskStack[BIGBROTHER_TASK_STACK_SIZE];

eSubSystemStates oc_sys_state = SS_STATE_PWRON;
//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
/* Queue object */
/*
 * Semaphore for the Big Brother task where it will be waiting on.
 * Sub systems or Gossiper post this with respective queues filled in.
 */
Semaphore_Handle semBigBrotherMsg;

static Queue_Struct bigBrotherRxMsg;
static Queue_Struct bigBrotherTxMsg;

/*
 * bigBrotherRxMsgQueue - Used by the gossiper to pass the frame once recived
 * from Ethernet or UART and processed and needs to forward to bigBrother.
 */
Queue_Handle bigBrotherRxMsgQueue;
/*
 * bigBrotherTxMsgQueue - Used by the BigBrother to pass the frame once
 * underlying subsystem processed it (GPP/RF etc) and need to send back
 * to Gosipper. This is the one all the subsystem will be listening only.
 */
Queue_Handle bigBrotherTxMsgQueue;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
static void bigbrother_taskfxn(UArg a0, UArg a1);
static void bigbrother_init(void);
static ReturnStatus bigbrother_process_rx_msg(uint8_t *pMsg);
static ReturnStatus bigbrother_process_tx_msg(uint8_t *pMsg);
extern void post_createtask(void);
static void bigborther_initiate_post(void);

extern void gossiper_createtask(void);
extern void usb_rx_createtask(void);
extern void usb_tx_createtask(void);
extern void uartdma_rx_createtask(void);
extern void uartdma_tx_createtask(void);
//extern void watchdog_create_task(void);;

/*****************************************************************************
 **    FUNCTION NAME   : bb_sys_post_complete
 **
 **    DESCRIPTION     : Get POST results from EEPROM.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
ReturnStatus bb_sys_post_complete()
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("BIGBROTHER:INFO::POST test is completed.\n");
    static uint8_t count = 0;

    if (count == 0) {
       // Task_sleep(60000);
        // TODO: Starting UART DMA Interface based on EBMP.
        uartdma_rx_createtask();            // P - 07
        uartdma_tx_createtask();          // P - 07
        count++;
      //  uart_enable();
        /* TODO: enable this back */
#if ENABLE_POWER
        OcGpio_configure(&pin_24v,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_24v, 1);

        OcGpio_configure(&pin_5v0,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_5v0, 1);

        OcGpio_configure(&pin_3v3,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_3v3, 1);

        OcGpio_configure(&pin_gbcv2_on,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_gbcv2_on, 1);

        OcGpio_configure(&pin_12v_bb,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_12v_bb, 1);

        OcGpio_configure(&pin_12v_fe,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_12v_fe, 1);

        OcGpio_configure(&pin_20v_fe,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_20v_fe, 1);

        OcGpio_configure(&pin_1v8,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
        OcGpio_write(&pin_1v8, 1);
#endif
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_process_tx_msg
 **
 **    DESCRIPTION     : Processes the big brother outgoing messages.
 **
 **    ARGUMENTS       : Pointer to BIGBROTHER_TXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus bigbrother_process_tx_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("BIGBROTHER:INFO:: Processing Big Brother TX Message.\n");
    if (pMsg != NULL) {
        Util_enqueueMsg(gossiperTxMsgQueue, semGossiperMsg, (uint8_t*) pMsg);
    } else {
        LOGGER_ERROR("BIGBROTHER::ERROR::No Valid Pointer.\n");
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_process_rx_msg
 **
 **    DESCRIPTION     : Processes the big brother incoming messages.
 **
 **    ARGUMENTS       : Pointer to BIGBROTHER_RXEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus bigbrother_process_rx_msg(uint8_t *pMsg)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("BIGBROTHER:INFO:: Processing Big Brother RX Message.\n");
    OCMPMessageFrame * pOCMPMessageFrame = (OCMPMessageFrame *) pMsg;
    if (pOCMPMessageFrame != NULL) {
        LOGGER_DEBUG("BIGBROTHER:INFO:: RX Msg recieved with Length: 0x%x,"
                    "Interface: 0x%x, Seq.No: 0x%x, TimeStamp: 0x%x.\n",
                     pOCMPMessageFrame->header.ocmpFrameLen,
                     pOCMPMessageFrame->header.ocmpInterface,
                     pOCMPMessageFrame->header.ocmpSeqNumber,
                     pOCMPMessageFrame->header.ocmpTimestamp);
        // Forward this to respective subsystem.
        if (!SSRegistry_sendMessage(pOCMPMessageFrame->message.subsystem,
                                    pMsg)) {
            LOGGER_ERROR("BIGBROTHER::ERROR::Subsystem %d doesn't exist\n",
                         pOCMPMessageFrame->message.subsystem);
            free(pMsg);
        }
    } else {
        LOGGER_ERROR("BIGBROTHER:ERROR:: No message recieved.\n");
        free(pMsg);
    }
    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : bigborther_initiate_post
 **
 **    DESCRIPTION     : Creates POST test task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void bigborther_initiate_post(void)
{
    LOGGER_DEBUG("BIGBROTHER:INFO::Creating task to perform POST.\n");
    post_createtask();
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_ioexp_init
 **
 **    DESCRIPTION     : Initializes Io expander SX1509.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
OcGpio_Pin pin_v5_a_pgood   = { &ec_io, OC_EC_PGOOD_5V0};
OcGpio_Pin pin_v12_a_pgood   = { &ec_io, OC_EC_PGOOD_12V0};

ReturnStatus bigbrother_ioexp_init(void)
{
    ReturnStatus status = RETURN_OK;

    OcGpio_init(&pwr_io);

    /* Initialize pins that aren't covered yet by a subsystem */
    OcGpio_configure(&pin_v5_a_pgood, OCGPIO_CFG_INPUT);
    OcGpio_configure(&pin_v12_a_pgood, OCGPIO_CFG_INPUT);

    return status;
}

/*******************************************************************************
 **    FUNCTION NAME   : bigborther_spwan_task
 **
 **    DESCRIPTION     : Application task start up point for open cellular.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
static void bigborther_spwan_task(void)
{
    /* Read OC UID EEPROM */

    /* Check the list for possible devices connected. */

    /* Launches other tasks */
//    usb_rx_createtask();                // P - 05
//    usb_tx_createtask();                // P - 04
    gossiper_createtask();              // P - 06
//    ebmp_create_task();
//    watchdog_create_task();

    /* Initialize subsystem interface to set up interfaces and launch
     * subsystem tasks */
    SSRegistry_init();

}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_init
 **
 **    DESCRIPTION     : Initializes the Big Brother task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void bigbrother_init(void)
{
    /*Creating Semaphore for RX Message Queue*/
    semBigBrotherMsg = Semaphore_create(0, NULL, NULL);
    if (semBigBrotherMsg == NULL) {
        LOGGER_ERROR("BIGBROTHER:ERROR::BIGBROTHER RX Semaphore creation failed.\n");
    }
    /*Creating RX Message Queue*/
    bigBrotherRxMsgQueue = Util_constructQueue(&bigBrotherRxMsg);
    LOGGER_DEBUG("BIGBROTHER:INFO::Constructing message Queue for 0x%x Big Brother RX Messages.\n",
                 bigBrotherRxMsgQueue);

    /*Creating TX Message Queue*/
    bigBrotherTxMsgQueue = Util_constructQueue(&bigBrotherTxMsg);
    LOGGER_DEBUG("BIGBROTHER:INFO::Constructing message Queue for 0x%x Big Brother RX Messages.\n",
                 bigBrotherTxMsgQueue);
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_taskfxn
 **
 **    DESCRIPTION     : handles the system state and subsystem states.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void bigbrother_taskfxn(UArg a0, UArg a1)
{
    bigbrother_init();

    /* Initialize GPIO Expander SX1509 */
    bigbrother_ioexp_init();
//    hci_buzzer_beep(1);

    //Create Tasks.
    bigborther_spwan_task();
    //Perform POST
    bigborther_initiate_post();
    while (true) {
        if (Semaphore_pend(semBigBrotherMsg, BIOS_WAIT_FOREVER)) {
            while (!Queue_empty(bigBrotherRxMsgQueue)) {
                uint8_t *pWrite = (uint8_t *) Util_dequeueMsg(
                                  bigBrotherRxMsgQueue);
                if (pWrite) {
                    bigbrother_process_rx_msg(pWrite);
                }
            }
            while (!Queue_empty(bigBrotherTxMsgQueue)) {
                uint8_t *pWrite = (uint8_t *) Util_dequeueMsg(
                                  bigBrotherTxMsgQueue);
                if (pWrite) {
                    bigbrother_process_tx_msg(pWrite);
                }
            }
        }
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : bigbrother_createtask
 **
 **    DESCRIPTION     : Creates task for Big Brother task
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void bigbrother_createtask(void)
{
    //watchdog_pin();
    Task_Params taskParams;
    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = bigBrotherTaskStack;
    taskParams.stackSize = BIGBROTHER_TASK_STACK_SIZE;
    taskParams.priority = BIGBROTHER_TASK_PRIORITY;
    Task_construct(&bigBrotherTask, bigbrother_taskfxn, &taskParams, NULL);
    LOGGER_DEBUG("BIGBROTHER:INFO::Creating a BigBrother task.\n");
}
