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
#include "inc/subsystem/hci/hci.h"

#include "registry/SSRegistry.h"

/*****************************************************************************
 *                             HANDLES DEFINITION
 *****************************************************************************/
/* Global Task Configuration Variables */
static Char hciTaskStack[HCI_TASK_STACK_SIZE];

OCSubsystem ssHci = {
    .taskStackSize = HCI_TASK_STACK_SIZE,
    .taskPriority = HCI_TASK_PRIORITY,
    .taskStack = hciTaskStack,
};

extern void *sys_config[];
#define HCI ((Hci_Cfg *)sys_config[OC_SS_HCI])

bool HCI_Init(void *return_buf)
{
    /* Initialize IO pins */
    OcGpio_configure(&HCI->led.pin_ec_gpio, OCGPIO_CFG_OUTPUT |
                                            OCGPIO_CFG_OUT_HIGH);

    HciBuzzer_init();

    return true;
}

