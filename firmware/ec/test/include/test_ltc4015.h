/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef _TEST_LTC4015_H
#define _TEST_LTC4015_H

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"
#include "drivers/GpioSX1509.h"
#include "drivers/OcGpio.h"
#include "fake/fake_GPIO.h"
#include "fake/fake_I2C.h"
#include "fake/fake_ThreadedISR.h"
#include "helpers/array.h"
#include "helpers/attribute.h"
#include "helpers/memory.h"
#include "inc/devices/ltc4015.h"
#include <string.h>
#include <math.h>
#include "unity.h"

#define LTC4015_INIT_BATTERY_VOLTAGE_LOW_LIMIT 16470
#define LTC4015_INIT_BATTERY_VOLTAGE_HIGH_LIMIT 21844
#define LTC4015_INIT_CHARGE_CURRENT_LOW_LIMIT 2047
#define LTC4015_INIT_INPUT_VOLTAGE_LOW_LIMIT 9830
#define LTC4015_INIT_INPUT_CURRENT_HIGH_LIMIT 23892
#define LTC4015_INIT_INPUT_CURRENT_LIMIT_SETTING 76
/* ======================== Constants & variables =========================== */

extern const OcGpio_FnTable GpioSX1509_fnTable;

typedef enum LTC4015Status {
    LTC4015_STATUS_BATTERY_VOLTAGE = 0,
    LTC4015_STATUS_BATTERY_CURRENT,
    LTC4015_STATUS_SYSTEM_VOLTAGE,
    LTC4015_STATUS_INPUT_VOLATGE,
    LTC4015_STATUS_INPUT_CURRENT,
    LTC4015_STATUS_DIE_TEMPERATURE,
    LTC4015_STATUS_ICHARGE_DAC
} LTC4015Status;

typedef enum LTC4015Config {
    LTC4015_CONFIG_BATTERY_VOLTAGE_LOW = 0,
    LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH,
    LTC4015_CONFIG_BATTERY_CURRENT_LOW,
    LTC4015_CONFIG_INPUT_VOLTAGE_LOW,
    LTC4015_CONFIG_INPUT_CURRENT_HIGH,
    LTC4015_CONFIG_INPUT_CURRENT_LIMIT,
    LTC4015_CONFIG_ICHARGE,
    LTC4015_CONFIG_VCHARGE,
    LTC4015_CONFIG_DIE_TEMPERATURE_HIGH,
} LTC4015Config;

