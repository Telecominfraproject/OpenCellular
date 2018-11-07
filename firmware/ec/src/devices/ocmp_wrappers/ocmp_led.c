
/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_led.h"

#include "inc/devices/led.h"

/* TODO: Implement enable and disable led commands in future if nedded */
bool led_testpattern_control(void *driver, void *param)
{
    ReturnStatus status = RETURN_OK;
    ledTestParam *testPattern = (ledTestParam *)param;
    switch (*testPattern) {
        case HCI_LED_OFF: {
            status = hci_led_turnoff_all(driver);
            break;
        }
        case HCI_LED_RED: {
            status = hci_led_turnon_red(driver);
            break;
        }
        case HCI_LED_GREEN: {
            status = hci_led_turnon_green(driver);
            break;
        }
        default: {
            LOGGER_ERROR("HCILED::Unknown param %d\n", *testPattern);
            status = RETURN_NOTOK;
        }
    }
    return (status == RETURN_OK);
}

static ePostCode _probe(void *driver, POSTData *postData)
{
    led_configure(driver);
    return led_probe(driver, postData);
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (led_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    if (hci_led_system_boot(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }

    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable LED_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,    .cb_init = _init,      .cb_get_status = NULL,
    .cb_get_config = NULL, .cb_set_config = NULL,
};
