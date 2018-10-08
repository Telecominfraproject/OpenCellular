/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef EBMP_H_
#define EBMP_H_

#include "inc/subsystem/gpp/gpp.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define EBMP_TASK_STACK_SIZE 1024
#define EBMP_TASK_PRIORITY 2

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/*
 * GPP states are define here. Where we define various states GPP or AP can be in.
 * S0_SC[059] and S5[09] are the inputs
 * T0: AP SOC under Reset.       (0,0)
 * T1: AP starts the booting.    (0,0)
 * T2: AP starts DDR init.       (0,1)
 * T3: PCIe and SPC init.           (1,1)
 * T4: Normal Ubuntu Boot: PE2 -> 1 Recovery Boot: PE2 -> 0. (1,0)
 * T5: mSATA boot progress(0,0)
 * T6: OC_Watchdog deamon started successfully.(0,1)
 * T7: OC_Watchdog deamon process responds to EC via OC-Middleware.(1,1)
 */
typedef enum {
    STATE_INVALID = -1,
    STATE_T0 = 0,
    STATE_T1,
    STATE_T2,
    STATE_T3,
    STATE_T4,
    STATE_T5,
    STATE_T6,
    STATE_T7,
    STATE_UPGRADE
} apStates;

typedef enum {
    AP_RESET = 0,
    AP_BOOT_PROGRESS_MONITOR_1 = 1,
    AP_BOOT_PROGRESS_MONITOR_2 = 2
} apBootMonitor;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
void ebmp_init(Gpp_gpioCfg *driver);

#endif /* EBMP_H_ */
