/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_powersource.h"
#include "helpers/array.h"
#include "inc/devices/powerSource.h"

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
    pwr_get_source_info(driver);
    if ( pwr_process_get_status_parameters_data(param_id,return_buf) == RETURN_OK) {
        ret = true;
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
static ePostCode _probe(void *driver)
{
    /* powersource is part of TIVA */
    return POST_DEV_FOUND;
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
    AlertData *alert_data_cp = alert_token;

    pwr_source_init(driver, alert_token);
    return POST_DEV_NO_CFG_REQ;
}

const Driver_fxnTable PWRSRC_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
};
