/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "inc/devices/ltc4015.h"

#include "devices/i2c/threaded_int.h"
#include "inc/common/byteorder.h"
#include "helpers/math.h"
#include "helpers/memory.h"
#include "ltc4015_registers.h"

#include <math.h>
#include <stdlib.h> /* For abort() */

#define WTF abort()

static ReturnStatus LTC4015_reg_write(const LTC4015_Dev *dev,
                                      uint8_t regAddress, uint16_t regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle battHandle = i2c_get_handle(dev->cfg.i2c_dev.bus);
    if (!battHandle) {
        LOGGER_ERROR("LTC4015:ERROR:: Failed to open I2C bus for battery "
                     "charge controller 0x%x.\n",
                     dev->cfg.i2c_dev.slave_addr);
    } else {
        regValue = htole16(regValue);
        status = i2c_reg_write(battHandle, dev->cfg.i2c_dev.slave_addr,
                               regAddress, regValue, 2);
    }
    return status;
}

static ReturnStatus LTC4015_reg_read(const LTC4015_Dev *dev, uint8_t regAddress,
                                     uint16_t *regValue)
{
    ReturnStatus status = RETURN_NOTOK;
    I2C_Handle battHandle = i2c_get_handle(dev->cfg.i2c_dev.bus);
    if (!battHandle) {
        LOGGER_ERROR("LTC4015:ERROR:: Failed to open I2C bus for battery "
                     "charge controller 0x%x.\n",
                     dev->cfg.i2c_dev.slave_addr);
    } else {
        status = i2c_reg_read(battHandle, dev->cfg.i2c_dev.slave_addr,
                              regAddress, regValue, 2);
        *regValue = letoh16(*regValue);
    }
    return status;
}

ReturnStatus LTC4015_cfg_icharge(LTC4015_Dev *dev,
                                 uint16_t max_chargeCurrent) // milliAmps
{
    /* Maximum charge current target = (ICHARGE_TARGET + 1) * 1mV/RSNSB
     => ICHARGE_TARGET = (target*RSNSB/1mV)-1 */
    int icharge_target =
            round((max_chargeCurrent * dev->cfg.r_snsb) / 1000.0) - 1;
    icharge_target = MAX(0, icharge_target);
    return LTC4015_reg_write(dev, LTC4015_ICHARGE_TARGET_SUBADDR,
                             icharge_target);
}

ReturnStatus LTC4015_get_cfg_icharge(LTC4015_Dev *dev,
                                     uint16_t *max_chargeCurrent) // milliAmps
{
    /* Maximum charge current target = (ICHARGE_TARGET + 1) * 1mV/RSNSB */
    uint16_t ichargeCurrent = 0x0000;
    ReturnStatus status = LTC4015_reg_read(dev, LTC4015_ICHARGE_TARGET_SUBADDR,
                                           &ichargeCurrent);
    *max_chargeCurrent = (ichargeCurrent + 1) * 1000 / dev->cfg.r_snsb;
    return status;
}

ReturnStatus LTC4015_cfg_vcharge(LTC4015_Dev *dev,
                                 uint16_t charge_voltageLevel) // millivolts
{
    /* See datasheet, page 61:VCHARGE_SETTING */
    const double target_v = charge_voltageLevel / (1000.0 * dev->cfg.cellcount);
    double vchargeSetting;
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            vchargeSetting = round((target_v - 2.0) * 105.0);
            break;
        case LTC4015_CHEM_LI_FE_PO4:
            vchargeSetting = round((target_v - 3.4125) * 80.0);
            break;
        case LTC4015_CHEM_LI_ION:
            vchargeSetting = round((target_v - 3.8125) * 80.0);
            break;
        default:
            WTF;
            break;
    }
    vchargeSetting = MAX(0, vchargeSetting);
    return LTC4015_reg_write(dev, LTC4015_VCHARGE_SETTING_SUBADDR,
                             vchargeSetting);
}

