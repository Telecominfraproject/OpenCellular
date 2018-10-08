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
#include "common/inc/ocmp_wrappers/ocmp_dat-xxr5a-pp.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "inc/subsystem/rffe/rffe_powermonitor.h"
#include "inc/subsystem/rffe/rffe_sensor.h"
#include "inc/subsystem/testModule/testModule.h"
#include "inc/devices/debug_ocgpio.h"
#include "inc/devices/dat-xxr5a-pp.h"

extern Fe_Cfg g_fe_cfg;
SCHEMA_IMPORT OcGpio_Port ec_io;
SCHEMA_IMPORT OcGpio_Port gbc_io_1;
SCHEMA_IMPORT OcGpio_Port fe_ch1_gain_io;
SCHEMA_IMPORT OcGpio_Port fe_ch1_lna_io;
SCHEMA_IMPORT OcGpio_Port fe_ch2_gain_io;
SCHEMA_IMPORT OcGpio_Port fe_ch2_lna_io;
SCHEMA_IMPORT OcGpio_Port fe_watchdog_io;

/*****************************************************************************
 *                               EEPROM CONFIG
 *****************************************************************************/
Eeprom_Cfg eeprom_fe_inv = {
    .i2c_dev = { OC_CONNECT1_I2C4, 0x50 },
    .pin_wp = &(OcGpio_Pin){ &fe_ch1_lna_io, 0 },
    .type = CAT24C256,
    .ss = OC_SS_RF,
};
/*****************************************************************************
 *                               SYSTEM CONFIG
 *****************************************************************************/
/* RFFE Subsystem Config.*/
// FE Channel 1 Power sensor.
INA226_Dev fe_ch1_ps_5_7v = {
    /* CH1 5.7V Sensor */
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C4,
                                    .slave_addr = RFFE_INA226_CH1_5_7V_ADDR,
                            },
                    .pin_alert =
                            &(OcGpio_Pin){ &ec_io, OC_EC_RFFE_TEMP_INA_ALERT },
            },
};

//FE Channel 2 Power sensor.
INA226_Dev fe_ch2_ps_5_7v = {
    /* CH2 5.7V Sensor */
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C4,
                                    .slave_addr = RFFE_INA226_CH2_5_7V_ADDR,
                            },
                    .pin_alert =
                            &(OcGpio_Pin){ &ec_io, OC_EC_RFFE_TEMP_INA_ALERT },
            },
};

//FE Channel 1 temperature sensor.
I2C_Dev fe_ch1_ts = {
    .bus = OC_CONNECT1_I2C4,
    .slave_addr = RFFE_CH1_TEMP_SENSOR_ADDR,
};

//FE Channel 2 temperature sensor.
I2C_Dev fe_ch2_ts = (I2C_Dev){
    .bus = OC_CONNECT1_I2C4,
    .slave_addr = RFFE_CH2_TEMP_SENSOR_ADDR,
};

//FE EEPROM inventory
void *fe_eeprom_inventory = &eeprom_fe_inv;

//FE Channel 1 ADC
I2C_Dev fe_ch1_ads7830 = {
    .bus = OC_CONNECT1_I2C4,
    .slave_addr = RFFE_CHANNEL1_ADC_ADDR,
};

//FE Channel 2 ADC
I2C_Dev fe_ch2_ads7830 = {
    .bus = OC_CONNECT1_I2C4,
    .slave_addr = RFFE_CHANNEL2_ADC_ADDR,
};

Fe_Gain_Cfg fe_ch1_gain = {
    /* CH1_TX_ATTN_16DB */
    .pin_tx_attn_16db = { &fe_ch1_gain_io, 1 },
    /* CH1_TX_ATTN_P5DB */
    .pin_tx_attn_p5db = { &fe_ch1_gain_io, 2 },
    /* CH1_TX_ATTN_1DB */
    .pin_tx_attn_1db = { &fe_ch1_gain_io, 3 },
    /* CH1_TX_ATTN_2DB */
    .pin_tx_attn_2db = { &fe_ch1_gain_io, 4 },
    /* CH1_TX_ATTN_4DB */
    .pin_tx_attn_4db = { &fe_ch1_gain_io, 5 },
    /* CH1_TX_ATTN_8DB */
    .pin_tx_attn_8db = { &fe_ch1_gain_io, 6 },
    /* CH1_TX_ATTN_ENB */
    .pin_tx_attn_enb = { &fe_ch1_gain_io, 7 },
};

