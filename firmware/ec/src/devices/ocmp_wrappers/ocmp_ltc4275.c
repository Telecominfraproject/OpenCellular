/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_ltc4275.h"

#include "helpers/array.h"
#include "helpers/math.h"
#include "inc/devices/ltc4275.h"

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = false;
    switch (param_id) {
        case LTC4275_STATUS_CLASS: {
            ePDClassType *res = (ePDClassType *)return_buf;
            if (ltc4275_get_class(driver, res) == RETURN_OK) {
                ret = true;
            }
        } break;
        case LTC4275_STATUS_POWERGOOD: {
            ePDPowerState *res = (ePDPowerState *)return_buf;
            if (ltc4275_get_power_good(driver, res) == RETURN_OK) {
                ret = true;
            }
            break;
        }
        default: {
            LOGGER_ERROR("LTC4275::Unknown status param %d\n", param_id);
            ret = false;
        }
    }
    return ret;
}

/*****************************************************************************
 *****************************************************************************/
static ePostCode _probe(void *driver, POSTData *postData)
{
    ltc4275_config(driver);
    return ltc4275_probe(driver, postData);
}

/*****************************************************************************
 *****************************************************************************/
static void _alert_handler(LTC4275_Event evt, void *context)
{
    unsigned int alert;
    switch (evt) {
        case LTC4275_CONNECT_EVT:
            alert = LTC4275_CONNECT_ALERT;
            break;
        case LTC4275_DISCONNECT_EVT:
            alert = LTC4275_DISCONNECT_ALERT;
            break;
        case LTC4275_INCOMPATIBLE_EVT:
            alert = LTC4275_INCOMPATIBLE_ALERT;
            break;
        default:
            LOGGER_ERROR("Unknown LTC4275evt: %d\n", evt);
            return;
    }
    OCMP_GenerateAlert(context, alert, &evt);
    LOGGER_DEBUG("LTC4275A alert: %d generated.\n", alert);
}

/*****************************************************************************
 *****************************************************************************/
static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (ltc4275_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    ltc4275_set_alert_handler(driver, _alert_handler, (void *)alert_token);
    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable LTC4275_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
};
