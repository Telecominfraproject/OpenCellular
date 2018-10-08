/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_ltc4015.h"

#include "inc/devices/ltc4015.h"

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

typedef enum LTC4015Alert {
    LTC4015_ALERT_BATTERY_VOLTAGE_LOW = 0,
    LTC4015_ALERT_BATTERY_VOLTAGE_HIGH,
    LTC4015_ALERT_BATTERY_CURRENT_LOW,
    LTC4015_ALERT_INPUT_VOLTAGE_LOW,
    LTC4015_ALERT_INPUT_CURRENT_HIGH,
    LTC4015_ALERT_DIE_TEMPERATURE_HIGH,
} LTC4015Alert;

static bool _choose_battery_charger(LTC4015_Dev *dev)
{
    if (OcGpio_write(&dev->cfg.pin_lt4015_i2c_sel,
                     (dev->cfg.chem == LTC4015_CHEM_LI_ION)) < OCGPIO_SUCCESS) {
        return false;
    }
    return true;
}

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    if (!_choose_battery_charger(driver))
        return false;

    switch (param_id) {
        case LTC4015_STATUS_BATTERY_VOLTAGE: {
            int16_t *res = return_buf;
            return (LTC4015_get_battery_voltage(driver, res) == RETURN_OK);
        }
        case LTC4015_STATUS_BATTERY_CURRENT: {
            int16_t *res = return_buf;
            return (LTC4015_get_battery_current(driver, res) == RETURN_OK);
        }
        case LTC4015_STATUS_SYSTEM_VOLTAGE: {
            int16_t *res = return_buf;
            return (LTC4015_get_system_voltage(driver, res) == RETURN_OK);
        }
        case LTC4015_STATUS_INPUT_VOLATGE: {
            int16_t *res = return_buf;
            return (LTC4015_get_input_voltage(driver, res) == RETURN_OK);
        }
        case LTC4015_STATUS_INPUT_CURRENT: {
            int16_t *res = return_buf;
            return (LTC4015_get_input_current(driver, res) == RETURN_OK);
        }
        case LTC4015_STATUS_DIE_TEMPERATURE: {
            int16_t *res = return_buf;
            return (LTC4015_get_die_temperature(driver, res) == RETURN_OK);
        }
        case LTC4015_STATUS_ICHARGE_DAC: {
            int16_t *res = return_buf;
            return (LTC4015_get_icharge_dac(driver, res) == RETURN_OK);
        }
        default:
            LOGGER_ERROR("LTC4015::Unknown status param %d\n", param_id);
            return false;
    }
}

