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
#include "inc/subsystem/hci/hci_led.h"

#include "inc/common/system_states.h"
#include "inc/devices/led.h"
#include "inc/subsystem/hci/hci.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

extern void *led_hci_ioexp;
#define HCI ((HciLedCfg *)led_hci_ioexp)

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
ledSystemState s_ledState = SYSTEM_BOOT;

/* TODO: Change the following approach for booting up and resetting led counter
 * for synchronization in future if needed */
#define HCI_LED_TASK_PRIORITY 2
#define HCI_LED_TASK_STACK_SIZE 1024

static Char hciLedTaskStack[HCI_LED_TASK_STACK_SIZE];

/*****************************************************************************
 **    FUNCTION NAME   : HCI_LedTaskFxn
 **
 **    DESCRIPTION     : Handles the HCI LED Booting up and reset counter.
 **
 **    ARGUMENTS       : a0, a1 - not used
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void HCI_LedTaskFxn(UArg a0, UArg a1)
{
    while (true) {
        if (s_ledState == SYSTEM_BOOT) {
            //hci_led_system_boot();
        } else if ((s_ledState == SYSTEM_RUNNING) ||
                   (s_ledState == SYSTEM_FAILURE)) {
            OcGpio_write(&HCI->pin_ec_gpio, false);
            Task_sleep(100);
            OcGpio_write(&HCI->pin_ec_gpio, true);
        }
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : HCI_LedTaskInit
 **
 **    DESCRIPTION     : Initializes the HCI LED task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
void HCI_LedTaskInit(void)
{
    /* Create a task */
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = hciLedTaskStack;
    taskParams.stackSize = HCI_LED_TASK_STACK_SIZE;
    taskParams.priority = HCI_LED_TASK_PRIORITY;
    Task_Handle task = Task_create(HCI_LedTaskFxn, &taskParams, NULL);
    if (!task) {
        DEBUG("HCI::FATAL: Unable to start Led Polling task\n");
        return;
    }
}
