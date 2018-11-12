/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef HCI_H_
#define HCI_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/subsystem/hci/hci_led.h"
#include "inc/subsystem/hci/hci_buzzer.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define HCI_TASK_PRIORITY 6
#define HCI_TASK_STACK_SIZE 4096

#define HCI_LED_TEMP_SENSOR_ADDR 0x1A

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
typedef struct Hci_Cfg {
    HciBuzzer_Cfg buzzer;
    HciLedCfg led;
} Hci_Cfg;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
bool HCI_Init(void *driver, void *return_buf);

#endif /* HCI_H_ */
