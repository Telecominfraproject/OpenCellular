/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_ltc4015.h"
#include <stdint.h>

OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

bool LTC4015_GpioPins[] = {
    [0x04] = 0x1,
};

uint32_t LTC4015_GpioConfig[] = {
    [0x04] = OCGPIO_CFG_INPUT,
};

extern const OcGpio_FnTable GpioSX1509_fnTable;

OcGpio_Port gbc_io_1 = {
    .fn_table = &GpioSX1509_fnTable,
    .cfg =
        &(SX1509_Cfg){
            .i2c_dev = { OC_CONNECT1_I2C0, BIGBROTHER_IOEXP1_ADDRESS },
            .pin_irq = NULL,
        },
    .object_data = &(SX1509_Obj){},
};

/* Invalid Device */
LTC4015_Dev gbc_pwr_invalid_dev = {
    .cfg =
        {
            .i2c_dev =
                {
                    .bus = 2,
                    .slave_addr = 0x52,
                },
            .chem = 0,
            .r_snsb = 30,
            .r_snsi = 7,
            .cellcount = 3,
            .pin_lt4015_i2c_sel = { &gbc_io_1, 4, 32 },
        },
};

/* Invalid Bus */
LTC4015_Dev gbc_pwr_invalid_bus = {
    .cfg =
        {
            .i2c_dev =
                {
                    .bus = 0xFF,
                    .slave_addr = 0x52,
                },
            .chem = 0,
            .r_snsb = 30,
            .r_snsi = 7,
            .cellcount = 3,
            .pin_lt4015_i2c_sel = { &gbc_io_1, 4, 32 },
        },
};
/* Invalid Cfg for _choose_battery_charger*/
LTC4015_Dev gbc_pwr_invalid_leadAcid_cfg = {
    .cfg =
        {
            .i2c_dev =
                {
                    .bus = 2,
                    .slave_addr = 0x52,
                },
            .chem = 0,
            .r_snsb = 30,
            .r_snsi = 7,
            .cellcount = 3,
            .pin_lt4015_i2c_sel = {},
        },
};
/* ======================== Constants & variables =========================== */
uint16_t LTC4015_regs[] = {
    [LTC4015_REG_BATTERY_VOLTAGE_LOW_LIMIT] = 0x00,
    [LTC4015_REG_BATTERY_VOLTAGE_HIGH_LIMIT] = 0x00,
    [LTC4015_REG_INPUT_VOLTAGE_LOW_LIMIT] = 0x00,
    [LTC4015_REG_INPUT_VOLTAGE_HIGH_LIMIT] = 0x00,
    [LTC4015_REG_OUTPUT_VOLTAGE_LOW_LIMIT] = 0x00,
    [LTC4015_REG_OUTPUT_VOLTAGE_HIGH_LIMIT] = 0x00,
    [LTC4015_REG_INPUT_CURRENT_HIGH_LIMIT] = 0x00,
    [LTC4015_REG_CHARGE_CURRENT_LOW_LIMIT] = 0x00,
    [LTC4015_REG_DIE_TEMP_HIGH_LIMIT] = 0x00,
    [LTC4015_REG_BATTERY_SERIES_RESISTANCE_HIGH] = 0x00,
    [LTC4015_REG_THERMISTOR_RATIO_HIGH] = 0x00,
    [LTC4015_REG_THERMISTOR_RATIO_LOW] = 0x00,
    [LTC4015_REG_ENABLE_LIMIT_MONITIOR] = 0x00,
    [LTC4015_REG_ENABLE_CHARGER_STATE] = 0x00,
    [LTC4015_REG_ENABLE_CHARGER_STATUS] = 0x00,
    [LTC4015_REG_QCOUNT_LOW_LIMIT] = 0x00,
    [LTC4015_REG_QCOUNT_HIGH_LIMIT] = 0x00,
    [LTC4015_REG_PRESCALE_FACTOR] = 0x00,
    [LTC4015_REG_COLUMB_COUNTER_VALUE] = 0x00,
    [LTC4015_REG_CONFIGURATION_SETTING] = 0x00,
    [LTC4015_REG_INPUT_CURRENT_LIMIT_SETTING] = 0x00,
    [LTC4015_REG_UVCLFB_INPUT_BUFFER] = 0x00,
    [LTC4015_REG_RESERVE_1] = 0x00,
    [LTC4015_REG_RESERVE_2] = 0x00,
    [LTC4015_REG_ARM_SHIP_MODE] = 0x00,
    [LTC4015_REG_CHARGE_CURRENT_TARGET] = 0x00,
    [LTC4015_REG_VCHARGE_SETTING] = 0x00,
    [LTC4015_REG_LOW_BAT_THRESHOLD] = 0x00,
    [LTC4015_REG_CV_STATE_BATTER_CHARGER_TIME] = 0x00,
    [LTC4015_REG_MAX_CHARGE_TIME] = 0x00,
    [LTC4015_REG_JEITA_T1] = 0x00,
    [LTC4015_REG_JEITA_T2] = 0x00,
    [LTC4015_REG_JEITA_T3] = 0x00,
    [LTC4015_REG_JEITA_T4] = 0x00,
    [LTC4015_REG_JEITA_T5] = 0x00,
    [LTC4015_REG_JEITA_T6] = 0x00,
    [LTC4015_REG_VCHARGE_JEITA_6_5] = 0x00,
    [LTC4015_REG_VCHARGE_JEITA_4_3_2] = 0x00,
    [LTC4015_REG_ICHARGE_TARGET_JEITA_6_5] = 0x00,
    [LTC4015_REG_ICHARGE_TARGET_JEITA_4_3_2] = 0x00,
    [LTC4015_REG_BATTERY_CHARGER_CFGURATION] = 0x00,
    [LTC4015_REG_ABSORB_VOLTAGE] = 0x00,
    [LTC4015_REG_ABSORB_CHARGE_MAX_TIME] = 0x00,
    [LTC4015_REG_LEADACID_EQUALIZE_CHARGE_VOLTAGE] = 0x00,
    [LTC4015_REG_LEADACID_EQUALIZATION_TIME] = 0x00,
    [LTC4015_REG_P04_RECHARGE_THRESHOLD] = 0x00,
    [LTC4015_REG_RESERVE_3] = 0x00,
    [LTC4015_REG_LITHIUM_MAX_CHARGE_TIME] = 0x00,
    [LTC4015_REG_LITIUM_CONST_VOLTAGE_REGULATION] = 0x00,
    [LTC4015_REG_LEADACID_P04_ABSORB_TIME] = 0x00,
    [LTC4015_REG_LEADACID_EQUALIZE_TIME] = 0x00,
    [LTC4015_REG_BATTERY_CHARGE_STATE] = 0x00,
    [LTC4015_REG_CHARGE_STATUS_INDICATOR] = 0x00,
    [LTC4015_REG_LIMIT_ALERT_REGISTER] = 0x00,
    [LTC4015_REG_CHARGER_STATE_ALERT_REGISTER] = 0x00,
    [LTC4015_REG_CHARGE_STATUS_ALERT_INDICATOR] = 0x00,
    [LTC4015_REG_SYSTEM_STATUS_INDICATOR] = 0x00,
    [LTC4015_REG_VBAT_VALUE] = 0x00,
    [LTC4015_REG_VIN] = 0x00,
    [LTC4015_REG_VSYS] = 0x00,
    [LTC4015_REG_BATTEY_CURRENT] = 0x00,
    [LTC4015_REG_INPUT_CURRENT] = 0x00,
    [LTC4015_REG_DIE_TEMPERATURE] = 0x00,
    [LTC4015_REG_NTC_THERMISTOR_RATIO] = 0x00,
    [LTC4015_REG_BATTERY_SERIES_RESISTANCE] = 0x00,
    [LTC4015_REG_NTC_THERMISTOR_JITA_TEMP] = 0x00,
    [LTC4015_REG_CHEM_CELLS_PIN] = 0x00,
    [LTC4015_REG_CHARGE_CUEERNT_DAC_CONTROL] = 0x00,
    [LTC4015_REG_CHARGE_VOLTAGE_DAC_CONTROL] = 0x00,
    [LTC4015_REG_INPUT_CUEERNT_DAC_CONTROL] = 0x00,
    [LTC4015_REG_DIGITALLY_BATTERY_VOLTAGE] = 0x00,
    [LTC4015_REG_BSR_IBAT_VALUE] = 0x00,
    [LTC4015_REG_RESERVE_4] = 0x00,
    [LTC4015_REG_VAILD_BIT_MESURMENT] = 0x00,
};
