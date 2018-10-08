/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_se98a.h"

#include "helpers/array.h"
#include "helpers/math.h"
#include "inc/devices/se98a.h"

typedef enum Se98aStatus {
    SE98A_STATUS_TEMPERATURE = 0,
} Se98aStatus;

typedef enum Se98aConfig {
    SE98A_CONFIG_LIM_LOW = 0,
    SE98A_CONFIG_LIM_HIGH,
    SE98A_CONFIG_LIM_CRIT,
} Se98aConfig;

typedef enum Se98aAlert {
    SE98A_ALERT_LOW = 0,
    SE98A_ALERT_HIGH,
    SE98A_ALERT_CRITICAL
} Se98aAlert;

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    switch (param_id) {
        case SE98A_STATUS_TEMPERATURE: {
            int8_t *res = return_buf;
            if (se98a_read(driver, res) == RETURN_OK) {
                return true;
            }
            break;
        }
        default:
            LOGGER_ERROR("SE98A::Unknown status param %d\n", param_id);
            return false;
    }
    return false;
}

static bool _get_config(void *driver, unsigned int param_id, void *return_buf)
{
    switch (param_id) {
        case SE98A_CONFIG_LIM_LOW:
        case SE98A_CONFIG_LIM_HIGH:
        case SE98A_CONFIG_LIM_CRIT: {
            int8_t *res = return_buf;
            if (se98a_get_limit(driver, param_id + 1, res) == RETURN_OK) {
                return true;
            }
            break;
        }
        default:
            LOGGER_ERROR("SE98A::Unknown config param %d\n", param_id);
            return false;
    }
    return false;
}

static bool _set_config(void *driver, unsigned int param_id, const void *data)
{
    switch (param_id) {
        case SE98A_CONFIG_LIM_LOW:
        case SE98A_CONFIG_LIM_HIGH:
        case SE98A_CONFIG_LIM_CRIT: {
            const int8_t *limit = data;
            if (se98a_set_limit(driver, param_id + 1, *limit) == RETURN_OK) {
                return true;
            }
            break;
        }
        default:
            LOGGER_ERROR("SE98A::Unknown config param %d\n", param_id);
            return false;
    }
    return false;
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    return se98a_probe(driver, postData);
}

static void _alert_handler(SE98A_Event evt, int8_t temperature, void *context)
{
    unsigned int alert;
    switch (evt) {
        case SE98A_EVT_ACT:
            alert = SE98A_ALERT_CRITICAL;
            break;
        case SE98A_EVT_AAW:
            alert = SE98A_ALERT_HIGH;
            break;
        case SE98A_EVT_BAW:
            alert = SE98A_ALERT_LOW;
            break;
        default:
            LOGGER_ERROR("Unknown SE98A evt: %d\n", evt);
            return;
    }

    uint8_t alert_data = (uint8_t)MAX((int8_t)0, temperature);
    OCMP_GenerateAlert(context, alert, &alert_data);
    LOGGER_DEBUG("SE98A Event: %d Temperature: %d\n", evt, temperature);
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (se98a_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (!config) {
        return POST_DEV_CFG_DONE;
    }
    const SE98A_Config *se98a_config = config;
    for (int i = 0; i < ARRAY_SIZE(se98a_config->limits); ++i) {
        if (se98a_set_limit(driver, i + 1, se98a_config->limits[i]) !=
            RETURN_OK) {
            return POST_DEV_CFG_FAIL;
        }
    }

    se98a_set_alert_handler(driver, _alert_handler, (void *)alert_token);
    if (se98a_enable_alerts(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable SE98_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,           .cb_init = _init,
    .cb_get_status = _get_status, .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
