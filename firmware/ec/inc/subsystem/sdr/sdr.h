/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef SDR_H_
#define SDR_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "drivers/OcGpio.h"
#include "inc/devices/adt7481.h"
#include "inc/devices/eeprom.h"
#include "inc/devices/ina226.h"

#include <stdbool.h>

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define SDR_TASK_PRIORITY 2
#define SDR_TASK_STACK_SIZE 4096

/*
 * Define all the constant information of RF SDR subsystem here, like device
 * addresses or constant configuration values or NUMBER of sensors
 */

/* SDR Temperature Sensor Device Addresses */
#define SDR_FPGA_TEMP_SENSOR_ADDR 0x4C

/* SDR INA226 Sensor Device Addresses */
#define SDR_FPGA_CURRENT_SENSOR_ADDR 0x44
#define SDR_CURRENT_SENSOR_ADDR 0x41

/* FX3 IO Expander Device Address */
#define SDR_EEPROM_IOEXP_ADDRESS 0x1F

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Subsystem config */
typedef struct Sdr_FpgaCfg {
    INA226_Dev current_sensor;
    const I2C_Dev temp_sensor;
} Sdr_FpgaCfg;

typedef struct Sdr_gpioCfg {
    OcGpio_Pin pin_sdr_reg_ldo_pgood;
    OcGpio_Pin pin_trxfe_12v_onoff;
    OcGpio_Pin pin_rf_fe_io_reset;
    OcGpio_Pin pin_sdr_reset_in;
    OcGpio_Pin pin_ec_trxfe_reset;
    OcGpio_Pin pin_fx3_reset;
} Sdr_gpioCfg;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
void sdr_pwr_control(Sdr_gpioCfg *driver,
                     uint8_t control); /* TODO: hack to let OBC work */

/* Schema hooks */
bool SDR_Init(void *driver, void *return_buf);
bool Sdr_InventoryGetStatus(void *driver, unsigned int param_id,
                            void *return_buf);
bool SDR_fx3Reset(void *driver, void *params);
bool SDR_reset(void *driver, void *params);

#endif /* SDR_H_ */
