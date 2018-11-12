/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_rfpowermonitor.h"

#include "inc/subsystem/rffe/rffe_powermonitor.h"

/* OCMP message handler */
static bool _ocmp_get_status(void *driver, unsigned int param_id,
                             void *return_buf)
{
    switch (param_id) {
        case FE_POWER_STATUS_FORWARD: {
            if (rffe_powermonitor_read_power(driver, RFFE_STAT_FW_POWER,
                                             return_buf) == RETURN_OK) {
                return true;
            }
            break;
        }
        case FE_POWER_STATUS_REVERSE: {
            if (rffe_powermonitor_read_power(driver, RFFE_STAT_REV_POWER,
                                             return_buf) == RETURN_OK) {
                return true;
            }
            break;
        }
        default:
            LOGGER_ERROR("RFPOWERMONITOR::Unknown status param %d\n", param_id);
            return false;
    }
    return false;
}

const Driver_fxnTable RFPowerMonitor_fxnTable = {
    .cb_get_status = _ocmp_get_status,
};
