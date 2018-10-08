/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_adt7481.h"
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"
#include "common/inc/global/OC_CONNECT1.h"
#include "inc/subsystem/sdr/sdr.h"
#include "inc/devices/debug_ocgpio.h"

SCHEMA_IMPORT OcGpio_Port ec_io;
SCHEMA_IMPORT OcGpio_Port sdr_fx3_io;
/*****************************************************************************
 *                               EEPROM CONFIG
 *****************************************************************************/
Eeprom_Cfg eeprom_sdr_inv = {
    .i2c_dev = { OC_CONNECT1_I2C3, 0x50 },
    /* .pin_wp = &(OcGpio_Pin){ &sdr_eeprom_wp_io, 0 }, */
    .pin_wp = NULL, /* IO Expander disabled on rev c */
    .type = CAT24C256,
    .ss = OC_SS_SDR,
};
/*****************************************************************************
 *                               SYSTEM CONFIG
 *****************************************************************************/
/* SDR Subsystem Config.*/
// SDR FPGA power sensor.
INA226_Dev sdr_fpga_ps = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C3,
                                    .slave_addr = SDR_FPGA_CURRENT_SENSOR_ADDR,
                            },
                    .pin_alert = &(OcGpio_Pin){ &ec_io,
                                                OC_EC_SDR_FPGA_TEMP_INA_ALERT },
            },
};

//SDR FPGA temperature sensor
I2C_Dev sdr_fpga_ts = {
    .bus = OC_CONNECT1_I2C3,
    .slave_addr = SDR_FPGA_TEMP_SENSOR_ADDR,
};

//SDR EEPROM
void *sdr_eeprom_inventory = &eeprom_sdr_inv;

//SDR Power sensor
INA226_Dev sdr_ps = {
    .cfg =
            {
                    .dev =
                            {
                                    .bus = OC_CONNECT1_I2C6,
                                    .slave_addr = SDR_CURRENT_SENSOR_ADDR,
                            },
                    .pin_alert = &(OcGpio_Pin){ &ec_io, OC_EC_SDR_INA_ALERT },
            },
};

// SDR IO EXPANDERS
S_OCGPIO_Cfg debug_sdr_ioexpanderx1E = {
    .port = &sdr_fx3_io,
};

//SDR Factory config
const INA226_Config fact_sdr_3v_ps_cfg = {
    .current_lim = 3000,
};

const ADT7481_Config fact_sdr_fpga_adt7481_cfg = {
    .lowlimit = -20,
    .highlimit = 75,
    .critlimit = 85,
};

const INA226_Config fact_sdr_fpga_ps_cfg = {
    .current_lim = 500,
};

Sdr_gpioCfg sdr_gpioCfg = (Sdr_gpioCfg){
    /* EC_TRXFECONN_GPIO2/SDR_REG_LDO_PGOOD */
    .pin_sdr_reg_ldo_pgood = { &ec_io, OC_EC_SDR_PWR_GD },
    /* TRXFE_12V_ONOFF */
    .pin_trxfe_12v_onoff = { &ec_io, OC_EC_SDR_PWR_CNTRL },
    /* EC_FE_RESET_OUT/RF_FE_IO_RESET */
    .pin_rf_fe_io_reset = { &ec_io, OC_EC_SDR_FE_IO_RESET_CTRL },
    /* EC_TRXFECONN_GPIO1/SDR_RESET_IN */
    .pin_sdr_reset_in = { &ec_io, OC_EC_SDR_DEVICE_CONTROL },
    /* EC_TRXFE_RESET */
    .pin_ec_trxfe_reset = { &ec_io, OC_EC_RFFE_RESET },
    /* FX3_RESET */
    .pin_fx3_reset = { &sdr_fx3_io, 0 },
};
