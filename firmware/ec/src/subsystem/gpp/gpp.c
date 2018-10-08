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
#include "inc/subsystem/gpp/gpp.h"

#include "Board.h"
#include "helpers/math.h"
#include "inc/common/system_states.h"
#include "inc/subsystem/gpp/ebmp.h"
#include "inc/utils/util.h"
#include "registry/SSRegistry.h"

#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <inc/hw_memmap.h>
#include <ti/sysbios/BIOS.h>

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************

bool gpp_check_processor_reset(Gpp_gpioCfg *driver)
{
    bool ret = false;
    if (OcGpio_read(&driver->pin_soc_pltrst_n)) {
        ret = true;
    }
    return ret;
}

bool gpp_check_core_power(Gpp_gpioCfg *driver)
{
    bool ret = false;
    if (OcGpio_read(&driver->pin_soc_corepwr_ok)) {
        ret = true;
    }
    return ret;
}

bool gpp_pmic_control(Gpp_gpioCfg *driver, uint8_t control)
{
    bool ret = false;

    if (control == OC_PMIC_ENABLE) {
        /*TODO:: Disabling for USB debugging*/

        OcGpio_write(&driver->pin_ap_12v_onoff, false);
        SysCtlDelay(1000);
        /* Embedded Boot Management Protocol (EBMP)for Application processor(AP)
         * EBMP tells EC firmware about the boot process stages of the AP to the
         * Embedded Controller(EC) by toggling two GPIO's.ebmp_init is a function
         * where we register the required GPIO's for interrupts if AP move from one boot
         * process state to other.*/
        ebmp_init(driver);
        SysCtlDelay(100);
        OcGpio_write(&driver->pin_ap_12v_onoff, true);
        SysCtlDelay(100);

        if (gpp_check_core_power(driver)) {
            //OcGpio_write(&cfg->pin_ec_reset_to_proc, true);
            //SysCtlDelay(10);
            if (gpp_check_processor_reset(driver)) {
                ret = true;
                LOGGER_DEBUG("GPP:INFO:: Processor out of reset.\n");
            }
        }
    } else {
        OcGpio_write(&driver->pin_ap_12v_onoff, false);
        ret = true;
    }
    return ret;
}

bool gpp_msata_das(Gpp_gpioCfg *driver)
{
    bool ret = false;
    if (!(OcGpio_read(&driver->pin_msata_ec_das))) {
        ret = true;
        LOGGER_DEBUG("GPP:INFO:: mSATA is active.\n");
    }
    return ret;
}

bool gpp_pwrgd_protection(Gpp_gpioCfg *driver)
{
    bool ret = false;
    if (OcGpio_read(&driver->pin_lt4256_ec_pwrgd)) {
        ret = true;
    }
    return ret;
}

/*****************************************************************************
 * gpp_pre_init : Intiliazes all GPIO's required for initialization.
 *****************************************************************************/
bool gpp_pre_init(void *driver, void *returnValue)
{
    Gpp_gpioCfg *gpioCfg = (Gpp_gpioCfg *)driver;
    OcGpio_configure(&gpioCfg->pin_soc_pltrst_n, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->pin_soc_corepwr_ok, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->pin_msata_ec_das, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->pin_lt4256_ec_pwrgd, OCGPIO_CFG_INPUT);
    OcGpio_configure(&gpioCfg->pin_ap_12v_onoff,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&gpioCfg->pin_ec_reset_to_proc,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    return true;
}

/*****************************************************************************
 * gpp_post_init : power on devices required after GPP init is success.
 *****************************************************************************/
bool gpp_post_init(void *driver, void *ssState)
{
    bool ret = false;
    eSubSystemStates *newState = (eSubSystemStates *)ssState;
    if (!gpp_pwrgd_protection(driver)) {
        LOGGER_DEBUG(
                "GPP:INFO:: LT4256 EC power good is for genration of 12V ok.\n");
    } else {
        *newState = SS_STATE_FAULTY;
        return ret;
    }
    //Power on processor.
    if (gpp_pmic_control(driver, OC_PMIC_ENABLE)) {
        *newState = SS_STATE_CFG;
        ret = true;
    } else {
        *newState = SS_STATE_FAULTY;
    }
    /*mSATA DAS not helping with anything as of now.*/
    //        if (!gpp_msata_das()) {
    //            returnValue = SS_STATE_FAULTY;
    //            LOGGER_ERROR("GPP:ERROR:: GPP mSATA device activity failure.\n");
    //        }
    return ret;
}

/*****************************************************************************
 * gpp_ap_reset : resets application processor.
 *****************************************************************************/
static bool gpp_ap_reset(Gpp_gpioCfg *driver)
{
    const OcGpio_Pin *pin = &(driver->pin_ec_reset_to_proc);
    if (OcGpio_write(pin, OC_GBC_PROC_RESET) < OCGPIO_SUCCESS) {
        return false;
    }
    Task_sleep(100);
    if (OcGpio_write(pin, OC_GBC_PROC_ENABLE) < OCGPIO_SUCCESS) {
        return false;
    }
    return true;
}

/*****************************************************************************
 * gpp_ap_reset : Calls application processor reset function.
 *****************************************************************************/
bool GPP_ap_Reset(void *driver, void *params)
{
    return (gpp_ap_reset(driver) == RETURN_OK);
}
