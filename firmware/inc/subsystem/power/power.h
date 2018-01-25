/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef POWER_H_
#define POWER_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/devices/ltc4015.h"
#include "inc/devices/ltc4274.h"
#include "inc/devices/ltc4275.h"
#include "inc/devices/powerSource.h"
#include "inc/devices/se98a.h"

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
typedef struct Power_Cfg {
    OcGpio_Pin pin_ec_pd_pwrgd_ok;
    OcGpio_Pin pin_solar_aux_prsnt_n;
    OcGpio_Pin pin_poe_prsnt_n;
    OcGpio_Pin pin_lt4275_ec_nt2p;
    OcGpio_Pin pin_necpse_rst;
    OcGpio_Pin pin_lt4015_i2c_sel;
    OcGpio_Pin pin_int_bat_prsnt;
    OcGpio_Pin pin_ext_bat_prsnt;
    SE98A_Dev lead_acid_temp_sens;
    LTC4015_Dev ext_bat_charger;
    LTC4015_Dev int_bat_charger;
    LTC4274_Dev pse;
    LTC4275_Dev pd;
    PWRSRC_Dev powerSource;
} Power_Cfg;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
bool pwr_pre_init();
bool pwr_post_init();

#endif
