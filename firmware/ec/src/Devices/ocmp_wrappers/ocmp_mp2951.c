/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "common/inc/global/Framework.h"
#include "inc/devices/mp2951.h"

typedef enum MP2951Status {
    MP2951_STATUS_VENDOR_ID = 0,
	MP2951_STATUS_PRODUCT_ID = 1,
} MP2951Status;

typedef enum MP2951Config {
    MP2951_INPUT_VOLT_ON_LIM = 0,
	MP2951_INPUT_VOLT_OFF_LIM = 1,
} MP2951Config;

static bool _get_status(void *driver, unsigned int param_id,
                        void *return_buf) {
    switch (param_id) {
    	case MP2951_STATUS_VENDOR_ID:{
            uint8_t *res = return_buf;
            return (mp2951_getVendId(driver, res) == RETURN_OK);
    	}
    	case MP2951_STATUS_PRODUCT_ID:{
            uint8_t *res = return_buf;
            return (mp2951_getDevId(driver, res) == RETURN_OK);
    	}
        default:
            LOGGER_ERROR("MP2951::Unknown status param %d\n", param_id);
            return false;
    }
}

static bool _get_config(void *driver, unsigned int param_id,
                        void *return_buf) {
    switch (param_id) {
        case MP2951_INPUT_VOLT_ON_LIM: {
            uint16_t *res = return_buf;
            return (MP2951_readInputVolOnLim(driver, res) == RETURN_OK);
        }
        case MP2951_INPUT_VOLT_OFF_LIM: {
            uint16_t *res = return_buf;
            return (MP2951_readInputVolOffLim(driver, res) == RETURN_OK);
        }
        default:
            LOGGER_ERROR("MP2951::Unknown config param %d\n", param_id);
            return false;
    }
}

static bool _set_config(void *driver, unsigned int param_id,
                        const void *data) {
    switch (param_id) {
        case MP2951_INPUT_VOLT_ON_LIM: {
            const uint16_t *res = data;
            return (MP2951_setInputVolOnLim(driver, *res) == RETURN_OK);
        }
        case MP2951_INPUT_VOLT_OFF_LIM: {
        	const uint16_t *res = data;
            return (MP2951_setInputVolOffLim(driver, *res) == RETURN_OK);
        }
        default:
            LOGGER_ERROR("MP2951::Unknown config param %d\n", param_id);
            return false;
    }
}
static ePostCode _probe(void *driver, POSTData *postData)
{
    return mp2951_probe(driver,postData);
}
static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    return POST_DEV_CFG_DONE;
}
const Driver_fxnTable MP2951_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,
    .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
