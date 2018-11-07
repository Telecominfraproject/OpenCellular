/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef GPP_H_
#define GPP_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/devices/ina226.h"
#include "inc/devices/se98a.h"
#include "inc/common/system_states.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define GPP_TASK_PRIORITY 2
#define GPP_TASK_STACK_SIZE 2048

/*
 * Define all the constant information of GPP subsystem here, like device
 * addresses or Constant configuration values or NUMBER of sensors.
 */
#define GPP_TEMP_SENSOR_NOS 3

/*
 * Device address of GPP sub system are as below.
 */
#define GPP_AP_TEMPSENS1_ADDR 0x1A
#define GPP_AP_TEMPSENS2_ADDR 0x1D
#define GPP_AP_TEMPSENS3_ADDR 0x1C
#define GPP_AP_CURRENT_SENSOR_ADDR 0x44
#define GPP_MSATA_CURRENT_SENSOR_ADDR 0x45

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
typedef struct AP_Cfg {
    INA226_Dev current_sensor;
    SE98A_Dev temp_sensor[GPP_TEMP_SENSOR_NOS];
} AP_Cfg;

typedef struct mSATA_Cfg {
    INA226_Dev current_sensor;
} mSATA_Cfg;

typedef struct Gpp_gpioCfg {
    OcGpio_Pin pin_soc_pltrst_n;
    OcGpio_Pin pin_soc_corepwr_ok;
    OcGpio_Pin pin_msata_ec_das;
    OcGpio_Pin pin_lt4256_ec_pwrgd;
    OcGpio_Pin pin_ap_12v_onoff;
    OcGpio_Pin pin_ec_reset_to_proc;
    OcGpio_Pin pin_ap_boot_alert1;
    OcGpio_Pin pin_ap_boot_alert2;
} Gpp_gpioCfg;

bool gpp_pre_init(void *driver, void *returnValue);
bool gpp_post_init(void *driver, void *returnValue);
bool GPP_ap_Reset(void *driver, void *params);

#endif /* GPP_H_ */
