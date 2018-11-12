/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef RFFE_POWERMONITOR_H_
#define RFFE_POWERMONITOR_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/Framework.h"
#include "rffe_ctrl.h" /* Temporary, for channel # enum */

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define RFFEPOWERMONITOR_TASK_PRIORITY 2
#define RFFEPOWERMONITOR_TASK_STACK_SIZE 1024

/* RF POWER Detector Device Addresses */
#define RFFE_CHANNEL1_ADC_ADDR 0x4A
#define RFFE_CHANNEL2_ADC_ADDR 0x48

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
typedef enum { RFFE_STAT_FW_POWER = 1, RFFE_STAT_REV_POWER } eRffeStatusParamId;

typedef enum {
    RFFE_STATUS_FW_POWER = 0x01,
    RFFE_STATUS_REV_POWER = 0x02,
    RFFE_STATUS_PARAMS_MAX = 0x04
} eRffeStatusParam;

/* RF Power Detector Control */
typedef enum {
    RFFE_HB_F_POWER = 0,
    RFFE_HB_R_POWER,
    RFFE_LB_F_POWER,
    RFFE_LB_R_POWER
} eRffePowerDetect;

/*
 * RFStatus - Generic Structure for Channel 1 & 2 RF Front end status.
 */
typedef struct __attribute__((packed, aligned(1))) {
    uint16_t fwPower;
    uint16_t revPower;
} rffeStatus;

/*
 * RF_FEStatus_Data - RF FE Status data payload
 */
typedef struct __attribute__((packed, aligned(1))) {
    rffeStatus rffeStatus;
} rffeStatusData;

typedef enum FePowerStatus {
    FE_POWER_STATUS_FORWARD = 0,
    FE_POWER_STATUS_REVERSE,
} FePowerStatus;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
ReturnStatus rffe_powermonitor_read_power(const I2C_Dev *i2c_dev,
                                          eRffeStatusParamId rfPowerSelect,
                                          uint16_t *rfpower);
void rffe_powermonitor_createtask(void);

#endif /* RFFE_POWERMONITOR_H_ */
