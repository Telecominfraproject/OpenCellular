/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef SYNC_H_
#define SYNC_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/devices/adt7481.h"
#include "inc/devices/sx1509.h" /* Just for POST check - remove for 0.2 */
#include "drivers/OcGpio.h"

#include <stdbool.h>

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define SYNC_TASK_PRIORITY                  2
#define SYNC_TASK_STACK_SIZE                1024

/* Temporary fix */
#define SYNC_GPS_TASK_PRIORITY              2
#define SYNC_GPS_TASK_STACK_SIZE            1024

#define SYNC_IO_DEVICE_ADDR                 0x71
#define SYNC_TEMP_SENSOR_ADDR               0x4C

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
typedef struct Sync_Cfg {
    const I2C_Dev temp_sens;
    const I2C_Dev io_exp; /* Temporary - for POST */

    /* TODO: this pins are just lumped together, they'll be broken into their
     * respective drivers in time */
    OcGpio_Pin pin_spdt_cntrl_lvl;
    OcGpio_Pin pin_warmup_survey_init_sel;
    OcGpio_Pin pin_r_phase_lock_ioexp;
    OcGpio_Pin pin_r_lock_ok_ioexp;
    OcGpio_Pin pin_r_alarm_ioexp;
    OcGpio_Pin pin_12v_reg_enb;
    OcGpio_Pin pin_temp_alert;
    OcGpio_Pin pin_spdt_cntrl_lte_cpu_gps_lvl;
    OcGpio_Pin pin_init_survey_sel;

    OcGpio_Pin pin_ec_sync_reset;
} Sync_Cfg;

typedef enum gpsStatus {
    GPS_NOTLOCKED = 0,
    GPS_LOCKED
} gpsStatus;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
/* Schema hooks */
bool SYNC_Init(void *return_buf);
bool SYNC_reset(void *driver, void *params);
bool SYNC_GpsStatus(void *driver, unsigned int param_id, void *return_buf);

#endif /* SYNC_H_ */
