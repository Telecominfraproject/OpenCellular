/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_testmodule.h"

#include "inc/common/global_header.h"

typedef enum {
    TWOG_IMEI = 0,
    TWOG_IMSI = 1,
    TWOG_GETMFG = 2,
    TWOG_GETMODEL = 3,
    TWOG_RSSI = 4,
    TWOG_BER = 5,
    TWOG_REGSTATUS = 6,
    TWOG_NETWORK_OP_INFO = 7,
    TWOG_CELLID = 8,
    TWOG_BSIC = 9,
    TWOG_LASTERR = 10,
    TWOG_PARAM_MAX /* Limiter */
} eTestModule_StatusParam;

static ePostCode _probe(void **driver)
{
    /* TODO: this is a problem: we can't probe until we've initialized, but we
     * don't init until after probing */
    return POST_DEV_FOUND;
}

static ePostCode _init(void *driver, const void **config,
                       const void *alert_token)
{
    return g510_task_init(driver, config, alert_token);
}

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = false;
    switch (param_id) {
        case TWOG_IMEI: {
            ret = g510_get_imei(return_buf);
            break;
        }
        case TWOG_IMSI: {
            ret = g510_get_imsi(return_buf);
            break;
        }
        case TWOG_GETMFG: {
            ret = g510_get_mfg(return_buf);
            break;
        }
        case TWOG_GETMODEL: {
            ret = g510_get_model(return_buf);
            break;
        }
        /* TODO: optimize this - no reason to call CSQ twice */
        case TWOG_RSSI: {
            ret = g510_get_rssi(return_buf);
            break;
        }
        case TWOG_BER: {
            ret = g510_get_ber(return_buf);
            break;
        }
        case TWOG_REGSTATUS: {
            ret = g510_get_regStatus(return_buf);
            break;
        }
        case TWOG_NETWORK_OP_INFO:
            /* TODO: from +COPS=? */
            return false;
        case TWOG_CELLID: {
            ret = g510_get_cellId(return_buf);
            break;
        }
        case TWOG_BSIC: {
            /* TODO: from +MCELL? */
            return false;
        }
        case TWOG_LASTERR: {
            /* TODO: implement last error */
            return false;
        }
        default:
            LOGGER("TESTMOD::ERROR: Unknown param %d\n", param_id);
            return false;
    }
    return ret;
}

const Driver_fxnTable G510_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
};
