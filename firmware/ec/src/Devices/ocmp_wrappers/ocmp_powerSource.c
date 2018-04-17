/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "src/registry/Framework.h"

#include "helpers/array.h"
#include "inc/devices/powerSource.h"
#include "inc/devices/ocmp_wrappers/ocmp_powerSource.h"


static bool _get_status(void *driver, unsigned int param_id,
        void *return_buf)
{
    bool ret = false;
    /* TODO: As of now using pwr_get_sourc_info as it is for Power source Update.
     * Once we change the handing of the powersource status #298 this will also changed. */
    pwr_get_source_info();
    if ( pwr_process_get_status_parameters_data(param_id,return_buf) == RETURN_OK) {
        ret = true;
    }
    return ret;
}

static ePostCode _probe(void *driver)
{
    return POST_DEV_FOUND;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    pwr_source_init();
    return POST_DEV_CFG_DONE;
}

const Driver PWRSRC = {
    .name = "powerSource",
    .status = (Parameter[]){
        { .name = "poeAvailability", .type = TYPE_UINT8 },
        { .name = "poeAccessebility", .type = TYPE_UINT8 },
        { .name = "solarAvailability", .type = TYPE_UINT8 },
        { .name = "solarAccessebility", .type = TYPE_UINT8 },
        { .name = "extBattAccessebility", .type = TYPE_UINT8 },
        { .name = "extBattAccessebility", .type = TYPE_UINT8 },
        { .name = "intBattAccessebility", .type = TYPE_UINT8 },
        { .name = "intBattAccessebility", .type = TYPE_UINT8 },
        {}
    },
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
};
