/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "include/test_rfwatchdog.h"

extern Fe_gpioCfg fe_gpiocfg;
extern Fe_Watchdog_Cfg fe_watchdog_cfg;

const I2C_Dev I2C_DEV = {
    .bus = OC_CONNECT1_I2C2,
    .slave_addr = RFFE_IO_REVPOWER_ALERT_ADDR,
};
OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};
bool OcGpio_GpioPins[] = {
    [RFWATCHDOG_PIN_1] = OCGPIO_CFG_INPUT,
    [RFWATCHDOG_PIN_2] = OCGPIO_CFG_INPUT,
    [OC_EC_FE_TRXFE_CONN_RESET] = OCGPIO_CFG_INPUT,
};
uint32_t OcGpio_GpioConfig[] = {
    [RFWATCHDOG_PIN_1] = OCGPIO_CFG_INPUT,
    [RFWATCHDOG_PIN_2] = OCGPIO_CFG_INPUT,
    [OC_EC_FE_TRXFE_CONN_RESET] = OCGPIO_CFG_INPUT,
};

OcGpio_Port fe_watchdog_io = {
    .fn_table = &GpioPCA9557_fnTable,
    .cfg =
        &(PCA9557_Cfg){
            .i2c_dev = { OC_CONNECT1_I2C2, RFFE_IO_REVPOWER_ALERT_ADDR },
        },
    .object_data = &(PCA9557_Obj){},
};

Fe_gpioCfg fe_gpiocfg_invalid = {
    /* EC_TRXFECONN_GPIO3/RF_PGOOD_LDO */
    .pin_rf_pgood_ldo = { &ec_io, OC_EC_FE_PWR_GD },
    .pin_fe_12v_ctrl = { &ec_io, OC_EC_FE_CONTROL },
};
// FE watch dog
Fe_Watchdog_Cfg fe_watchdog_invalid = {
    /* CO6_WD */
    .pin_co6_wd = { &fe_watchdog_io, -1 },
    /* CO5_WD INVALID */
    .pin_co5_wd = { &fe_watchdog_io, -1 },
    /* CO4_WD */
    .pin_co4_wd = { &fe_watchdog_io, -1 },
    /* CO3_WD INVALID */
    .pin_co3_wd = { &fe_watchdog_io, -1 },
};

/* FE CH watch dog */
RfWatchdog_Cfg fe_NULL = {
    .pin_alert_lb = NULL,
    .pin_alert_hb = NULL,
    .pin_interrupt = NULL,
};
RfWatchdog_Cfg fe_ch1_invalid_alert_lb = {
    .pin_alert_lb = NULL,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co5_wd,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

RfWatchdog_Cfg fe_ch1_invalid_alert_hb = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co6_wd,
    .pin_alert_hb = NULL,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

RfWatchdog_Cfg fe_ch1_invalid_interrupt = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co6_wd,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co5_wd,
    .pin_interrupt = &fe_gpiocfg_invalid.pin_trxfe_conn_reset,
};
RfWatchdog_Cfg fe_ch2_invalid_alert_lb = {
    .pin_alert_lb = NULL,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co4_wd,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

RfWatchdog_Cfg fe_ch2_invalid_alert_hb = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co3_wd,
    .pin_alert_hb = NULL,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

RfWatchdog_Cfg fe_ch2_invalid_interrupt = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co3_wd,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co4_wd,
    .pin_interrupt = &fe_gpiocfg_invalid.pin_trxfe_conn_reset,
};
