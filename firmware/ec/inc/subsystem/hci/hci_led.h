/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef HCI_LED_H_
#define HCI_LED_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/ocmp_frame.h"
#include "inc/common/global_header.h"
#include "inc/devices/se98a.h"
#include "inc/devices/sx1509.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define LED_SX1509_LEFT_ADDRESS 0x3E
#define LED_SX1509_RIGHT_ADDRESS 0x3F

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
typedef enum {
    HCI_LED_DRIVER_LEFT = 0,
    HCI_LED_DRIVER_RIGHT = 1,

    HCI_LED_DRIVER_COUNT
} HciLed_DriverId;

/* Subsystem config */
typedef struct HciLedCfg {
    I2C_Dev sx1509_dev[HCI_LED_DRIVER_COUNT];
    OcGpio_Pin pin_ec_gpio;
} HciLedCfg;

/* LED System States */
typedef enum {
    SYSTEM_INIT = 0,
    SYSTEM_BOOT,
    SYSTEM_RUNNING,
    SYSTEM_FAILURE,
    RADIO_FAILURE,
    BACKHAUL_FAILURE
} ledSystemState;

typedef struct {
    ledSystemState ledSystemState;
} ledCtrlMsgData;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
void HCI_LedTaskInit(void);

#endif /* HCI_LED_H_ */
