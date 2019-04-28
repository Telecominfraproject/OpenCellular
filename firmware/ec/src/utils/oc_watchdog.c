/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "inc/hw_types.h"
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/watchdog.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/Error.h>

#include "inc/utils/oc_watchdog.h"

UInt g_events;
uint32_t g_reqEvents = 0x0000;
static uint8_t intInterval = 1;
static uint32_t testcount = 1;

Event_Handle WD_Event;
Error_Block eb;

#define MAX_WD_TIME_OUT  (0xFFFFFFFF) /*35 seconds*/
#define SET_EVENT(x)    ((1<<(x))| g_events)

Task_Struct wdTask;
Char wdTaskStack[WD_TASK_STACK_SIZE];

Semaphore_Handle semWDRx;

extern ocTask_t ocTask[MAX_TASK_LIMIT];

void WatchdogIntHandler(void)
{
    /* If Watchdog Interrupt is created */
    if(WatchdogIntStatus(WATCHDOG0_BASE, true)) {
        /* Watchdog interval is of 35 seconds * 4 */
        /* For first three intervals (1-2-3)Watchdog will be feed automatically */
        /* For every fourth interval Watchdog will check the status for tasks*/
        if((intInterval%5)==0) {
            /* What events Should I expect.*/
            uint32_t expectedEvents = Util_get_wd_req_evets();
            if(g_events == expectedEvents) {
                /*Feed watchdog*/
                WatchdogIntClear(WATCHDOG0_BASE);
                /*Clear counters*/
                g_events = 0x0000;
                intInterval = 1;
            } else {
                /* Giving soft reset to device.*/
                SysCtlReset();
            }
        } else {
            /*Feed Watchdog without checks*/
            /*This is to give enough free time for EC tasks 4*35 seconds*/
            WatchdogIntClear(WATCHDOG0_BASE);
            intInterval++;
        }
        testcount++;
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : wd_task_init
 **
 **    DESCRIPTION     : Initializes the pre-requisites for Watchdog.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void wd_task_init(void)
{
    /* Create Semaphore for RX Message Queue */
    semWDRx = Semaphore_create(0, NULL, NULL);

    /* using events for intererupt*/
    /* create an Event object. All events are binary */
    WD_Event = Event_create(NULL, &eb);
     if (WD_Event == NULL) {
         LOGGER_DEBUG("WD Event creation failed.");
     }
}

/*****************************************************************************
 **    FUNCTION NAME   : wd_hw_init
 **
 **    DESCRIPTION     : Initializes the hardware for Watchdog.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void wd_hw_init()
{
    /* Enable watchdog 0 */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    /*Enable watchdog interrupt*/
    IntEnable(INT_WATCHDOG);

    /* Watch dog period*/
    WatchdogReloadSet(WATCHDOG0_BASE,MAX_WD_TIME_OUT);

    /*Enable reset generation from the watchdog timer.*/
    WatchdogResetEnable(WATCHDOG0_BASE);

    /* Set debug stall mode */
    WatchdogStallEnable(WATCHDOG0_BASE);

    /*Enable the watchdog timer.*/
    WatchdogEnable(WATCHDOG0_BASE);

    /* Interrupt call back is set in the RTOS CFG. */
}
/*****************************************************************************
 **    FUNCTION NAME   : wd_taskfxn
 **
 **    DESCRIPTION     : handles the health state for all tasks.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void wd_taskfxn(UArg a0, UArg a1)
{
    uint32_t event =0x0000;
    wd_task_init();

    /* Initialize HW for Watchdog */
    wd_hw_init();

    while (TRUE) {
         /* Wait for ALL of the ISR events to be posted */
         /* AND mask is set. It can be altered to OR mask and we can add few more error handling per task.*/
         /*g_events = Event_pend(WD_Event, (Event_Id_00 + Event_Id_01 + Event_Id_02
                          + Event_Id_01 + Event_Id_02 + Event_Id_03 + Event_Id_04 + Event_Id_05
                          + Event_Id_06 + Event_Id_07 + Event_Id_08 + Event_Id_09 + Event_Id_10
                          + Event_Id_11 + Event_Id_12 + Event_Id_13 + Event_Id_14 + Event_Id_15
                          + Event_Id_16 + Event_Id_17 + Event_Id_18 + Event_Id_19 + Event_Id_20
                          + Event_Id_21 + Event_Id_22 + Event_Id_23 + Event_Id_24 + Event_Id_25
                          + Event_Id_26 + Event_Id_27 + Event_Id_28 + Event_Id_29 + Event_Id_30
                          + Event_Id_31 ), Event_Id_NONE, BIOS_WAIT_FOREVER);*/
         event = Event_pend(WD_Event, Event_Id_NONE, (Event_Id_00 + Event_Id_01 + Event_Id_02 + Event_Id_03 + Event_Id_04 + Event_Id_05 + Event_Id_06 + Event_Id_07 + Event_Id_08 + Event_Id_09 + Event_Id_10 + Event_Id_11 + Event_Id_12 + Event_Id_13 + Event_Id_14 + Event_Id_15 + Event_Id_16 + Event_Id_17 + Event_Id_18), BIOS_WAIT_FOREVER);
         {
             /* All the tasks have reported.*/
             g_events |= event;
             LOGGER_DEBUG("WATCHDOG:INFO::WD[%d] Interval %d : Collected events are 0x%08x with recent event 0x%08x.\n",testcount, intInterval, g_events, event);
         }
     }
}

/*****************************************************************************
 **    FUNCTION NAME   : wd_createtask
 **
 **    DESCRIPTION     : Creates task for Watchdog
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void wd_createtask(void)
{
    Task_Params taskParams;
    // Configure task
    Task_Params_init(&taskParams);
    taskParams.instance->name = "Watchdog_t";
    taskParams.stack =wdTaskStack;
    taskParams.stackSize = WD_TASK_STACK_SIZE;
    taskParams.priority = WD_TASK_PRIORITY;
    Util_create_task(&taskParams, &wd_taskfxn, false);
    LOGGER_DEBUG("WATCHDOG:INFO::Creating a Watchdog task.\n");
}

void wd_kick(Task_Handle task)
{
#ifdef OC_Watchdog
    ocTask_t* taskEnv = (ocTask_t*)Task_getEnv(task);
    Event_post(WD_Event, taskEnv->wdEventId);
    LOGGER_DEBUG("WATCHDOG:INFO::Task[%s] Feed Watchdog with event 0x%x.\n",(taskEnv->task_name), (taskEnv->wdEventId));
#endif
}