Fe_Gain_Cfg fe_ch2_gain = {
    /* CH2_TX_ATTN_16DB */
    .pin_tx_attn_16db = { &fe_ch2_gain_io, 1 },
    /* CH2_TX_ATTN_P5DB */
    .pin_tx_attn_p5db = { &fe_ch2_gain_io, 2 },
    /* CH2_TX_ATTN_1DB */
    .pin_tx_attn_1db = { &fe_ch2_gain_io, 3 },
    /* CH2_TX_ATTN_2DB */
    .pin_tx_attn_2db = { &fe_ch2_gain_io, 4 },
    /* CH2_TX_ATTN_4DB */
    .pin_tx_attn_4db = { &fe_ch2_gain_io, 5 },
    /* CH2_TX_ATTN_8DB */
    .pin_tx_attn_8db = { &fe_ch2_gain_io, 6 },
    /* CH2_TX_ATTN_ENB */
    .pin_tx_attn_enb = { &fe_ch2_gain_io, 7 },
};

Fe_Lna_Cfg fe_ch1_lna = {
    /* CH1_RX_ATTN_P5DB */
    .pin_rx_attn_p5db = { &fe_ch1_lna_io, 2 },
    /* CH1_RX_ATTN_1DB */
    .pin_rx_attn_1db = { &fe_ch1_lna_io, 3 },
    /* CH1_RX_ATTN_2DB */
    .pin_rx_attn_2db = { &fe_ch1_lna_io, 4 },
    /* CH1_RX_ATTN_4DB */
    .pin_rx_attn_4db = { &fe_ch1_lna_io, 5 },
    /* CH1_RX_ATTN_8DB */
    .pin_rx_attn_8db = { &fe_ch1_lna_io, 6 },
    /* CH1_RX_ATTN_ENB */
    .pin_rx_attn_enb = { &fe_ch1_lna_io, 7 },
};

Fe_Lna_Cfg fe_ch2_lna = {
    /* CH2_RX_ATTN_P5DB */
    .pin_rx_attn_p5db = { &fe_ch2_lna_io, 2 },
    /* CH2_RX_ATTN_1DB */
    .pin_rx_attn_1db = { &fe_ch2_lna_io, 3 },
    /* CH2_RX_ATTN_2DB */
    .pin_rx_attn_2db = { &fe_ch2_lna_io, 4 },
    /* CH2_RX_ATTN_4DB */
    .pin_rx_attn_4db = { &fe_ch2_lna_io, 5 },
    /* CH2_RX_ATTN_8DB */
    .pin_rx_attn_8db = { &fe_ch2_lna_io, 6 },
    /* CH2_RX_ATTN_ENB */
    .pin_rx_attn_enb = { &fe_ch2_lna_io, 7 },
};

//FE watch dog
Fe_Watchdog_Cfg fe_watchdog_cfg = {
    /* AOSEL_FPGA */
    .pin_aosel_fpga = { &fe_watchdog_io, 0 },
    /* CH2_RF_PWR_OFF */
    .pin_ch2_rf_pwr_off = { &fe_watchdog_io, 1 },
    /* CO6_WD */
    .pin_co6_wd = { &fe_watchdog_io, 2 },
    /* CO5_WD */
    .pin_co5_wd = { &fe_watchdog_io, 3 },
    /* CO4_WD */
    .pin_co4_wd = { &fe_watchdog_io, 4 },
    /* CO3_WD */
    .pin_co3_wd = { &fe_watchdog_io, 5 },
    /* CO2_WD */
    .pin_co2_wd = { &fe_watchdog_io, 6 },
    /* COPOL_FPGA */
    .pin_copol_fpga = { &fe_watchdog_io, 7 },
};

Fe_gpioCfg fe_gpiocfg = {
    /* EC_TRXFECONN_GPIO3/RF_PGOOD_LDO */
    .pin_rf_pgood_ldo = { &ec_io, OC_EC_FE_PWR_GD },
    /* FE_12V_CTRL */
    .pin_fe_12v_ctrl = { &ec_io, OC_EC_FE_CONTROL },
    .pin_trxfe_conn_reset = { &ec_io, OC_EC_FE_TRXFE_CONN_RESET },
};

//FE Ch1 TX Gain control
Fe_Ch1_Gain_Cfg fe_ch1_tx_gain_cfg = (Fe_Ch1_Gain_Cfg){
    .fe_gain_cfg = &fe_ch1_gain,
};

//FE Ch2 TX Gain control
Fe_Ch2_Gain_Cfg fe_ch2_tx_gain_cfg = (Fe_Ch2_Gain_Cfg){
    /* CH1_2G_LB_BAND_SEL_L */
    .pin_ch1_2g_lb_band_sel_l = { &fe_ch2_gain_io, 0 },
    .fe_gain_cfg = &fe_ch2_gain,
};

//FE Ch1 LNA config
Fe_Ch1_Lna_Cfg fe_ch1_rx_gain_cfg = (Fe_Ch1_Lna_Cfg){
    .fe_lna_cfg = &fe_ch1_lna,
};

//FE Ch2 LNA config
Fe_Ch2_Lna_Cfg fe_ch2_rx_gain_cfg = (Fe_Ch2_Lna_Cfg){
    /* CH1_RF_PWR_OFF */
    .pin_ch1_rf_pwr_off = { &fe_ch2_lna_io, 1 },
    .fe_lna_cfg = &fe_ch2_lna,
};