static bool _get_config(void *driver, unsigned int param_id, void *return_buf)
{
    if (!_choose_battery_charger(driver))
        return false;

    switch (param_id) {
        case LTC4015_CONFIG_BATTERY_VOLTAGE_LOW: {
            int16_t *res = return_buf;
            return (LTC4015_get_cfg_battery_voltage_low(driver, res) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH: {
            int16_t *res = return_buf;
            return (LTC4015_get_cfg_battery_voltage_high(driver, res) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_BATTERY_CURRENT_LOW: {
            int16_t *res = return_buf;
            return (LTC4015_get_cfg_battery_current_low(driver, res) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_INPUT_VOLTAGE_LOW: {
            int16_t *res = return_buf;
            return (LTC4015_get_cfg_input_voltage_low(driver, res) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_INPUT_CURRENT_HIGH: {
            int16_t *res = return_buf;
            return (LTC4015_get_cfg_input_current_high(driver, res) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_INPUT_CURRENT_LIMIT: {
            uint16_t *res = return_buf;
            return (LTC4015_get_cfg_input_current_limit(driver, res) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_ICHARGE: {
            uint16_t *res = return_buf;
            return (LTC4015_get_cfg_icharge(driver, res) == RETURN_OK);
        }
        case LTC4015_CONFIG_VCHARGE: {
            uint16_t *res = return_buf;
            return (LTC4015_get_cfg_vcharge(driver, res) == RETURN_OK);
        }
        case LTC4015_CONFIG_DIE_TEMPERATURE_HIGH: {
            int16_t *res = return_buf;
            return (LTC4015_get_cfg_die_temperature_high(driver, res) ==
                    RETURN_OK);
        }
        default:
            LOGGER_ERROR("LTC4015::Unknown config param %d\n", param_id);
            return false;
    }
}

static bool _set_config(void *driver, unsigned int param_id, const void *data)
{
    if (!_choose_battery_charger(driver))
        return false;

    switch (param_id) {
        case LTC4015_CONFIG_BATTERY_VOLTAGE_LOW: {
            const int16_t *limit = data;
            return (LTC4015_cfg_battery_voltage_low(driver, *limit) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_BATTERY_VOLTAGE_HIGH: {
            const int16_t *limit = data;
            return (LTC4015_cfg_battery_voltage_high(driver, *limit) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_BATTERY_CURRENT_LOW: {
            const int16_t *limit = data;
            return (LTC4015_cfg_battery_current_low(driver, *limit) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_INPUT_VOLTAGE_LOW: {
            const int16_t *limit = data;
            return (LTC4015_cfg_input_voltage_low(driver, *limit) == RETURN_OK);
        }
        case LTC4015_CONFIG_INPUT_CURRENT_HIGH: {
            const int16_t *limit = data;
            return (LTC4015_cfg_input_current_high(driver, *limit) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_INPUT_CURRENT_LIMIT: {
            const uint16_t *limit = data;
            return (LTC4015_cfg_input_current_limit(driver, *limit) ==
                    RETURN_OK);
        }
        case LTC4015_CONFIG_ICHARGE: {
            const uint16_t *limit = data;
            return (LTC4015_cfg_icharge(driver, *limit) == RETURN_OK);
        }
        case LTC4015_CONFIG_VCHARGE: {
            const uint16_t *limit = data;
            return (LTC4015_cfg_vcharge(driver, *limit) == RETURN_OK);
        }
        case LTC4015_CONFIG_DIE_TEMPERATURE_HIGH: {
            const int16_t *limit = data;
            return (LTC4015_cfg_die_temperature_high(driver, *limit) ==
                    RETURN_OK);
        }
        default:
            LOGGER_ERROR("LTC4015::Unknown config param %d\n", param_id);
            return false;
    }
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    LTC4015_configure(driver);
    if (!_choose_battery_charger(driver))
        return false;

    return LTC4015_probe(driver, postData);
}

static void _alert_handler(LTC4015_Event evt, int16_t value, void *alert_data)
{
    unsigned int alert;
    switch (evt) {
        case LTC4015_EVT_BVL:
            alert = LTC4015_ALERT_BATTERY_VOLTAGE_LOW;
            break;
        case LTC4015_EVT_BVH:
            alert = LTC4015_ALERT_BATTERY_VOLTAGE_HIGH;
            break;
        case LTC4015_EVT_BCL:
            alert = LTC4015_ALERT_BATTERY_CURRENT_LOW;
            break;
        case LTC4015_EVT_IVL:
            alert = LTC4015_ALERT_INPUT_VOLTAGE_LOW;
            break;
        case LTC4015_EVT_ICH:
            alert = LTC4015_ALERT_INPUT_CURRENT_HIGH;
            break;
        case LTC4015_EVT_DTH:
            alert = LTC4015_ALERT_DIE_TEMPERATURE_HIGH;
            break;
        default:
            LOGGER_ERROR("Unknown LTC4015 evt: %d\n", evt);
            return;
    }

    OCMP_GenerateAlert(alert_data, alert, &value);
    LOGGER_DEBUG("LTC4015 Event: %d Value: %d\n", evt, value);
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (!_choose_battery_charger(driver))
        return false;

    if (LTC4015_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    if (!config) {
        return POST_DEV_CFG_DONE;
    }

    const LTC4015_Config *ltc4015_config = config;

    if (LTC4015_cfg_battery_voltage_low(
                driver, ltc4015_config->batteryVoltageLow) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (LTC4015_cfg_battery_voltage_high(
                driver, ltc4015_config->batteryVoltageHigh) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (LTC4015_cfg_battery_current_low(
                driver, ltc4015_config->batteryCurrentLow) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (LTC4015_cfg_input_voltage_low(
                driver, ltc4015_config->inputVoltageLow) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (LTC4015_cfg_input_current_high(
                driver, ltc4015_config->inputCurrentHigh) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (LTC4015_cfg_input_current_limit(
                driver, ltc4015_config->inputCurrentLimit) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (ltc4015_config->icharge) {
        if (LTC4015_cfg_icharge(driver, ltc4015_config->icharge) != RETURN_OK) {
            return POST_DEV_CFG_FAIL;
        }
    }
    if (ltc4015_config->vcharge) {
        if (LTC4015_cfg_vcharge(driver, ltc4015_config->vcharge) != RETURN_OK) {
            return POST_DEV_CFG_FAIL;
        }
    }

    LTC4015_setAlertHandler(driver, _alert_handler, (void *)alert_token);
    if (LTC4015_enableLimitAlerts(driver,
                                  LTC4015_EVT_BVL | LTC4015_EVT_BVH |
                                          LTC4015_EVT_IVL | LTC4015_EVT_ICH |
                                          LTC4015_EVT_BCL) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    if (LTC4015_enableChargerStateAlerts(driver, LTC4015_EVT_BMFA)) {
        return POST_DEV_CFG_FAIL;
    }

    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable LTC4015_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,           .cb_init = _init,
    .cb_get_status = _get_status, .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