/*Enumes are defined as per the LTC4015 datasheet*/
typedef enum LTC4015Regs {
    LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT = 0x01,
    LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT,
    LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT,
    LTC4015_REG_INPUT_VOLTAGE_HIGH_LIMIT,
    LTC4015_REG_OUTPUT_VOLTAGE_LOW_LIMIT,
    LTC4015_REG_OUTPUT_VOLTAGE_HIGH_LIMIT,
    LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT,
    LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT,
    LTC4015_REG_DIE_TEMP_HIGH_LIMIT,
    LTC4015_REG_BATTERY_SERIES_RESISTANCE_HIGH,
    LTC4015_REG_THERMISTOR_RATIO_HIGH,
    LTC4015_REG_THERMISTOR_RATIO_LOW,
    LTC4015_REG_ENABLE_LIMIT_MONITIOR,
    LTC4015_REG_ENABLE_CHARGER_STATE,
    LTC4015_REG_ENABLE_CHARGER_STATUS,
    LTC4015_REG_QCOUNT_LOW_LIMIT,
    LTC4015_REG_QCOUNT_HIGH_LIMIT,
    LTC4015_REG_PRESCALE_FACTOR,
    LTC4015_REG_COLUMB_COUNTER_VALUE,
    LTC4015_REG_CONFIGURATION_SETTING,
    LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING,
    LTC4015_REG_UVCLFB_INPUT_BUFFER,
    LTC4015_REG_RESERVE_1,
    LTC4015_REG_RESERVE_2,
    LTC4015_REG_ARM_SHIP_MODE,
    LTC4015_REG_CHARGE_CURRENT_TARGET,
    LTC4015_REG_VCHARGE_SETTING,
    LTC4015_REG_LOW_BAT_THRESHOLD,
    LTC4015_REG_CV_STATE_BATTER_CHARGER_TIME,
    LTC4015_REG_MAX_CHARGE_TIME,
    LTC4015_REG_JEITA_T1,
    LTC4015_REG_JEITA_T2,
    LTC4015_REG_JEITA_T3,
    LTC4015_REG_JEITA_T4,
    LTC4015_REG_JEITA_T5,
    LTC4015_REG_JEITA_T6,
    LTC4015_REG_VCHARGE_JEITA_6_5,
    LTC4015_REG_VCHARGE_JEITA_4_3_2,
    LTC4015_REG_ICHARGE_TARGET_JEITA_6_5,
    LTC4015_REG_ICHARGE_TARGET_JEITA_4_3_2,
    LTC4015_REG_BATTERY_CHARGER_CFGURATION,
    LTC4015_REG_ABSORB_VOLTAGE,
    LTC4015_REG_ABSORB_CHARGE_MAX_TIME,
    LTC4015_REG_LEADACID_EQUALIZE_CHARGE_VOLTAGE,
    LTC4015_REG_LEADACID_EQUALIZATION_TIME,
    LTC4015_REG_P04_RECHARGE_THRESHOLD,
    LTC4015_REG_RESERVE_3,
    LTC4015_REG_LITHIUM_MAX_CHARGE_TIME,
    LTC4015_REG_LITIUM_CONST_VOLTAGE_REGULATION,
    LTC4015_REG_LEADACID_P04_ABSORB_TIME,
    LTC4015_REG_LEADACID_EQUALIZE_TIME,
    LTC4015_REG_BATTERY_CHARGE_STATE,
    LTC4015_REG_CHARGE_STATUS_INDICATOR,
    LTC4015_REG_LIMIT_ALERT_REGISTER,
    LTC4015_REG_CHARGER_STATE_ALERT_REGISTER,
    LTC4015_REG_CHARGE_STATUS_ALERT_INDICATOR,
    LTC4015_REG_SYSTEM_STATUS_INDICATOR,
    LTC4015_REG_VBAT_VALUE,
    LTC4015_REG_VIN,
    LTC4015_REG_VSYS,
    LTC4015_REG_BATTEY_CURRENT,
    LTC4015_REG_INPUT_CURRENT,
    LTC4015_REG_DIE_TEMPERATURE,
    LTC4015_REG_NTC_THERMISTOR_RATIO,
    LTC4015_REG_BATTERY_SERIES_RESISTANCE,
    LTC4015_REG_NTC_THERMISTOR_JITA_TEMP,
    LTC4015_REG_CHEM_CELLS_PIN,
    LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL,
    LTC4015_REG_CHARGE_VOLTAGE_DAC_CONTROL,
    LTC4015_REG_INPUT_CUEERNT_DAC_CONTROL,
    LTC4015_REG_DIGITALLY_BATTERY_VOLTAGE,
    LTC4015_REG_BSR_IBAT_VALUE,
    LTC4015_REG_RESERVE_4,
    LTC4015_REG_VAILD_BIT_MESURMENT,
} LTC4015Regs;

int16_t ocmp_ltc4015_get_battery_current(LTC4015_Dev *dev, int16_t value);

int16_t ocmp_ltc4015_get_battery_voltage(LTC4015_Dev *dev, int16_t value);

int16_t ocmp_ltc4015_get_system_voltage(int16_t value);

int16_t ocmp_ltc4015_get_input_voltage(int16_t value);

int16_t ocmp_ltc4015_get_input_current(LTC4015_Dev *dev, int16_t value);

int16_t ocmp_ltc4015_get_dia_temperature(int16_t value);

int16_t ocmp_ltc4015_get_icharge_dac(LTC4015_Dev *dev, int16_t value);

int16_t ocmp_ltc4015_get_cfg_battery_current_low(LTC4015_Dev *dev,
                                                 int16_t value);

int16_t ocmp_ltc4015_get_cfg_input_voltage_low(int16_t value);

int16_t ocmp_ltc4015_get_cfg_input_current_high(LTC4015_Dev *dev,
                                                int16_t value);

uint16_t ocmp_ltc4015_get_cfg_input_current_limit(LTC4015_Dev *dev,
                                                  uint16_t value);

uint16_t ocmp_ltc4015_get_cfg_icharge(LTC4015_Dev *dev, uint16_t value);

uint16_t ocmp_ltc4015_get_cfg_vcharge(LTC4015_Dev *dev, uint16_t value);

int16_t ocmp_ltc4015_get_cfg_die_temperature_high(int16_t value);

uint16_t ocmp_ltc4015_set_cfg_battery_current_low(LTC4015_Dev *dev,
                                                  int16_t value);

uint16_t ocmp_ltc4015_set_cfg_input_voltage_low(int16_t value);

uint16_t ocmp_ltc4015_set_cfg_input_current_high(LTC4015_Dev *dev,
                                                 int16_t value);

uint16_t ocmp_ltc4015_set_cfg_input_current_limit(LTC4015_Dev *dev,
                                                  int16_t value);
int ocmp_ltc4015_set_cfg_icharge(LTC4015_Dev *dev, uint16_t value);

double ocmp_ltc4015_set_cfg_vcharge(LTC4015_Dev *dev, int16_t value);

uint16_t ocmp_ltc4015_set_cfg_die_temperature_high(int16_t value);

int16_t ocmp_ltc4015_set_cfg_battery_voltage(LTC4015_Dev *dev, int16_t value);
#endif
