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
#include "inc/common/post.h"
#include "inc/common/post_frame.h"
#include "comm/gossiper.h"
#include "inc/common/bigbrother.h"
#include "inc/common/ocmp_frame.h"
#include "inc/devices/eth_sw.h"
#include "inc/subsystem/bms/bms.h"
#include "inc/subsystem/gpp/gpp.h"
#include "inc/subsystem/hci/hci.h"
#include "inc/subsystem/power/power.h"
#include "inc/subsystem/rffe/rffe.h"
#include "inc/subsystem/sdr/sdr.h"
#include "inc/subsystem/sync/sync.h"
#include "inc/utils/ocmp_util.h"
#include "registry/SSRegistry.h"

#include <ti/sysbios/BIOS.h>

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************

/* Queue object */
/*
 * Semaphore for the POST task where it will be waiting on
 * sub systems or BigBrother postmessages.
 */
Semaphore_Handle semPOSTMsg;

static Queue_Struct postRxMsg;

/*
 * postRxMsgQueue - Used by the BigBrother/Subsystem to pass the POST request
 * and ack message.
 */
Queue_Handle postRxMsgQueue;

/* Global Task Configuration Variables */
static Task_Struct postTask;
static Char postTaskStack[POST_TASK_STACK_SIZE];

/* POST state */
static uint8_t postState = 0;
static OCMPSubsystem POST_subSystem;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
static void post_taskfxn(UArg a0, UArg a1);
static void post_task_init(void);
static ReturnStatus post_process_msg(OCMPSubsystem OC_subSystem);
static OCMPMessageFrame* post_create_execute_msg(OCMPSubsystem OC_subSystem);
static void post_activate(OCMPMessageFrame *pPOSTMsg);
static void post_process_rx_msg(OCMPMessageFrame *pPOSTMsg);
static void post_move_to_next_subsystem();
static void post_update_result_to_bigbrother(OCMPMessageFrame *pPOSTMsg);

POSTData PostResult[POST_RECORDS] = { { 0 } };
static uint8_t deviceCount = 0;

/*****************************************************************************
 **    FUNCTION NAME   : post_move_to_next_subsystem
 **
 **    DESCRIPTION     : Move to next subssytem in the OCSubSystem.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void post_move_to_next_subsystem()
{
    POST_subSystem = (OCMPSubsystem) (POST_subSystem + (OCMPSubsystem) 1);
    if (POST_subSystem > OC_SS_MAX_LIMIT) {
        POST_subSystem = OC_SS_BB;
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : post_update_result_to_bigbrother
 **
 **    DESCRIPTION     : Send POST completion status to Bigbrother.
 **
 **    ARGUMENTS       : OCMPMessageFrame pointer for the update status
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void post_update_result_to_bigbrother(OCMPMessageFrame *pPOSTMsg)
{
    pPOSTMsg->message.subsystem = OC_SS_BB; //OC_SUBSYSTEM_MAX_LIMIT subsystem number taken for bigbrother
    memcpy((pPOSTMsg->message.ocmp_data), &postState, 1);
    Util_enqueueMsg(bigBrotherRxMsgQueue, semBigBrotherMsg,
                    (uint8_t*) pPOSTMsg);
}

/*****************************************************************************
 **    FUNCTION NAME   : post_process_msg
 **
 **    DESCRIPTION     : Forward excecute POST message to subsystem.
 **
 **    ARGUMENTS       : Subsystem
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static ReturnStatus post_process_msg(OCMPSubsystem OC_subSystem)
{
    ReturnStatus status = RETURN_OK;
    LOGGER_DEBUG("POST:INFO:: Processing execute POST for the Subsystem %d.\n",
                    OC_subSystem);
    OCMPMessageFrame *postFrame = post_create_execute_msg(OC_subSystem);

    if (OC_subSystem == OC_SS_WD) {
        /* TODO: WD no implementaion added, POST is completetd here.*/
        /* Once Test module added POST completetion message has to be sent
         * at OC_SS_MAX_LIMIT */
        post_update_result_to_bigbrother(postFrame);
        POST_subSystem = OC_SS_BB;
    } else if (OC_subSystem == OC_SS_MAX_LIMIT) {
        post_update_result_to_bigbrother(postFrame);
        POST_subSystem = OC_SS_BB;
    } else {
        if (!SSRegistry_sendMessage(OC_subSystem, postFrame)) {
            LOGGER_DEBUG("POST:ERROR::Subsystem %d does not exist\n",
                         OC_subSystem);
        }
    }

    return status;
}