ReturnStatus
LTC4015_get_cfg_vcharge(LTC4015_Dev *dev,
                        uint16_t *charge_voltageLevel) // millivolts
{
    /* See datasheet, page 61:VCHARGE_SETTING */
    uint16_t vchargeSetting = 0x0000;
    ReturnStatus status = LTC4015_reg_read(dev, LTC4015_VCHARGE_SETTING_SUBADDR,
                                           &vchargeSetting);
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            *charge_voltageLevel = round(((vchargeSetting / 105.0) + 2.0) *
                                         dev->cfg.cellcount * 1000.0);
            break;
        case LTC4015_CHEM_LI_FE_PO4:
            *charge_voltageLevel = round(((vchargeSetting / 80.0) + 3.4125) *
                                         dev->cfg.cellcount * 1000.0);
            break;
        case LTC4015_CHEM_LI_ION:
            *charge_voltageLevel = round(((vchargeSetting / 80.0) + 3.8125) *
                                         dev->cfg.cellcount * 1000.0);
            break;
        default:
            WTF;
            break;
    }
    /* TODO: bounds check? */

    return status;
}

/* Convert a voltage to a valid vbat register value */
static uint16_t voltage_to_vbat_reg(LTC4015_Dev *dev, int16_t voltage)
{
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            return (voltage / (dev->cfg.cellcount * 128.176)) * 1000.0;
        case LTC4015_CHEM_LI_FE_PO4:
        case LTC4015_CHEM_LI_ION:
            return (voltage / (dev->cfg.cellcount * 192.264)) * 1000.0;
        default:
            WTF;
            break;
    }
    return 0; /* Should never get here, but keeps compiler happy */
}

ReturnStatus LTC4015_cfg_battery_voltage_low(LTC4015_Dev *dev,
                                             int16_t underVoltage) //millivolts
{
    /* See datasheet, page 56:VBAT_LO_ALERT_LIMIT
     under voltage limit = [VBAT_*_ALERT_LIMIT] • x(uV) */
    return LTC4015_reg_write(dev, LTC4015_VBAT_LO_ALERT_LIMIT_SUBADDR,
                             voltage_to_vbat_reg(dev, underVoltage));
}

/* Convert a voltage to a valid vbat register value */
static int16_t vbat_reg_to_voltage(LTC4015_Dev *dev, uint16_t vbat_reg)
{
    switch (dev->cfg.chem) {
        case LTC4015_CHEM_LEAD_ACID:
            return ((int16_t)vbat_reg / 1000.0) *
                   (128.176 * dev->cfg.cellcount);
        case LTC4015_CHEM_LI_FE_PO4:
        case LTC4015_CHEM_LI_ION:
            return ((int16_t)vbat_reg / 1000.0) *
                   (192.264 * dev->cfg.cellcount);
        default:
            WTF;
            break;
    }
    return 0; /* Should never get here, but keeps compiler happy */
}

ReturnStatus
LTC4015_get_cfg_battery_voltage_low(LTC4015_Dev *dev,
                                    int16_t *underVolatage) //millivolts
{
    /* See datasheet, page 56 */
    uint16_t vbatLoLimit = 0x0000;
    ReturnStatus status = LTC4015_reg_read(
            dev, LTC4015_VBAT_LO_ALERT_LIMIT_SUBADDR, &vbatLoLimit);
    *underVolatage = vbat_reg_to_voltage(dev, vbatLoLimit);
    return status;
}

ReturnStatus LTC4015_cfg_battery_voltage_high(LTC4015_Dev *dev,
                                              int16_t overVoltage) //millivolts
{
    /* See datasheet, page 56:VBAT_HI_ALERT_LIMIT
     under voltage limit = [VBAT_*_ALERT_LIMIT] • x(uV) */
    return LTC4015_reg_write(dev, LTC4015_VBAT_HI_ALERT_LIMIT_SUBADDR,
                             voltage_to_vbat_reg(dev, overVoltage));
}

ReturnStatus
LTC4015_get_cfg_battery_voltage_high(LTC4015_Dev *dev,
                                     int16_t *overVoltage) //millivolts
{
    /* See datasheet, page 56 */
    uint16_t vbatHiLimit = 0x0000;
    ReturnStatus status = LTC4015_reg_read(
            dev, LTC4015_VBAT_HI_ALERT_LIMIT_SUBADDR, &vbatHiLimit);
    *overVoltage = vbat_reg_to_voltage(dev, vbatHiLimit);
    return status;
}

