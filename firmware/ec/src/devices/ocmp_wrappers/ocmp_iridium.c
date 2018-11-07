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
#include "common/inc/ocmp_wrappers/ocmp_iridium.h"

#include <inc/devices/sbd.h>
#include "inc/common/global_header.h"

static ePostCode _probe(void **driver)
{
    /* TODO: this is a problem: we can't probe until we've initialized, but we
     * don't init until after probing */
    return POST_DEV_FOUND;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (sbd_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    return POST_DEV_CFG_DONE;
}

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = false;
    switch (param_id) {
        case IRIDIUM_NO_OUT_MSG: {
            ret = sbd9603_get_queueLength(return_buf);
            break;
        }
        case IRIDIUM_LASTERR: {
            ret = sbd9603_get_lastError(return_buf);
            break;
        }
        case IRIDIUM_IMEI: {
            ret = sbd9603_get_imei(return_buf);
            break;
        }
        case IRIDIUM_MFG: {
            ret = sbd9603_get_mfg(return_buf);
            break;
        }
        /* TODO: optimize this - no reason to call CSQ twice */
        case IRIDIUM_MODEL: {
            ret = sbd9603_get_model(return_buf);
            break;
        }
        case IRIDIUM_SIG_QUALITY: {
            ret = sbd9603_get_signalqual(return_buf);
            break;
        }
        case IRIDIUM_REGSTATUS: {
            ret = sbd9603_get_regStatus(return_buf);
            break;
        }
        default:
            LOGGER("OBC::ERROR: Unknown param %d\n", param_id);
            return false;
    }
    return ret;
}

const Driver_fxnTable OBC_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
};
