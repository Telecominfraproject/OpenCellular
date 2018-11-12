/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef BMS_H_
#define BMS_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/devices/ina226.h"
#include "inc/devices/se98a.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define BMS_TASK_PRIORITY 2
#define BMS_TASK_STACK_SIZE 2048

/*
 * Define all the constant information of BMS subsystem here, like device
 * addresses or Constant configuration values or NUMBER of sensors
 */
#define BMS_EC_TEMP_SENSOR_ADDR 0x19
#define BMS_EC_CURRENT_SENSOR_12V_ADDR 0x40
#define BMS_EC_CURRENT_SENSOR_3P3V_ADDR 0x45

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
typedef struct Bms_Cfg {
    INA226_Dev ec_current_sensor_12v;
    INA226_Dev ec_current_sensor_3p3v;
    SE98A_Dev ec_temp_sensor;
} Bms_Cfg;

#endif /* BMS_H_ */
