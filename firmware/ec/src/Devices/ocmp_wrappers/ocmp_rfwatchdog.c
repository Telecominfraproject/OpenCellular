/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_rfwatchdog.h"

#include "inc/subsystem/rffe/rffe_ctrl.h"

static ePostCode _rffe_watchdog_init(void *driver, const void *config,
                                     const void *alert_token)
{
    RfWatchdog_Cfg *cfg = (RfWatchdog_Cfg *)driver;
    if (OcGpio_configure(cfg->pin_alert_lb, OCGPIO_CFG_INPUT)) {
        return POST_DEV_CFG_FAIL;
    }
    if (OcGpio_configure(cfg->pin_alert_hb, OCGPIO_CFG_INPUT)) {
        return POST_DEV_CFG_FAIL;
    }
    if (OcGpio_configure(cfg->pin_interrupt, OCGPIO_CFG_INPUT |
                                          OCGPIO_CFG_INT_FALLING)) {
        return POST_DEV_CFG_FAIL;
    }

    ThreadedInt_Init(cfg->pin_interrupt, _rffe_watchdog_handler, cfg);
    return POST_DEV_CFG_DONE;
}

const Driver_fxnTable RFFEWatchdogP_fxnTable = {
    .cb_init = _rffe_watchdog_init,
};



