/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_ina226.h"

#include "common/inc/global/Framework.h"
#include "inc/devices/ina226.h"

typedef enum INA226Status {
    INA226_STATUS_BUS_VOLTAGE = 0,
    INA226_STATUS_SHUNT_VOLTAGE,
    INA226_STATUS_CURRENT,
    INA226_STATUS_POWER,
} INA226Status;

typedef enum INA226Config {
    INA226_CONFIG_CURRENT_LIM = 0,
} INA226Config;

typedef enum INA226Alert {
    INA226_ALERT_OVERCURRENT = 0,
} INA226Alert;

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    switch (param_id) {
        case INA226_STATUS_BUS_VOLTAGE: {
            uint16_t *res = return_buf;
            return (ina226_readBusVoltage(driver, res) == RETURN_OK);
        }
        case INA226_STATUS_SHUNT_VOLTAGE: {
            uint16_t *res = return_buf;
            return (ina226_readShuntVoltage(driver, res) == RETURN_OK);
        }
        case INA226_STATUS_CURRENT: {
            uint16_t *res = return_buf;
            return (ina226_readCurrent(driver, res) == RETURN_OK);
        }
        case INA226_STATUS_POWER: {
            uint16_t *res = return_buf;
            return (ina226_readPower(driver, res) == RETURN_OK);
        }
        default:
            LOGGER_ERROR("INA226::Unknown status param %d\n", param_id);
            return false;
    }
}

static bool _get_config(void *driver, unsigned int param_id, void *return_buf)
{
    switch (param_id) {
        case INA226_CONFIG_CURRENT_LIM: {
            uint16_t *res = return_buf;
            return (ina226_readCurrentLim(driver, res) == RETURN_OK);
        }
        default:
            LOGGER_ERROR("INA226::Unknown config param %d\n", param_id);
            return false;
    }
}

static bool _set_config(void *driver, unsigned int param_id, const void *data)
{
    switch (param_id) {
        case INA226_CONFIG_CURRENT_LIM: {
            const uint16_t *limit = data;
            return (ina226_setCurrentLim(driver, *limit) == RETURN_OK);
        }
        default:
            LOGGER_ERROR("INA226::Unknown config param %d\n", param_id);
            return false;
    }
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    return ina226_probe(driver, postData);
}

static void _alert_handler(INA226_Event evt, uint16_t value, void *alert_data)
{
    if (evt != INA226_EVT_COL) {
        LOGGER_WARNING("IN226::Unsupported INA event 0x%x\n", evt);
        return;
    }

    OCMP_GenerateAlert(alert_data, INA226_ALERT_OVERCURRENT, &value);
    LOGGER_DEBUG("INA226 Event: 0x%x Current: %u\n", evt, value);
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (ina226_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    if (!config) {
        return POST_DEV_CFG_DONE;
    }
    const INA226_Config *ina226_config = config;
    if (ina226_setCurrentLim(driver, ina226_config->current_lim) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    ina226_setAlertHandler(driver, _alert_handler, (void *)alert_token);
    if (ina226_enableAlert(driver, INA226_EVT_COL) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable INA226_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,           .cb_init = _init,
    .cb_get_status = _get_status, .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