/* FE CH watch dog */
RfWatchdog_Cfg fe_ch1_watchdog = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co6_wd,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co5_wd,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

/* FE CH watch dog */
RfWatchdog_Cfg fe_ch2_watchdog = {
    .pin_alert_lb = &fe_watchdog_cfg.pin_co3_wd,
    .pin_alert_hb = &fe_watchdog_cfg.pin_co4_wd,
    .pin_interrupt = &fe_gpiocfg.pin_trxfe_conn_reset,
};

/* FE GPIO's */
Fe_Cfg fe_rffecfg = {
    .fe_gpio_cfg = &fe_gpiocfg,
    .fe_ch1_gain_cfg = (Fe_Ch1_Gain_Cfg *)&fe_ch1_tx_gain_cfg,
    .fe_ch2_gain_cfg = (Fe_Ch2_Gain_Cfg *)&fe_ch2_tx_gain_cfg,
    .fe_ch1_lna_cfg = (Fe_Ch1_Lna_Cfg *)&fe_ch1_rx_gain_cfg,
    .fe_ch2_lna_cfg = (Fe_Ch2_Lna_Cfg *)&fe_ch2_rx_gain_cfg,
    .fe_watchdog_cfg = (Fe_Watchdog_Cfg *)&fe_watchdog_cfg,
};

FE_Ch_Band_cfg fe_ch1_bandcfg = {
    .channel = RFFE_CHANNEL1,
};

FE_Ch_Band_cfg fe_ch2_bandcfg = {
    .channel = RFFE_CHANNEL2,
};

Fe_Ch_Pwr_Cfg fe_ch1_pwrcfg = { .channel = RFFE_CHANNEL1,
                                .fe_Rffecfg = (Fe_Cfg *)&fe_rffecfg };

Fe_Ch_Pwr_Cfg fe_ch2_pwrcfg = { .channel = RFFE_CHANNEL2,
                                .fe_Rffecfg = (Fe_Cfg *)&fe_rffecfg };

// TestModule
TestMod_Cfg testModuleCfg = (TestMod_Cfg){
    .g510_cfg =
            {
                    .uart = OC_CONNECT1_UART4,
                    /* 2G_SIM_PRESENCE */
                    .pin_sim_present = { &gbc_io_1, 0, OCGPIO_CFG_IN_PU },

                    /* NOTE: enable & power go through MOSFETs, inverting them */
                    /* 2GMODULE_POWEROFF  */
                    .pin_enable = { &gbc_io_1, 2, OCGPIO_CFG_INVERT },
                    /* EC_2GMODULE_PWR_ON  */
                    .pin_pwr_en = { &gbc_io_1, 1, OCGPIO_CFG_INVERT },
            },
    .pin_ant_sw = {},
};

// RFFE IO EXPANDERS
S_OCGPIO_Cfg debug_fe_ioexpanderx18 = {
    .port = &fe_ch1_gain_io,
};

S_OCGPIO_Cfg debug_fe_ioexpanderx1C = {
    .port = &fe_ch2_gain_io,
};

S_OCGPIO_Cfg debug_fe_ioexpanderx1B = {
    .port = &fe_watchdog_io,
};

S_OCGPIO_Cfg debug_fe_ioexpanderx1A = {
    .port = &fe_ch1_lna_io,
};

S_OCGPIO_Cfg debug_fe_ioexpanderx1D = {
    .port = &fe_ch2_lna_io,
};

//FE  Factory config
const ADT7481_Config fact_fe_ch1_adt7481_cfg = {
    .lowlimit = -20,
    .highlimit = 80,
    .critlimit = 85,
};

const INA226_Config fact_fe_ch1_ps_cfg = {
    .current_lim = 2000,
};

const ADT7481_Config fact_fe_ch2_adt7481_cfg = {
    .lowlimit = -20,
    .highlimit = 80,
    .critlimit = 85,
};

const INA226_Config fact_fe_ch2_ps_cfg = {
    .current_lim = 2000,
};

const DATR5APP_Config fact_ch1_tx_gain_cfg = {
    .attenuation = INT16_MAX, /* Default to max attenuation */
};

const DATR5APP_Config fact_ch1_rx_gain_cfg = {
    .attenuation = INT16_MAX, /* Default to max attenuation */
};

const DATR5APP_Config fact_ch2_tx_gain_cfg = {
    .attenuation = INT16_MAX, /* Default to max attenuation */
};

const DATR5APP_Config fact_ch2_rx_gain_cfg = {
    .attenuation = INT16_MAX, /* Default to max attenuation */
};

const FE_Band_Cfg fact_ch1_band_cfg = {
    .band = RFFE_BAND8_900,
};

const FE_Band_Cfg fact_ch2_band_cfg = {
    .band = RFFE_BAND8_900,
};