ReturnStatus
LTC4015_cfg_input_voltage_low(LTC4015_Dev *dev,
                              int16_t inputUnderVoltage) // millivolts
{
    /* See datasheet, page 56:VIN_LO_ALERT_LIMIT
     VIN_LO_ALERT_LIMIT = limit/1.648mV */
    uint16_t vinLoLimit = (inputUnderVoltage / (1.648));
    return LTC4015_reg_write(dev, LTC4015_VIN_LO_ALERT_LIMIT_SUBADDR,
                             vinLoLimit);
}

ReturnStatus
LTC4015_get_cfg_input_voltage_low(LTC4015_Dev *dev,
                                  int16_t *inpUnderVoltage) //millivolts
{
    /* See datasheet, page 56
     * VIN_LO_ALERT_LIMIT = (inpUnderVoltage/(1.648)) */
    uint16_t vInLoAlertLimit = 0x0000;
    ReturnStatus status = LTC4015_reg_read(
            dev, LTC4015_VIN_LO_ALERT_LIMIT_SUBADDR, &vInLoAlertLimit);
    *inpUnderVoltage = (int16_t)vInLoAlertLimit * 1.648;
    return status;
}

ReturnStatus
LTC4015_cfg_input_current_high(LTC4015_Dev *dev,
                               int16_t inputOvercurrent) // milliAmps
{
    /* See datasheet, page 56:IIN_HI_ALERT_LIMIT
     IIN_HI_ALERT_LIMIT = (limit*RSNSI)/1.46487uV */
    uint16_t iInHiLimit = ((inputOvercurrent * dev->cfg.r_snsi) / 1.46487);
    return LTC4015_reg_write(dev, LTC4015_IIN_HI_ALERT_LIMIT_SUBADDR,
                             iInHiLimit);
}

ReturnStatus LTC4015_get_cfg_input_current_high(LTC4015_Dev *dev,
                                                int16_t *inpOverCurrent)
{
    /* See datasheet, page 56
     * IIN_HI_ALERT_LIMIT = ((inpOverCurrent*PWR_INT_BATT_RSNSI)/(1.46487)) */
    uint16_t iInHiALertLimit = 0x0000;
    ReturnStatus status = LTC4015_reg_read(
            dev, LTC4015_IIN_HI_ALERT_LIMIT_SUBADDR, &iInHiALertLimit);
    *inpOverCurrent = ((int16_t)iInHiALertLimit * 1.46487) / dev->cfg.r_snsi;
    return status;
}

ReturnStatus LTC4015_cfg_battery_current_low(LTC4015_Dev *dev,
                                             int16_t lowbattCurrent)
{
    /* See datasheet, page 56:IBAT_LO_ALERT_LIMIT
     IBAT_LO_ALERT_LIMIT = (limit*RSNSB)/1.46487uV */
    uint16_t iBatLoAlertLimit = (lowbattCurrent * dev->cfg.r_snsb) / (1.46487);
    return LTC4015_reg_write(dev, LTC4015_IBAT_LO_ALERT_LIMIT_SUBADDR,
                             iBatLoAlertLimit);
}

ReturnStatus LTC4015_get_cfg_battery_current_low(LTC4015_Dev *dev,
                                                 int16_t *lowbattCurrent)
{
    /* See datasheet, page 56
     * IBAT_LO_ALERT_LIMIT = ((current*PWR_INT_BATT_RSNSB)/(1.46487)) */
    uint16_t iBatLoAlertLimit = 0x0000;
    ReturnStatus status = LTC4015_reg_read(
            dev, LTC4015_IBAT_LO_ALERT_LIMIT_SUBADDR, &iBatLoAlertLimit);
    *lowbattCurrent = ((int16_t)iBatLoAlertLimit * 1.46487) / dev->cfg.r_snsb;
    return status;
}

ReturnStatus LTC4015_cfg_die_temperature_high(LTC4015_Dev *dev,
                                              int16_t dieTemp) // Degrees C
{
    /* See datasheet, page 57:DIE_TEMP_HI_ALERT_LIMIT
     DIE_TEMP_HI_ALERT_LIMIT = (DIE_TEMP • 12010)/45.6°C */
    uint16_t dieTempAlertLimit = (dieTemp * 45.6) + 12010;
    return LTC4015_reg_write(dev, LTC4015_DIE_TEMP_HI_ALERT_LIMIT_SUBADDR,
                             dieTempAlertLimit);
}

