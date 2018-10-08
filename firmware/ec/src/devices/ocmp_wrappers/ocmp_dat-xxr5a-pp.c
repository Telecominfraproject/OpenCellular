/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_dat-xxr5a-pp.h"

#include "common/inc/global/Framework.h"
#include "drivers/PinGroup.h"
#include "helpers/math.h"
#include "inc/subsystem/rffe/rffe.h"
#include "inc/common/global_header.h"
#include "inc/devices/dat-xxr5a-pp.h"

#include <ti/sysbios/knl/Task.h>

typedef enum DatConfig {
    DAT_CONFIG_ATTENUATION = 0,
} DatConfig;

static bool _get_config(void *driver, unsigned int param_id, void *return_buf)
{
    bool ret = false;
    switch (param_id) {
        case DAT_CONFIG_ATTENUATION: {
            DATR5APP_Cfg *cfg = driver;
            uint8_t atten;
            const PinGroup pin_group = { .num_pin = DATR5APP_PIN_COUNT,
                                         .pins = cfg->pin_group };
            if (PinGroup_read(&pin_group, &atten) != RETURN_OK) {
                ret = false;
            } else {
                LOGGER_DEBUG("DAT-XXR5A-PP+:DEBUG:: Attenuation is %.1f\n",
                             (atten / 2.0));
                *(int16_t *)return_buf = atten;
                ret = true;
            }
            break;
        }
        default: {
            LOGGER_ERROR("DAT-XXR5A-PP+::Unknown config param %d\n", param_id);
            ret = false;
        }
    }
    return ret;
}

static bool _set_attenuation(void *driver, int16_t atten)
{
    DATR5APP_Cfg *cfg = driver;

    /* TODO: no idea why we accept negative numbers */
    if (cfg->pin_16db.port) {
        atten = CONSTRAIN(atten, 0, (31.5 * 2));
    } else {
        atten = CONSTRAIN(atten, 0, (15.5 * 2));
    }

    /* Disable input latch */
    OcGpio_write(&cfg->pin_le, false);
    /* Attenuation enable */
    // OcGpio_write(&cfg->pin_tx_attn_enb, true);

    /* Set the attenuation value */
    /* TODO: value is provided as x2, so 0.5 value is bit 0, consider
     * changing, at least on CLI to more intuitive interface */
    const PinGroup pin_group = { .num_pin = DATR5APP_PIN_COUNT,
                                 .pins = cfg->pin_group };

    if (PinGroup_write(&pin_group, atten) != RETURN_OK) {
        return false;
    }

    /* Latch the input (tPDSUP, tLEPW, tPDHLD are all 10ns, so we can
     * sleep for 1ms, giving us more than enough time to latch) */
    Task_sleep(1);
    OcGpio_write(&cfg->pin_le, true);
    Task_sleep(1);
    OcGpio_write(&cfg->pin_le, false);
    Task_sleep(1);
    return true;
}

static bool _set_config(void *driver, unsigned int param_id, const void *data)
{
    switch (param_id) {
        case DAT_CONFIG_ATTENUATION: {
            return _set_attenuation(driver, *(int16_t *)data);
        }
        default:
            LOGGER_ERROR("DAT-XXR5A-PP+::Unknown config param %d\n", param_id);
            return false;
    }
}

static ePostCode _probe(void *driver)
{
    /* TODO: perhaps GPIO should provide probe/test function? for now we'll
     * assume that reading from the pin will tell us if the pin is working */
    DATR5APP_Cfg *cfg = driver;
    uint8_t atten;
    const PinGroup pin_group = { .num_pin = DATR5APP_PIN_COUNT,
                                 .pins = cfg->pin_group };
    if (PinGroup_read(&pin_group, &atten) != RETURN_OK) {
        return POST_DEV_MISSING;
    }

    return POST_DEV_FOUND;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    DATR5APP_Cfg *cfg = (DATR5APP_Cfg *)driver;
    DATR5APP_Config *cfg_atten = (DATR5APP_Config *)config;
    PinGroup pin_group = { .num_pin = DATR5APP_PIN_COUNT,
                           .pins = cfg->pin_group };
    if (OcGpio_configure(&cfg->pin_le, OCGPIO_CFG_OUTPUT) < OCGPIO_SUCCESS) {
        return POST_DEV_CFG_FAIL;
    }
    if (PinGroup_configure(&pin_group,
                           OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH) !=
        RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    /* Set default attenuation */
    _set_attenuation(driver, cfg_atten->attenuation);
    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable DATXXR5APP_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,           .cb_init = _init,
    .cb_get_status = NULL,        .cb_get_config = _get_config,
    .cb_set_config = _set_config,
};
