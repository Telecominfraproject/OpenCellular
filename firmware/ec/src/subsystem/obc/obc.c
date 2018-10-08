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
#include "inc/subsystem/obc/obc.h"

#include "common/inc/global/OC_CONNECT1.h"
#include "inc/common/global_header.h"
#include "inc/subsystem/sdr/sdr.h" /* temporary - Only required for 12v enable */

#include <ti/sysbios/knl/Task.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************

extern const Sdr_gpioCfg sdr_gpioCfg;

bool obc_pre_init(void *driver, void *returnValue);

ReturnStatus iridium_sw_reset(const Iridium_Cfg *iridium)
{
    DEBUG("Resetting Iridium module wait\n");

    OcGpio_configure(&iridium->pin_enable,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&iridium->pin_nw_avail, OCGPIO_CFG_INPUT);

    /* reset - for proper reset, Iridium should be disabled for ~2s */
    OcGpio_write(&iridium->pin_enable, false); /* Just to be sure it's low */
    Task_sleep(2100); // TODO: should be ~2s
    OcGpio_write(&iridium->pin_enable, true);
    Task_sleep(200); // TODO: idk...probably doesn't need to be lon
    return RETURN_OK;
}

bool IRIDIUM_reset(void *driver, void *params)
{
    ReturnStatus status = RETURN_OK;
    status = iridium_sw_reset(driver);
    return status;
}

bool obc_pre_init(void *driver, void *returnValue)
{
    /* TODO: temporary */
    /* TODO: this is a problem - need 12V for Iridium (plus 5v reg enabled)
     * I'm not sold on OBC directly enabling these lines, but there isn't
     * a great alternative at this point */
    Obc_gpioCfg *gpioCfg = (Obc_gpioCfg *)driver;
    sdr_pwr_control(&sdr_gpioCfg, OC_SDR_ENABLE);
    if (gpioCfg->pin_pwr_en) {
        if (OcGpio_configure(gpioCfg->pin_pwr_en,
                             OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH) <
            OCGPIO_SUCCESS) {
            return false;
        }
    }
    return true;
}