/*****************************************************************************
 **    FUNCTION NAME   : post_create_execute_msg
 **
 **    DESCRIPTION     : create excecute POST message for subsystem.
 **
 **    ARGUMENTS       : Subsystem
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static OCMPMessageFrame* post_create_execute_msg(OCMPSubsystem OC_subSystem)
{
    uint8_t dummyByte = 0xff;
    OCMPActionType actionType = OCMP_AXN_TYPE_ACTIVE;
    LOGGER_DEBUG("POST:INFO:: Processing execute POST for the Subsystem %d.\n",
                    OC_subSystem);
    if(OC_subSystem == OC_SS_MAX_LIMIT) {
        OC_subSystem = OC_SS_BB;
        actionType = OCMP_AXN_TYPE_REPLY;
    } else {
        actionType = OCMP_AXN_TYPE_ACTIVE;
    }
    OCMPMessageFrame *postExeMsg = create_ocmp_msg_frame(OC_subSystem,
                                                        OCMP_MSG_TYPE_POST,
                                                        actionType,0x00,0x00,1);
    return postExeMsg;
}

/*****************************************************************************
 **    FUNCTION NAME   : post_process_rx_msg
 **
 **    DESCRIPTION     : Processes the POST Acks received from the subssystems.
 **
 **    ARGUMENTS       : Pointer to POST_AckEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void post_activate(OCMPMessageFrame *pPOSTMsg)
{
    ReturnStatus POSTAckstatus = RETURN_NOTOK;
    LOGGER_DEBUG("POST:INFO:: Processing POST Ack received from the Subsystem %d.\n",
                     POST_subSystem);
    System_printf("POST:INFO:: Processing POST Ack received from the Subsystem %d.\n",
                         POST_subSystem);
    System_flush();
    //Do the casting for the pMsg
    //POSTAckstatus = (ReturnStatus) (pPOSTMsg->message.ocmp_data);
    memcpy(&POSTAckstatus, pPOSTMsg->message.ocmp_data, 1);
    //Compare the Ack
    if (POST_subSystem == OC_SS_BB) {
        postState = RETURN_OK;
    }
    if (pPOSTMsg->message.subsystem == POST_subSystem) {
        postState = (!POSTAckstatus) & postState;
        LOGGER_DEBUG("POST:INFO:: POST status for 0x%x Subsystem is 0x%x and OC POST status is 0x%x.\n",
                         POST_subSystem, POSTAckstatus, postState);
        if (pPOSTMsg) {
            free(pPOSTMsg);
        }
    }
    post_move_to_next_subsystem();
    post_process_msg(POST_subSystem);
}

/*****************************************************************************
 **    FUNCTION NAME   : post_process_rx_msg
 **
 **    DESCRIPTION     : Processes the POST Acks received from the subssystems.
 **
 **    ARGUMENTS       : Pointer to POST_AckEvt_t structure
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void post_process_rx_msg(OCMPMessageFrame *pPOSTMsg)
{
    LOGGER_DEBUG("POST:INFO::Processing POST messages.\n");
    switch (pPOSTMsg->message.action) {
        case OCMP_AXN_TYPE_ACTIVE:
        case OCMP_AXN_TYPE_REPLY:
        {
            post_activate(pPOSTMsg);
        }
            break;
        default:
        {
            LOGGER_ERROR("POST::ERROR::Unkown action type 0x%x for POST message.\n",
                             pPOSTMsg->message.action);
            /*TODO: Return POST fail to BB*/
        }
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : post_task_init
 **
 **    DESCRIPTION     : Initializes the POST task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void post_task_init(void)
{
    /*Creating Semaphore for RX Message Queue*/
    semPOSTMsg = Semaphore_create(0, NULL, NULL);
    if (semPOSTMsg == NULL) {
        LOGGER_ERROR("POST:ERROR::POST RX Semaphore creation failed.\n");
    }
    /*Creating RX Message Queue*/
    postRxMsgQueue = Util_constructQueue(&postRxMsg);
    LOGGER_DEBUG("POST:INFO::Constructing message Queue for 0x%x POST RX Messages.\n",
                     postRxMsgQueue);

    /* Reset POST state to fail */
    postState = 0;
    POST_subSystem = OC_SS_BB;
    /*Ask for activate permission from BB system*/
    OCMPMessageFrame *postFrame = post_create_execute_msg(OC_SS_BB);
    if (postFrame) {
        Util_enqueueMsg(bigBrotherRxMsgQueue, semBigBrotherMsg,
                        (uint8_t*) postFrame);
    }
}

/******************************************************************************
 **    FUNCTION NAME   : post_taskfxn
 **
 **    DESCRIPTION     : Executes POST test for Open cellular.
 **
 **    ARGUMENTS       : a0, a1 - not used
 **
 **    RETURN TYPE     : None
 **
 ******************************************************************************/
static void post_taskfxn(UArg a0, UArg a1)
{
    post_task_init();
    while (true) {
        if (Semaphore_pend(semPOSTMsg, BIOS_WAIT_FOREVER)) {
            while (!Queue_empty(postRxMsgQueue)) {
                OCMPMessageFrame *pWrite = (OCMPMessageFrame *) Util_dequeueMsg(
                        postRxMsgQueue);
                if (pWrite) {
                    post_process_rx_msg(pWrite);
                }
            }
        }
    }
}

/*******************************************************************************
 **    FUNCTION NAME   : post_createtask
 **
 **    DESCRIPTION     : Task creation function for the POST.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *******************************************************************************/
void post_createtask(void)
{
    Task_Params taskParams;

    // Configure task
    Task_Params_init(&taskParams);
    taskParams.stack = postTaskStack;
    taskParams.stackSize = POST_TASK_STACK_SIZE;
    taskParams.priority = OC_POST_TASKPRIORITY;
    Task_construct(&postTask, post_taskfxn, &taskParams, NULL);
}
