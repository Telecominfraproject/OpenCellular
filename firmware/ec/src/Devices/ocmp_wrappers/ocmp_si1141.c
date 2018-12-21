/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "common/inc/global/Framework.h"
#include "inc/devices/si1141.h"

typedef enum SI1141Status {
    SI1141_STATUS_PART_ID = 0,
	SI1141_STATUS_REV_ID,
	SI1141_STATUS_SEQ_ID,
} SI1141Status;

static bool _get_status(void *driver, unsigned int param_id,
                        void *return_buf) {
    switch (param_id) {
        case SI1141_STATUS_PART_ID: {
            uint16_t *res = return_buf;
            return (si1141_getPartId(driver, res) == RETURN_OK);
        }
        case SI1141_STATUS_REV_ID: {
            uint16_t *res = return_buf;
            return (si1141_getRevId(driver, res) == RETURN_OK);
        }
        case SI1141_STATUS_SEQ_ID: {
            uint16_t *res = return_buf;
            return (si1141_getSeqId(driver, res) == RETURN_OK);
        }
        default:
            LOGGER_ERROR("SI1141::Unknown status param %d\n", param_id);
            return false;
    }
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    return si1141_probe(driver, postData);
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
   /* if (si1141_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
   }

    if (!config) {
        return POST_DEV_CFG_DONE;
    }*/
    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable	SI1141_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
    //.cb_get_config = _get_config,
    //.cb_set_config = _set_config,
};
