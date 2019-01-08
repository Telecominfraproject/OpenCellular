/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef POWERSOURCE_H_
#define POWERSOURCE_H_

#include "common/inc/global/post_frame.h"
#include "common/inc/global/ocmp_frame.h"
#include "common/inc/global/Framework.h"
#include "drivers/OcGpio.h"
#include "inc/common/global_header.h"
#include "inc/common/i2cbus.h"

#include <ti/sysbios/gates/GateMutex.h>

typedef enum {
    PWR_SRC_EXT = 0,
    PWR_SRC_POE,
    PWR_SRC_LIION_BATT,
    PWR_SRC_MAX
} ePowerSource;

typedef enum {
    PWR_SRC_ACTIVE = 0,     /* If source is primary source */
    PWR_SRC_AVAILABLE,      /* If source is available */
    PWR_SRC_NON_AVAILABLE   /* If source is not connected */
} ePowerSourceState;

typedef struct {
    ePowerSource powerSource;
    ePowerSourceState state;
} tPowerSource;

typedef struct PWRSRC_Cfg {
    OcGpio_Pin        pin_dc_present;
    OcGpio_Pin        pin_poe_prsnt_n;
    OcGpio_Pin        pin_int_bat_prsnt;
    OcGpio_Pin        pin_disable_dc_input;
    OcGpio_Pin        pin_dc_input_fault;
    OcGpio_Pin        pin_oc_input_present;
    OcGpio_Pin        pin_power_off;
} PWRSRC_Cfg;

typedef struct PWRSRC_Cfg_Obj {
    GateMutex_Handle mutex;
} PWRSRC_Obj;

typedef struct PWRSRC_Dev {
    const PWRSRC_Cfg cfg;
    PWRSRC_Obj obj;
} PWRSRC_Dev;

typedef enum {
    PWR_STAT_EXT_PWR_AVAILABILITY,
    PWR_STAT_EXT_PWR_ACTIVE,
    PWR_STAT_POE_AVAILABILITY,
    PWR_STAT_POE_ACTIVE,
    PWR_STAT_BATT_AVAILABILITY,
    PWR_STAT_BATT_ACTIVE
} ePower_StatusParamId;

void pwr_source_init(PWRSRC_Dev *dev, void *alert_token);
void pwr_get_source_info(PWRSRC_Dev *pwrSrcDev);

#endif /* POWERSOURCE_H_ */