ReturnStatus LTC4015_get_cfg_die_temperature_high(LTC4015_Dev *dev,
                                                  int16_t *dieTemp) // Degrees C
{
    /* See datasheet, page 57
     * DIE_TEMP_HI_ALERT_LIMIT = (dieTemp • 12010)/45.6°C */
    uint16_t dieTempAlertLimit = 0x0000;
    ReturnStatus status = LTC4015_reg_read(
            dev, LTC4015_DIE_TEMP_HI_ALERT_LIMIT_SUBADDR, &dieTempAlertLimit);
    *dieTemp = (((int16_t)dieTempAlertLimit - 12010) / 45.6);
    return status;
}

ReturnStatus
LTC4015_cfg_input_current_limit(LTC4015_Dev *dev,
                                uint16_t inputCurrentLimit) // milliAmps
{
    /* See datasheet, page 61:IIN_LIMIT_SETTING
     IIN_LIMIT_SETTING = (limit * RSNSI / 500uV) - 1 */
    /* TODO: range check? this is only a 6-bit register */
    uint16_t iInLimitSetting =
            ((inputCurrentLimit * dev->cfg.r_snsi) / 500) - 1;
    return LTC4015_reg_write(dev, LTC4015_IIN_LIMIT_SETTING_SUBADDR,
                             iInLimitSetting);
}

ReturnStatus
LTC4015_get_cfg_input_current_limit(LTC4015_Dev *dev,
                                    uint16_t *currentLimit) //milli Amps
{
    /* See datasheet, page 56
     * Input current limit setting = (IIN_LIMIT_SETTING + 1) • 500uV / RSNSI */
    uint16_t iInlimitSetting = 0x0000;
    ReturnStatus status = LTC4015_reg_read(
            dev, LTC4015_IIN_LIMIT_SETTING_SUBADDR, &iInlimitSetting);
    *currentLimit = ((iInlimitSetting + 1) * 500.0) / dev->cfg.r_snsi;
    return status;
}

ReturnStatus LTC4015_get_die_temperature(LTC4015_Dev *dev,
                                         int16_t *dieTemp) // Degrees C
{
    /* Datasheet page 71: temperature = (DIE_TEMP • 12010)/45.6°C */
    uint16_t dieTemperature = 0x0000;
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_DIE_TEMP_SUBADDR, &dieTemperature);
    *dieTemp = (((int16_t)dieTemperature - 12010) / 45.6);
    return status;
}

ReturnStatus LTC4015_get_battery_current(LTC4015_Dev *dev,
                                         int16_t *iBatt) //milliAmps
{
    /* Page 70: Battery current = [IBAT] * 1.46487uV/Rsnsb */
    uint16_t batteryCurrent = 0x0000;
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_IBAT_SUBADDR, &batteryCurrent);
    *iBatt = ((float)((int16_t)batteryCurrent * 1.46487)) / (dev->cfg.r_snsb);
    return status;
}

ReturnStatus LTC4015_get_input_current(LTC4015_Dev *dev,
                                       int16_t *iIn) //milliAmps
{
    /* Page 71: Input current = [IIN] • 1.46487uV/Rsnsi */
    uint16_t inputCurrent = 0x0000;
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_IIN_SUBADDR, &inputCurrent);
    *iIn = ((float)((int16_t)inputCurrent * 1.46487)) / (dev->cfg.r_snsi);
    return status;
}

ReturnStatus LTC4015_get_battery_voltage(LTC4015_Dev *dev,
                                         int16_t *vbat) //milliVolts
{
    /* Page 71: 2's compliment VBATSENS/cellcount = [VBAT] • [x]uV */
    uint16_t batteryVoltage = 0x0000;
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_VBAT_SUBADDR, &batteryVoltage);
    *vbat = vbat_reg_to_voltage(dev, batteryVoltage);
    return status;
}

ReturnStatus LTC4015_get_input_voltage(LTC4015_Dev *dev,
                                       int16_t *vIn) //milliVolts
{
    /* Page 71: 2's compliment Input voltage = [VIN] • 1.648mV */
    uint16_t inputVoltage = 0x0000;
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_VIN_SUBADDR, &inputVoltage);
    *vIn = (int16_t)inputVoltage * 1.648;
    return status;
}

