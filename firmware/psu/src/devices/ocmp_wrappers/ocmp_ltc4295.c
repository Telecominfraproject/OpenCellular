/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_ltc4295.h"
#include "helpers/array.h"
#include "helpers/math.h"
#include "inc/devices/ltc4295.h"

/*****************************************************************************
 **    FUNCTION NAME   : _get_status
 **
 **    DESCRIPTION     : ocmp wrapper for getting status
 **
 **    ARGUMENTS       : driver , parameter id in ocmp msg , ocmp payload
 **
 **    RETURN TYPE     : true or false
 **
 *****************************************************************************/
static bool _get_status(void *driver, unsigned int param_id,
        void *return_buf)
{
    bool ret = false;
    switch (param_id) {
    case LTC4295_STATUS_CLASS:
    {
        ePDClassType *res = (ePDClassType*)return_buf;
        if (ltc4295_get_class(driver, res) == RETURN_OK) {
            ret = true;
        }
    }
    break;
    case LTC4295_STATUS_POWERGOOD:
    {
        ePDPowerState *res =(ePDPowerState*) return_buf;
        if (ltc4295_get_power_good(driver, res) == RETURN_OK) {
            ret = true;
        }
        break;
    }
    default:
    {
        LOGGER_ERROR("LTC4295::Unknown status param %d\n", param_id);
        ret = false;
    }
    }
    return ret;
}

/*****************************************************************************
 **    FUNCTION NAME   : _probe
 **
 **    DESCRIPTION     : ocmp wrapper for handling POST(power on self test)
 **
 **    ARGUMENTS       : driver , postData
 **
 **    RETURN TYPE     : ePostCode
 **
 *****************************************************************************/
static ePostCode _probe(void *driver, POSTData *postData)
{
    ltc4295_config(driver);
    return ltc4295_probe(driver,postData);
}

/*****************************************************************************
 **    FUNCTION NAME   : _alert_handler
 **
 **    DESCRIPTION     : call back function for handling alerts from device
 **                      layer
 **
 **    ARGUMENTS       : event type , current value, alert config,
 **
 **    RETURN TYPE     :
 **
 *****************************************************************************/
static void _alert_handler(LTC4295_Event evt, void *context)
{
    unsigned int alert;
    switch (evt) {
        case LTC4295_CONNECT_EVT:
            alert = LTC4295_CONNECT_ALERT;
            break;
        case LTC4295_DISCONNECT_EVT:
            alert = LTC4295_DISCONNECT_ALERT;
            break;
        case LTC4295_INCOMPATIBLE_EVT:
            alert = LTC4295_INCOMPATIBLE_ALERT;
            break;
        default:
            LOGGER_ERROR("Unknown LTC4295evt: %d\n", evt);
            return;
    }
    OCMP_GenerateAlert(context, alert, &evt);
    LOGGER_DEBUG("LTC4295A alert: %d generated.\n", alert);
}

/*****************************************************************************
 **    FUNCTION NAME   : _init
 **
 **    DESCRIPTION     : ocmp wrapper for handling init
 **
 **    ARGUMENTS       : driver , driver config, context for cb function
 **
 **    RETURN TYPE     : ePostCode
 **
 *****************************************************************************/
static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (ltc4295_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    ltc4295_set_alert_handler(driver, _alert_handler, (void *)alert_token);
    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable LTC4295_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
};
