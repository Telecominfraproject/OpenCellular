/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/global/OC_CONNECT1.h"
#include "common/inc/ocmp_wrappers/ocmp_adt7481.h"
#include "inc/subsystem/obc/obc.h"
#include "inc/subsystem/sync/sync.h"
#include "inc/devices/debug_ocgpio.h"

SCHEMA_IMPORT OcGpio_Port ec_io;
SCHEMA_IMPORT OcGpio_Port sync_io;
/*****************************************************************************
 *                               SYSTEM CONFIG
 *****************************************************************************/
/* OBC Subsystem Config.*/
//Irridium
Iridium_Cfg obc_irridium = {
    .uart = OC_CONNECT1_UARTXR0,
    /* IRIDIUM_RSTIOEXP */
    .pin_enable = { &sync_io, 2, OCGPIO_CFG_OUT_STD },
    /* R_NW_AVAIL */
    .pin_nw_avail = { &sync_io, 3, OCGPIO_CFG_IN_PU },
};

/* Sync Subsystem Config.*/
//Temperature sensor.
I2C_Dev sync_gps_ts = {
    .bus = OC_CONNECT1_I2C7,
    .slave_addr = SYNC_TEMP_SENSOR_ADDR,
};

I2C_Dev sync_gps_io = {
    .bus = OC_CONNECT1_I2C7,
    .slave_addr = SYNC_IO_DEVICE_ADDR,
};

// SYNC IO EXPANDERS
S_OCGPIO_Cfg debug_sync_ioexpanderx71 = {
    .port = &sync_io,
};

//Sync Factory config
const ADT7481_Config fact_sync_ts_cfg = {
    .lowlimit = -20,
    .highlimit = 80,
    .critlimit = 85,
};

Sync_gpioCfg sync_gpiocfg = (Sync_gpioCfg){
    /* SPDT_CNTRL_LVL */
    .pin_spdt_cntrl_lvl = { &sync_io, 0, OCGPIO_CFG_OUT_OD_NOPULL },
    /* WARMUP_SURVEY_INIT_SEL */
    .pin_warmup_survey_init_sel = { &sync_io, 1, OCGPIO_CFG_OUT_OD_NOPULL },
    /* R_PHASE_LOCK_IOEXP */
    .pin_r_phase_lock_ioexp = { &sync_io, 4, OCGPIO_CFG_IN_PU },
    /* R_LOCK_OK_IOEXP */
    .pin_r_lock_ok_ioexp = { &sync_io, 5, OCGPIO_CFG_IN_PU },
    /* R_ALARM_IOEXP */
    .pin_r_alarm_ioexp = { &sync_io, 6, OCGPIO_CFG_IN_PU },
    /* 12V_REG_ENB */
    .pin_12v_reg_enb = { &sync_io, 7, OCGPIO_CFG_OUT_STD },
    /* TEMP_ALERT */
    .pin_temp_alert = { &sync_io, 8, OCGPIO_CFG_IN_PU },
    /* SPDT_CNTRL_LTE_CPU_GPS_LVL */
    .pin_spdt_cntrl_lte_cpu_gps_lvl = { &sync_io, 9, OCGPIO_CFG_OUT_OD_NOPULL },
    /* INIT_SURVEY_SEL */
    .pin_init_survey_sel = { &sync_io, 10, OCGPIO_CFG_OUT_OD_NOPULL },
    /* EC_SYNC_RESET */
    .pin_ec_sync_reset = { &ec_io, OC_EC_SYNC_RESET },
};

Obc_gpioCfg sync_obc_gpiocfg = {
    /* 12V_REG_ENB */
    .pin_pwr_en = &(OcGpio_Pin){ &sync_io, 7, OCGPIO_CFG_OUT_STD },
};