ReturnStatus LTC4015_get_system_voltage(LTC4015_Dev *dev,
                                        int16_t *vSys) //milliVolts
{
    /* Page 71: 2's compliment system voltage = [VSYS] • 1.648mV */
    uint16_t sysVoltage = 0x0000;
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_VSYS_SUBADDR, &sysVoltage);
    *vSys = (int16_t)sysVoltage * 1.648;
    return status;
}

ReturnStatus LTC4015_get_icharge_dac(LTC4015_Dev *dev,
                                     int16_t *icharge) //milliAmps
{
    /* Page 72: (ICHARGE_DAC + 1) • 1mV/RSNSB */
    uint16_t ichargeDAC = 0x0000;
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_ICHARGE_DAC_SUBADDR, &ichargeDAC);
    *icharge = (int16_t)((ichargeDAC + 1) / dev->cfg.r_snsb);
    return status;
}

static ReturnStatus _enable_limit_alerts(LTC4015_Dev *dev, uint16_t alertConfig)
{
    return LTC4015_reg_write(dev, LTC4015_EN_LIMIT_ALERTS_SUBADDR, alertConfig);
}

static ReturnStatus _read_enable_limit_alerts(LTC4015_Dev *dev,
                                              uint16_t *regValue)
{
    return LTC4015_reg_read(dev, LTC4015_EN_LIMIT_ALERTS_SUBADDR, regValue);
}

static ReturnStatus _read_limit_alerts(LTC4015_Dev *dev, uint16_t *regValue)
{
    return LTC4015_reg_read(dev, LTC4015_LIMIT_ALERTS_SUBADDR, regValue);
}

static ReturnStatus _enable_charger_state_alerts(LTC4015_Dev *dev,
                                                 uint16_t regValue)
{
    return LTC4015_reg_write(dev, LTC4015_EN_CHARGER_STATE_ALERTS_SUBADDR,
                             regValue);
}

static ReturnStatus _read_enable_charger_state_alerts(LTC4015_Dev *dev,
                                                      uint16_t *regValue)
{
    return LTC4015_reg_read(dev, LTC4015_EN_CHARGER_STATE_ALERTS_SUBADDR,
                            regValue);
}

static ReturnStatus _read_charger_state_alerts(LTC4015_Dev *dev,
                                               uint16_t *regValue)
{
    return LTC4015_reg_read(dev, LTC4015_CHARGER_STATE_ALERTS_SUBADDR,
                            regValue);
}

static ReturnStatus _read_system_status(LTC4015_Dev *dev, uint16_t *regValue)
{
    ReturnStatus status =
            LTC4015_reg_read(dev, LTC4015_SYSTEM_STATUS_SUBADDR, regValue);
    return status;
}

ReturnStatus LTC4015_get_bat_presence(LTC4015_Dev *dev, bool *present)
{
    ReturnStatus status = RETURN_OK;
    uint16_t value = 0;
    status = _read_charger_state_alerts(dev, &value);
    *present = !(value & LTC4015_EVT_BMFA);
    return status;
}

