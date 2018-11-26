/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdint.h>
#include "include/test_ltc4015.h"

int16_t ocmp_ltc4015_get_battery_voltage(LTC4015_Dev *dev, int16_t value)
{
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            return ((int16_t)value / 1000.0) * (128.176 * dev->cfg.cellcount);
        case LTC4015_CHEM_LI_FE_PO4:
        case LTC4015_CHEM_LI_ION:
            return ((int16_t)value / 1000.0) * (192.264 * dev->cfg.cellcount);
        default:
            break;
    }
    return 0;
}

int16_t ocmp_ltc4015_get_battery_current(LTC4015_Dev *dev, int16_t value)
{
    int16_t expBatteryCurrent = 0;
    expBatteryCurrent = ((float)((int16_t)value * 1.46487)) / (dev->cfg.r_snsb);
    return expBatteryCurrent;
}

int16_t ocmp_ltc4015_get_system_voltage(int16_t value)
{
    int16_t expSystemVoltage = 0;
    expSystemVoltage = (int16_t)value * 1.648;
    return expSystemVoltage;
}

int16_t ocmp_ltc4015_get_input_voltage(int16_t value)
{
    int16_t expInputVoltage = 0;
    expInputVoltage = (int16_t)value * 1.648;
    return expInputVoltage;
}

int16_t ocmp_ltc4015_get_input_current(LTC4015_Dev *dev, int16_t value)
{
    int16_t expInputCurrent = 0;
    expInputCurrent = ((float)((int16_t)value * 1.46487)) / (dev->cfg.r_snsi);
    return expInputCurrent;
}

int16_t ocmp_ltc4015_get_dia_temperature(int16_t value)
{
    int16_t expDieTemperature = 0;
    expDieTemperature = (((int16_t)value - 12010) / 45.6);
    return expDieTemperature;
}

int16_t ocmp_ltc4015_get_icharge_dac(LTC4015_Dev *dev, int16_t value)
{
    int16_t expICharge = 0;
    expICharge = (int16_t)((value + 1) / dev->cfg.r_snsb);
    return expICharge;
}

int16_t ocmp_ltc4015_get_cfg_battery_current_low(LTC4015_Dev *dev,
                                                 int16_t value)
{
    int16_t expBatteryCurrentLow = 0;
    expBatteryCurrentLow = ((int16_t)value * 1.46487) / dev->cfg.r_snsb;
    return expBatteryCurrentLow;
}

int16_t ocmp_ltc4015_get_cfg_input_voltage_low(int16_t value)
{
    int16_t expInputVoltageLow = 0;
    expInputVoltageLow = (int16_t)value * 1.648;
    return expInputVoltageLow;
}

int16_t ocmp_ltc4015_get_cfg_input_current_high(LTC4015_Dev *dev, int16_t value)
{
    int16_t expInputCurrentHigh = 0;
    expInputCurrentHigh = ((int16_t)value * 1.46487) / dev->cfg.r_snsi;
    return expInputCurrentHigh;
}

uint16_t ocmp_ltc4015_get_cfg_input_current_limit(LTC4015_Dev *dev,
                                                  uint16_t value)
{
    uint16_t expInputCurrentLimit = 0;
    expInputCurrentLimit = ((value + 1) * 500.0) / dev->cfg.r_snsi;
    return expInputCurrentLimit;
}

uint16_t ocmp_ltc4015_get_cfg_icharge(LTC4015_Dev *dev, uint16_t value)
{
    uint16_t expIcharge = 0;
    expIcharge = (value + 1) * 1000 / dev->cfg.r_snsb;
    return expIcharge;
}

uint16_t ocmp_ltc4015_get_cfg_vcharge(LTC4015_Dev *dev, uint16_t value)
{
    uint16_t vcharge = 0;
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            vcharge =
                round(((value / 105.0) + 2.0) * dev->cfg.cellcount * 1000.0);
            break;
        case LTC4015_CHEM_LI_FE_PO4:
            vcharge =
                round(((value / 80.0) + 3.4125) * dev->cfg.cellcount * 1000.0);
            break;
        case LTC4015_CHEM_LI_ION:
            vcharge =
                round(((value / 80.0) + 3.8125) * dev->cfg.cellcount * 1000.0);
            break;
        default:
            break;
    }
    return vcharge;
}

int16_t ocmp_ltc4015_get_cfg_die_temperature_high(int16_t value)
{
    int16_t expDieTempHigh = 0;
    expDieTempHigh = (((int16_t)value - 12010) / 45.6);
    return expDieTempHigh;
}

int16_t ocmp_ltc4015_set_cfg_battery_voltage(LTC4015_Dev *dev, int16_t value)
{
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            return (value / (dev->cfg.cellcount * 128.176)) * 1000.0;
        case LTC4015_CHEM_LI_FE_PO4:
        case LTC4015_CHEM_LI_ION:
            return (value / (dev->cfg.cellcount * 192.264)) * 1000.0;
        default:
            break;
    }
    return 0;
}

uint16_t ocmp_ltc4015_set_cfg_battery_current_low(LTC4015_Dev *dev,
                                                  int16_t value)
{
    uint16_t expBatteryCurrentLow = 0;
    expBatteryCurrentLow = (value * dev->cfg.r_snsb) / (1.46487);
    return expBatteryCurrentLow;
}

uint16_t ocmp_ltc4015_set_cfg_input_voltage_low(int16_t value)
{
    uint16_t expInputVoltageLow = 0;
    expInputVoltageLow = (value / (1.648));
    return expInputVoltageLow;
}
uint16_t ocmp_ltc4015_set_cfg_input_current_high(LTC4015_Dev *dev,
                                                 int16_t value)
{
    uint16_t expInputCurrentHigh = 0;
    expInputCurrentHigh = ((value * dev->cfg.r_snsi) / 1.46487);
    return expInputCurrentHigh;
}

uint16_t ocmp_ltc4015_set_cfg_input_current_limit(LTC4015_Dev *dev,
                                                  int16_t value)
{
    uint16_t expInputCurrentLimit = 0;
    expInputCurrentLimit = ((value * dev->cfg.r_snsi) / 500) - 1;
    return expInputCurrentLimit;
}

int ocmp_ltc4015_set_cfg_icharge(LTC4015_Dev *dev, uint16_t value)
{
    int expIcharge = 0;
    expIcharge = round((value * dev->cfg.r_snsb) / 1000.0) - 1;
    expIcharge = MAX(0, expIcharge);
    return expIcharge;
}

double ocmp_ltc4015_set_cfg_vcharge(LTC4015_Dev *dev, int16_t value)
{
    double targetV = value / (1000.0 * dev->cfg.cellcount);
    double expVChargeSetting = 0;
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            expVChargeSetting = round((targetV - 2.0) * 105.0);
            break;
        case LTC4015_CHEM_LI_FE_PO4:
            expVChargeSetting = round((targetV - 3.4125) * 80.0);
            break;
        case LTC4015_CHEM_LI_ION:
            expVChargeSetting = round((targetV - 3.8125) * 80.0);
            break;
        default:
            break;
    }
    expVChargeSetting = MAX(0, expVChargeSetting);
    return expVChargeSetting;
}

uint16_t ocmp_ltc4015_set_cfg_die_temperature_high(int16_t value)
{
    uint16_t expdieTempAlertLimit = 0;
    expdieTempAlertLimit = (value * 45.6) + 12010;
    return expdieTempAlertLimit;
}
