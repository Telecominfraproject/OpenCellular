/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
/* Board Header files */
#include "inc/subsystem/power/power.h"

bool power_pre_init(void *driver, void *returnValue)
{
    ePostCode status = false;
    Power_cfg *powerCfg = (Power_cfg*)driver;
    if(!(powerCfg && powerCfg->driver && powerCfg->driver_cfg)) {
        return false;
    }

    /* call init to init the powersource related datastructures */
    if (powerCfg->driver->fxnTable->cb_init) {
         status = powerCfg->driver->fxnTable->cb_init(powerCfg->driver_cfg,
                        NULL,
                        NULL);
    }

    return (status==POST_DEV_NO_CFG_REQ);
}
