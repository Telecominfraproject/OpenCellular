/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_powersource.h"

#include "common/inc/global/Framework.h"
#include "inc/devices/powerSource.h"
#include "helpers/array.h"

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = false;
    /* TODO: As of now using pwr_get_sourc_info as it is for Power source Update.
     * Once we change the handing of the powersource status #298 this will also changed. */
    pwr_get_source_info(driver);
    if (pwr_process_get_status_parameters_data(param_id, return_buf) ==
        RETURN_OK) {
        ret = true;
    }
    return ret;
}

static ePostCode _probe(void *driver)
{
    pwr_source_config(driver);
    return POST_DEV_NOSTATUS;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    pwr_source_init();
    return POST_DEV_NO_CFG_REQ;
}

const Driver_fxnTable PWRSRC_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
};
