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
    PWR_SRC_AUX_OR_SOLAR = 0,
    PWR_SRC_POE,
    PWR_SRC_LEAD_ACID_BATT,
    PWR_SRC_LIION_BATT,
    PWR_SRC_MAX
} ePowerSource;

typedef enum {
    PWR_SRC_ACTIVE = 0, /* If source is primary source */
    PWR_SRC_AVAILABLE, /* If source is available */
    PWR_SRC_NON_AVAILABLE /* If source is not connected */
} ePowerSourceState;

typedef enum {
    PWR_STAT_POE_AVAILABILITY = 0x00,
    PWR_STAT_POE_ACCESSIBILITY = 0x01,
    PWR_STAT_SOLAR_AVAILABILITY = 0x02,
    PWR_STAT_SOLAR_ACCESSIBILITY = 0x03,
    PWR_STAT_EXTBATT_AVAILABILITY = 0x04,
    PWR_STAT_EXTBATT_ACCESSIBILITY = 0x05,
    PWR_STAT_INTBATT_AVAILABILITY = 0x06,
    PWR_STAT_INTBATT_ACCESSIBILITY = 0x07
} ePower_StatusParamId;

typedef enum {
    PWR_STATUS_POE_AVAILABILITY = 0x01,
    PWR_STATUS_POE_ACCESSIBILITY = 0x02,
    PWR_STATUS_SOLAR_AVAILABILITY = 0x04,
    PWR_STATUS_SOLAR_ACCESSIBILITY = 0x08,
    PWR_STATUS_EXTBATT_AVAILABILITY = 0x10,
    PWR_STATUS_EXTBATT_ACCESSIBILITY = 0x20,
    PWR_STATUS_INTBATT_AVAILABILITY = 0x40,
    PWR_STATUS_INTBATT_ACCESSIBILITY = 0x80,
    PWR_STATUS_PARAM_MAX = 0x100
} ePower_StatusParam;

typedef struct {
    ePowerSource powerSource;
    ePowerSourceState state;
} tPowerSource;

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t poeAvail;
    uint8_t poeAccess;
    uint8_t solarAvail;
    uint8_t solarAccess;
    uint8_t extBattAvail;
    uint8_t extBattAccess;
    uint8_t intBattAvail;
    uint8_t intBattAccess;
} tPower_Status_Data;

typedef struct PWRSRC_Cfg {
    OcGpio_Pin pin_solar_aux_prsnt_n;
    OcGpio_Pin pin_poe_prsnt_n;
    OcGpio_Pin pin_int_bat_prsnt;
    OcGpio_Pin pin_ext_bat_prsnt;
} PWRSRC_Cfg;

typedef struct PWRSRC_Cfg_Obj {
    GateMutex_Handle mutex;
} PWRSRC_Obj;

typedef struct PWRSRC_Dev {
    const PWRSRC_Cfg cfg;
    PWRSRC_Obj obj;
} PWRSRC_Dev;

void pwr_source_init(void);
void pwr_get_source_info(PWRSRC_Dev *pwrSrcDev);
ReturnStatus
pwr_process_get_status_parameters_data(ePower_StatusParamId paramIndex,
                                       uint8_t *pPowerStatusData);

#endif /* POWERSOURCE_H_ */
