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
#include "src/devices/i2c/threaded_int.h"
void _rffe_watchdog_handler(void *context)
{
    RfWatchdog_Cfg *cfg = context;
    if (OcGpio_read(cfg->pin_alert_lb) > 0) {
        OCMP_GenerateAlert(context, 0, NULL, NULL, OCMP_AXN_TYPE_ACTIVE);
    }
    if (OcGpio_read(cfg->pin_alert_hb) > 0) {
        OCMP_GenerateAlert(context, 1, NULL, NULL, OCMP_AXN_TYPE_ACTIVE);
    }
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

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
    if (OcGpio_configure(cfg->pin_interrupt,
                         OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_FALLING)) {
        return POST_DEV_CFG_FAIL;
    }

    ThreadedInt_Init(cfg->pin_interrupt, _rffe_watchdog_handler, cfg);
    return POST_DEV_CFG_DONE;
}
#pragma GCC diagnostic pop
const Driver_fxnTable RFFEWatchdogP_fxnTable = {
    .cb_init = _rffe_watchdog_init,
};
