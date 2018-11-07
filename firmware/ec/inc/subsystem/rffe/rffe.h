/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef RF_FE_H_
#define RF_FE_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/common/system_states.h"
#include "inc/devices/adt7481.h"
#include "inc/devices/eeprom.h"
#include "inc/devices/ina226.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define RFFE_TASK_PRIORITY 2
#define RFFE_TASK_STACK_SIZE 2048

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
/* TODO: replace these with the generic DATR5APP_Cfg */
typedef struct Fe_Gain_Cfg {
    OcGpio_Pin pin_tx_attn_p5db;
    OcGpio_Pin pin_tx_attn_1db;
    OcGpio_Pin pin_tx_attn_2db;
    OcGpio_Pin pin_tx_attn_4db;
    OcGpio_Pin pin_tx_attn_8db;
    OcGpio_Pin pin_tx_attn_16db;
    OcGpio_Pin pin_tx_attn_enb;
} Fe_Gain_Cfg;

typedef struct Fe_Lna_Cfg {
    OcGpio_Pin pin_rx_attn_p5db;
    OcGpio_Pin pin_rx_attn_1db;
    OcGpio_Pin pin_rx_attn_2db;
    OcGpio_Pin pin_rx_attn_4db;
    OcGpio_Pin pin_rx_attn_8db;
    OcGpio_Pin pin_unused; /* For compatability with DATR5APP_Cfg */
    OcGpio_Pin pin_rx_attn_enb;
} Fe_Lna_Cfg;

typedef struct Fe_Ch1_Gain_Cfg {
    Fe_Gain_Cfg *fe_gain_cfg;
} Fe_Ch1_Gain_Cfg;

typedef struct Fe_Ch2_Gain_Cfg {
    OcGpio_Pin pin_ch1_2g_lb_band_sel_l;
    Fe_Gain_Cfg *fe_gain_cfg;
} Fe_Ch2_Gain_Cfg;

typedef struct Fe_Ch1_Lna_Cfg {
    Fe_Lna_Cfg *fe_lna_cfg;
} Fe_Ch1_Lna_Cfg;

typedef struct Fe_Ch2_Lna_Cfg {
    OcGpio_Pin pin_ch1_rf_pwr_off;
    Fe_Lna_Cfg *fe_lna_cfg;
} Fe_Ch2_Lna_Cfg;

typedef struct Fe_Watchdog_Cfg {
    OcGpio_Pin pin_aosel_fpga;
    OcGpio_Pin pin_ch2_rf_pwr_off;
    OcGpio_Pin pin_co6_wd;
    OcGpio_Pin pin_co5_wd;
    OcGpio_Pin pin_co4_wd;
    OcGpio_Pin pin_co3_wd;
    OcGpio_Pin pin_co2_wd;
    OcGpio_Pin pin_copol_fpga;
} Fe_Watchdog_Cfg;

typedef struct Fe_gpioCfg {
    OcGpio_Pin pin_rf_pgood_ldo;
    OcGpio_Pin pin_fe_12v_ctrl;
    OcGpio_Pin pin_trxfe_conn_reset;
} Fe_gpioCfg;

typedef struct Fe_Cfg {
    Fe_gpioCfg *fe_gpio_cfg;
    Fe_Ch1_Gain_Cfg *fe_ch1_gain_cfg;
    Fe_Ch2_Gain_Cfg *fe_ch2_gain_cfg;
    Fe_Ch1_Lna_Cfg *fe_ch1_lna_cfg;
    Fe_Ch2_Lna_Cfg *fe_ch2_lna_cfg;
    Fe_Watchdog_Cfg *fe_watchdog_cfg;
} Fe_Cfg;

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t rffeBoardId[21];
    uint8_t rffeState;
} tRffeStatusData;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
bool rffe_pre_init(void *driver, void *returnValue);
bool rffe_post_init(void *driver, void *ssState);

bool RFFE_reset(void *driver, void *params);
bool RFFE_InventoryGetStatus(void *driver, unsigned int param_id,
                             void *return_buf);

#endif /* RF_FE_H_ */