static void _ltc4015_isr(void *context)
{
    LTC4015_Dev *dev = context;
    ReturnStatus status = RETURN_OK;
    uint16_t alert_status = 0;
    int16_t val = 0;
    bool present;

    /* See if we have a callback assigned to handle alerts */
    if (!dev->obj.alert_cb) {
        return;
    }

    /* Check battery missing alarm now */
    status = LTC4015_get_bat_presence(dev, &present);
    if (status != RETURN_OK) {
        return;
    }

    /*If battery is missing no need to check other limit alerts*/
    if (!present) {
        dev->obj.alert_cb(LTC4015_EVT_BMFA, val, dev->obj.cb_context);
        return;
    }

    /* Read the alert status register to clear the alert bits if set) */
    if (_read_limit_alerts(dev, &alert_status) != RETURN_OK) {
        LOGGER_DEBUG("LTC4015:ERROR:: INT limit alerts read failed\n");
        return;
    }

    if (alert_status & LTC4015_EVT_BVL) {
        status = LTC4015_get_battery_voltage(dev, &val);
        dev->obj.alert_cb(LTC4015_EVT_BVL, val, dev->obj.cb_context);
    }
    if (alert_status & LTC4015_EVT_BVH) {
        status = LTC4015_get_battery_voltage(dev, &val);
        dev->obj.alert_cb(LTC4015_EVT_BVH, val, dev->obj.cb_context);
    }
    if (alert_status & LTC4015_EVT_IVL) {
        status = LTC4015_get_input_voltage(dev, &val);
        dev->obj.alert_cb(LTC4015_EVT_IVL, val, dev->obj.cb_context);
    }
    if (alert_status & LTC4015_EVT_ICH) {
        status = LTC4015_get_input_current(dev, &val);
        dev->obj.alert_cb(LTC4015_EVT_ICH, val, dev->obj.cb_context);
    }
    if (alert_status & LTC4015_EVT_BCL) {
        status = LTC4015_get_battery_current(dev, &val);
        dev->obj.alert_cb(LTC4015_EVT_BCL, val, dev->obj.cb_context);
    }
    if (alert_status & LTC4015_EVT_DTH) {
        status = LTC4015_get_die_temperature(dev, &val);
        dev->obj.alert_cb(LTC4015_EVT_DTH, val, dev->obj.cb_context);
    }
}

ReturnStatus LTC4015_init(LTC4015_Dev *dev)
{
    dev->obj = (LTC4015_Obj){};

    /* TODO: Do the pre-configuration here if needed */

    if (dev->cfg.pin_alert) {
        const uint32_t pin_evt_cfg = OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING;
        if (OcGpio_configure(dev->cfg.pin_alert, pin_evt_cfg) <
            OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }

        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(dev->cfg.pin_alert, _ltc4015_isr, (void *)dev);
    }
    return RETURN_OK;
}

void LTC4015_setAlertHandler(LTC4015_Dev *dev, LTC4015_CallbackFn alert_cb,
                             void *cb_context)
{
    dev->obj.alert_cb = alert_cb;
    dev->obj.cb_context = cb_context;
}

ReturnStatus LTC4015_enableLimitAlerts(LTC4015_Dev *dev, uint16_t alert_mask)
{
    uint16_t alert_reg;

    /* Read the alert status register to clear the alert bits if set) */
    ReturnStatus res = _read_limit_alerts(dev, &alert_reg);
    if (res != RETURN_OK) {
        return res;
    }

    /* Get the previously configured alerts */
    res = _read_enable_limit_alerts(dev, &alert_reg);
    if (res != RETURN_OK) {
        return res;
    }

    alert_reg |= alert_mask;

    return _enable_limit_alerts(dev, alert_reg);
}

ReturnStatus LTC4015_enableChargerStateAlerts(LTC4015_Dev *dev,
                                              uint16_t alert_mask)
{
    uint16_t alert_reg;

    /* Read the alert status register to clear the alert bits if set) */
    ReturnStatus res = _read_charger_state_alerts(dev, &alert_reg);
    if (res != RETURN_OK) {
        return res;
    }

    /* Get the previously configured alerts */
    res = _read_enable_charger_state_alerts(dev, &alert_reg);
    if (res != RETURN_OK) {
        return res;
    }

    alert_reg |= alert_mask;

    return _enable_charger_state_alerts(dev, alert_reg);
}

void LTC4015_configure(LTC4015_Dev *dev)
{
    OcGpio_configure(&dev->cfg.pin_lt4015_i2c_sel, OCGPIO_CFG_OUTPUT);
}

ePostCode LTC4015_probe(LTC4015_Dev *dev, POSTData *postData)
{
    uint16_t ltcStatusReg = 0;
    /* TODO: Check reading bits from System regsiter is enough to conclude
     * whether battery is connected or not */
    if (_read_system_status(dev, &ltcStatusReg) != RETURN_OK) {
        return POST_DEV_MISSING;
    }
    if (!(ltcStatusReg & LTC4015_CHARGER_ENABLED)) {
        return POST_DEV_MISSING;
    }
    post_update_POSTData(postData, dev->cfg.i2c_dev.bus,
                         dev->cfg.i2c_dev.slave_addr, 0xFF, 0xFF);
    return POST_DEV_FOUND;
}
