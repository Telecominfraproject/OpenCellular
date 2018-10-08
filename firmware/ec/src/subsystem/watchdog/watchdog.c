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
#include "inc/subsystem/watchdog/watchdog.h"

#include "common/inc/global/ocmp_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/bigbrother.h"
#include "inc/common/global_header.h"
#include "inc/subsystem/gpp/gpp.h"
#include "inc/utils/util.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdlib.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
/* Global Task Configuration Variables */
static Task_Struct WatchdogTask;
static Char WatchdogTaskStack[WATCHDOG_TASK_STACK_SIZE];

/* Queue Handles */
Semaphore_Handle watchdogSem;
Queue_Handle watchdogMsgQueue;

/* Clock Handle */
Clock_Handle watchdog;

/*
 * localTimerRequired - This is the variable used when any other process
 * wants to use the time keeping functionality. Why to use separate
 * clock module when we have this one.
 */
uint8_t localTimerRequired = 0;
uint32_t localCounter = 0;
uint16_t reinterationTime = 2;

/*
 * apUp - This variable will be set based on AP's boot progress pins are
 * set after watchdog process has been initiated in AP. Zero only when AP
 * is either in shutdown or under reset or booting up, or made to restart by EC
 */
uint8_t apUp = 1;

/*
 * watchdogCmdReceived - This variable will b set when Watchdog will receive
 * the command from the AP. As soon as it is found set then, within next clk
 * function call, it will be cleared and only set when next command will be
 * received.
 */
uint32_t watchdogCmdReceived = 0;

/* timeup - In seconds */
uint32_t timeup = 60;

extern const void *sys_config[];

/*******************************************************************************
 * watchdog_reset_ap  : Command the GPP task to reset AP
 ******************************************************************************/
void watchdog_reset_ap(void)
{
    uint32_t delay = 0;
    const Gpp_gpioCfg *cfg = sys_config[OC_SS_GPP];

    OcGpio_write(&cfg->pin_ec_reset_to_proc, false);
    delay = 0x1000000;
    while (delay--)
        ;

    OcGpio_write(&cfg->pin_ec_reset_to_proc, true);

    //  OCMPMessageFrame * pWatchdogMsg = (OCMPMessageFrame *) malloc(sizeof(32));
    /* For now only AP reset is being applied directly to see the effect*/
    return;
}

/*******************************************************************************
 * watchdog_send_messages  : Processes the watchdog TX Messages
 ******************************************************************************/
void watchdog_send_messages(OCMPMessageFrame *pWatchdogMsg)
{
    if (pWatchdogMsg != NULL) {
        Util_enqueueMsg(bigBrotherTxMsgQueue, semBigBrotherMsg,
                        (uint8_t *)pWatchdogMsg);
    } else {
        LOGGER_DEBUG("WATCHDOG:ERROR:: pointer NULL!!??");
    }
}

/*****************************************************************************
 * watchdog_send_cmd_message: fill the msg structure and send it over UART
 ****************************************************************************/
void watchdog_send_cmd_message(void)
{
    OCMPMessageFrame *pWatchdogMsg = (OCMPMessageFrame *)malloc(32);
    pWatchdogMsg->header.ocmpInterface = OCMP_COMM_IFACE_UART;
    pWatchdogMsg->header.ocmpSof = OCMP_MSG_SOF;
    pWatchdogMsg->message.subsystem = OC_SS_WD;
    pWatchdogMsg->message.msgtype = OCMP_MSG_TYPE_STATUS;
    pWatchdogMsg->message.action = OCMP_AXN_TYPE_SET;
    watchdog_send_messages(pWatchdogMsg);
}

/*****************************************************************************
 * watchdog_call : This clock function will be called every 1 sec. It checks
 *                 if watchdog command is received or not? If not then it resets
 *                 the AP or send command to BMS to reset the AP
 *****************************************************************************/
Void watchdog_call(UArg arg0)
{
    if (apUp && (localCounter < timeup)) {
        /* Watchdog command is recieved properly */
        if (watchdogCmdReceived) {
            localCounter = 0;
            watchdogCmdReceived = 0;
        } else {
            localCounter += 1;
            if ((localCounter % reinterationTime) == 0)
                Semaphore_post(watchdogSem);
            //              send_wdt_cmd();
        }
    } else {
        apUp = localCounter = 0;
        /* Reset the AP as it is hanged */
        //      reset_ap();
    }
}

/*****************************************************************************
 * watchdog_process_msg : Sets the watchdog_cmd_received variable. So next clock
 *                        tick at 10msec will be able to reset the local_counter
 *                        and respond back with "reply".
 *****************************************************************************/
void watchdog_process_msg(OCMPMessageFrame *pWatchdogMsg)
{
    if (pWatchdogMsg->message.msgtype == OCMP_MSG_TYPE_CONFIG) {
        //set_config_watchdog(pWatchdogMsg);
        pWatchdogMsg->message.action = OCMP_AXN_TYPE_REPLY;
        watchdog_send_messages(pWatchdogMsg);
    } else if (pWatchdogMsg->message.msgtype == OCMP_MSG_TYPE_STATUS) {
        watchdogCmdReceived = 1;
        free((uint8_t *)pWatchdogMsg);
    }
}

/*****************************************************************************
 * watchdog_task_init : Function initializes the IPC objects for Watchdog task
 *****************************************************************************/
void watchdog_task_init(void)
{
    /* Create Semaphore for RX Watchdog Message Queue */
    watchdogSem = Semaphore_create(0, NULL, NULL);
    if (watchdogSem == NULL)
        LOGGER_DEBUG(
                "WATCHDOG:ERROR:: Failed in Creating Watchdog Semaphore.\n");

    /* Create Wathcdog control Queue used by Big brother */
    watchdogMsgQueue = Queue_create(NULL, NULL);
    if (watchdogMsgQueue == NULL)
        LOGGER_DEBUG(
                "WATCHDOG:ERROR:: Failed in Constructing Watchdog Message Queue.\n");
}

/*****************************************************************************
 * watchdog_task_fxn : Handles Watchdog Module
 *****************************************************************************/
void watchdog_task_fxn(UArg a0, UArg a1)
{
    watchdog_task_init();

    //  Clock_start(watchdog);

    while (1) {
        if (Semaphore_pend(watchdogSem, BIOS_WAIT_FOREVER)) {
            if (!Queue_empty(watchdogMsgQueue)) {
                OCMPMessageFrame *pWatchdogMsg =
                        (OCMPMessageFrame *)Util_dequeueMsg(watchdogMsgQueue);

                if (pWatchdogMsg) {
                    watchdog_process_msg(pWatchdogMsg);
                }
            } else {
                watchdog_send_cmd_message();
            }
        }
    }
}

/*****************************************************************************
 * watchdog_create_task : Creates task for Watchdog
 *****************************************************************************/
void watchdog_create_task(void)
{
    Task_Params taskParams;

    Task_Params_init(&taskParams);
    taskParams.stack = WatchdogTaskStack;
    taskParams.stackSize = WATCHDOG_TASK_STACK_SIZE;
    taskParams.priority = WATCHDOG_TASK_PRIORITY;

    Task_construct(&WatchdogTask, watchdog_task_fxn, &taskParams, NULL);

    LOGGER_DEBUG("WATCHDOG:INFO:: Creating Watchdog Task.\n");
}
