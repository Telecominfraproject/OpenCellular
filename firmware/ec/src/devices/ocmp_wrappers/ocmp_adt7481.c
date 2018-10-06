/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#include "common/inc/ocmp_wrappers/ocmp_adt7481.h"

#include "helpers/array.h"
#include "inc/devices/adt7481.h"

typedef enum Adt7481Status {
    ADT7481_STATUS_TEMPERATURE = 0,
} Adt7481Status;

typedef enum Adt7481SConfig {
    ADT7481_CONFIG_LIM_LOW = 0,
    ADT7481_CONFIG_LIM_HIGH,
    ADT7481_CONFIG_LIM_CRIT,
} Adt7481SConfig;

typedef enum Adt7481SAlert {
    ADT7481_ALERT_LOW = 0,
    ADT7481_ALERT_HIGH,
    ADT7481_ALERT_CRITICAL
} Adt7481SAlert;

static bool _get_status(void *driver, unsigned int param_id,
                        void *return_buf) {
    switch (param_id) {
        case ADT7481_STATUS_TEMPERATURE: {
            int8_t *res = return_buf;
            if (adt7481_get_remote2_temp_val(driver, res) == RETURN_OK) {
                return true;
            }
            break;
        }
        default:
            LOGGER_ERROR("ADT7481::Unknown status param %d\n", param_id);
            return false;
    }
    return false;
}

static bool _get_config(void *driver, unsigned int param_id,
                        void *return_buf) {
    switch (param_id) {
        case ADT7481_CONFIG_LIM_LOW:
        case ADT7481_CONFIG_LIM_HIGH:
        case ADT7481_CONFIG_LIM_CRIT: {
            int8_t *res = return_buf;
            if (adt7481_get_remote2_temp_limit(driver, param_id + 1, res)
                    == RETURN_OK) {
                return true;
            }
            break;
        }
        default:
            LOGGER_ERROR("ADT7481::Unknown config param %d\n", param_id);
            return false;
    }
    return false;
}

static bool _set_config(void *driver, unsigned int param_id,
                        const void *data) {
    switch (param_id) {
        case ADT7481_CONFIG_LIM_LOW:
        case ADT7481_CONFIG_LIM_HIGH:
        case ADT7481_CONFIG_LIM_CRIT: {
            const int8_t *limit = data;
            if (adt7481_set_remote2_temp_limit(driver, param_id + 1, *limit)
                    == RETURN_OK) {
                return true;
            }
            break;
        }
        default:
            LOGGER_ERROR("ADT7481::Unknown config param %d\n", param_id);
            return false;
    }
    return false;
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    return adt7481_probe(driver,postData);
}

// alert_token currently intentionally unused, so disabling unused-parameter
// check
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    const ADT7481_Config *adt7481_config = config;
    if (adt7481_config == NULL) {
        return POST_DEV_CFG_FAIL;
    }
    for (size_t i = 0; i < ARRAY_SIZE(adt7481_config->limits); ++i) {
        if (adt7481_set_local_temp_limit(
                driver, i + 1, adt7481_config->limits[i]) != RETURN_OK ||
            adt7481_set_remote1_temp_limit(
                driver, i + 1, adt7481_config->limits[i]) != RETURN_OK ||
            adt7481_set_remote2_temp_limit(
                driver, i + 1, adt7481_config->limits[i]) != RETURN_OK) {
            return POST_DEV_CFG_FAIL;
        }
    }

    if (adt7481_set_config1(driver, ADT7481_CONFIGURATION_REG_VALUE)
            != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    if (adt7481_set_conv_rate(driver, ADT7481_CONVERSION_RATE_REG_VALUE)
            != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    return POST_DEV_CFG_DONE;
}
#pragma GCC diagnostic pop

/* TODO: dedup with SE98A driver */
/* TODO: enable alerts (requires major ADT driver changes) */
const Driver_fxnTable ADT7481_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
    .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
