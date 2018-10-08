/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_ltc4274.h"

#include "helpers/array.h"
#include "helpers/math.h"
#include "inc/devices/ltc4274.h"

typedef enum LTC7274Status {
    LTC7274_STATUS_DETECT = 0,
    LTC7274_STATUS_CLASS,
    LTC7274_STATUS_POWERGOOD,
} LTC7274Status;

typedef enum LTC7274Config {
    LTC4274_CONFIG_OPERATING_MODE = 0,
    LTC4274_CONFIG_DETECT_ENABLE,
    LTC4274_CONFIG_INTERRUPT_MASK,
    LTC4274_CONFIG_INTERRUPT_ENABLE,
    LTC4274_CONFIG_HP_ENABLE
} LTC7274Config;

typedef enum LTC7274Alert {
    LTC4274_ALERT_NO_ACTIVE = 0,
    LTC4274_ALERT_POWER_ENABLE,
    LTC4274_ALERT_POWERGOOD,
    LTC4274_ALERT_DISCONNECT,
    LTC4274_ALERT_DETECTION,
    LTC4274_ALERT_CLASS,
    LTC4274_ALERT_TCUT,
    LTC4274_ALERT_TSTART,
    LTC4274_ALERT_SUPPLY
} LTC7274Alert;

// *params not needed by reset but kept in definition to match CB_Command type.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool LTC4274_reset(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = ltc4274_reset(driver);
    return status;
}
#pragma GCC diagnostic pop

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = true;
    uint8_t *res = return_buf;
    switch (param_id) {
        case LTC7274_STATUS_DETECT: {
            if (ltc4274_get_detection_status(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: Reading PSE detection and classification failed.\n");
            }
            break;
        }
        case LTC7274_STATUS_CLASS: {
            if (ltc4274_get_class_status(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: Reading PSE power status failed.\n");
            }
            break;
        }
        case LTC7274_STATUS_POWERGOOD: {
            if (ltc4274_get_powergood_status(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: Reading PSE power status failed.\n");
                return ret;
            }
            break;
        }
        default: {
            LOGGER("LTC4274:ERROR::Unkown parameter recived for PSE status.\n");
            ret = false;
        }
    }
    return ret;
}

static bool _set_config(void *driver, unsigned int param_id, const void *data)
{
    bool ret = true;
    uint8_t *res = (uint8_t *)data;
    switch (param_id) {
        case LTC4274_CONFIG_OPERATING_MODE: {
            if (ltc4274_set_cfg_operation_mode(driver, *res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE operational mode setting mode failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_DETECT_ENABLE: {
            if (ltc4274_set_cfg_detect_enable(driver, *res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE detection and classification enable failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_INTERRUPT_MASK: {
            if (ltc4274_set_interrupt_mask(driver, *res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR::PSE interrupts mask enable failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_INTERRUPT_ENABLE: {
            if (ltc4274_cfg_interrupt_enable(driver, *res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE interrupt enable failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_HP_ENABLE: {
            if (ltc4274_set_cfg_pshp_feature(driver, *res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE configuration for LTEPOE++ failed.\n");
            }
            break;
        }
        default: {
            LOGGER("LTC4274:ERROR:: PSE configurion unkown parmeter passed.\n");
            ret = false;
        }
    }
    return ret;
}

static bool _get_config(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = true;
    uint8_t *res = return_buf;
    switch (param_id) {
        case LTC4274_CONFIG_OPERATING_MODE: {
            if (ltc4274_get_operation_mode(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE operational mode setting mode failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_DETECT_ENABLE: {
            if (ltc4274_get_detect_enable(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE detection and classification enable failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_INTERRUPT_MASK: {
            if (ltc4274_get_interrupt_mask(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR::PSE interrupts mask enable failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_INTERRUPT_ENABLE: {
            if (ltc4274_get_interrupt_enable(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE interrupt enable failed.\n");
            }
            break;
        }
        case LTC4274_CONFIG_HP_ENABLE: {
            if (ltc4274_get_pshp_feature(driver, res) != RETURN_OK) {
                ret = false;
                LOGGER("LTC4274:ERROR:: PSE configuration for LTEPOE++ failed.\n");
            }
            break;
        }
        default: {
            LOGGER("LTC4274:ERROR:: PSE configurion unkown parmeter passed.\n");
            ret = false;
        }
    }
    return ret;
}
static ePostCode _probe(void *driver, POSTData *postData)
{
    ltc4274_config(driver);
    return ltc4274_probe(driver, postData);
}

static void _alert_handler(LTC4274_Event evt, void *context)
{
    unsigned int alert;
    switch (evt) {
        case LTC4274_EVT_SUPPLY:
            alert = LTC4274_ALERT_SUPPLY;
            break;
        case LTC4274_EVT_TSTART:
            alert = LTC4274_ALERT_TSTART;
            break;
        case LTC4274_EVT_TCUT:
            alert = LTC4274_ALERT_TCUT;
            break;
        case LTC4274_EVT_CLASS:
            alert = LTC4274_ALERT_CLASS;
            break;
        case LTC4274_EVT_DETECTION:
            alert = LTC4274_ALERT_DETECTION;
            break;
        case LTC4274_EVT_DISCONNECT:
            alert = LTC4274_ALERT_DISCONNECT;
            break;
        case LTC4274_EVT_POWERGOOD:
            alert = LTC4274_ALERT_POWERGOOD;
            break;
        case LTC4274_EVT_POWER_ENABLE:
            alert = LTC4274_ALERT_POWER_ENABLE;
            break;
        default: {
            alert = LTC4274_ALERT_NO_ACTIVE;
            return;
        }
    }
    uint8_t alert_data = 0x00;
    OCMP_GenerateAlert(context, alert, &alert_data);
    LOGGER_DEBUG("LTC7274 Event: %d \n", evt);
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    ltc4274_init(driver);

    if (!config) {
        return POST_DEV_CFG_DONE;
    }
    const LTC4274_Config *LTC7274_config = config;
    if (ltc4274_set_cfg_operation_mode(driver, LTC7274_config->operatingMode) !=
        RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (ltc4274_set_cfg_detect_enable(driver, LTC7274_config->detectEnable) !=
        RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (ltc4274_set_interrupt_mask(driver, LTC7274_config->interruptMask) !=
        RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    if (ltc4274_set_cfg_pshp_feature(driver, LTC7274_config->pseHpEnable) !=
        RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    ltc4274_set_alert_handler(driver, _alert_handler, (void *)alert_token);
    //TODO: SET enable or disable.
    if (ltc4274_cfg_interrupt_enable(driver, LTC7274_config->interruptEnable) !=
        RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    ltc4274_update_stateInfo(driver);

    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable LTC4274_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,           .cb_init = _init,
    .cb_get_status = _get_status, .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
