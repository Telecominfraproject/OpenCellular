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
#include "inc/subsystem/gpp/ebmp.h"

#include "Board.h"
#include "helpers/math.h"
#include "inc/common/system_states.h"
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
/* Global Task Configuration Variables */
static Char gppTaskStack[GPP_TASK_STACK_SIZE];

/* GPP device config */
extern void *sys_config[];
#define GPP ((Gpp_Cfg *)sys_config[OC_SS_GPP])

OCSubsystem ssGpp = {
    .taskStackSize = GPP_TASK_STACK_SIZE,
    .taskPriority = GPP_TASK_PRIORITY,
    .taskStack = gppTaskStack,
};

bool gpp_check_processor_reset(void)
{
    bool ret = false;
    if (OcGpio_read(&GPP->pin_soc_pltrst_n)) {
        ret = true;
    }
    return ret;
}

bool gpp_check_core_power(void)
{
    bool ret = false;
    if (OcGpio_read(&GPP->pin_soc_corepwr_ok)) {
        ret = true;
    }
    return ret;
}

bool gpp_pmic_control(uint8_t control)
{
    bool ret = false;

    if(control == OC_PMIC_ENABLE) {
        /*TODO:: Disabling for USB debugging*/

        OcGpio_write(&GPP->pin_ap_12v_onoff, false);
        SysCtlDelay(1000);
        /* Regsiter GPIO interrupts */
        ebmp_init();
        SysCtlDelay(100);
        OcGpio_write(&GPP->pin_ap_12v_onoff, true);
        SysCtlDelay(100);

        if(gpp_check_core_power()) {
            //OcGpio_write(&cfg->pin_ec_reset_to_proc, true);
            //SysCtlDelay(10);
            if(gpp_check_processor_reset()) {
                ret = true;
                LOGGER_DEBUG("GPP:INFO:: Processor out of reset.\n");
            }
        }
    } else {
        OcGpio_write(&GPP->pin_ap_12v_onoff, false);
        ret = true;
    }
    return ret;
}

bool gpp_msata_das(void)
{
    bool ret = false;
    if (!(OcGpio_read(&GPP->pin_msata_ec_das))) {
        ret = true;
        LOGGER_DEBUG("GPP:INFO:: mSATA is active.\n");
    }
    return ret;
}

bool gpp_pwrgd_protection(void)
{
    bool ret = false;
    if (OcGpio_read(&GPP->pin_lt4256_ec_pwrgd)) {
        ret = true;
    }
    return ret;
}

/*****************************************************************************
 * gpp_pre_init : Intiliazes all GPIO's required for initialization.
 *****************************************************************************/
bool gpp_pre_init(void *returnValue)
{
    OcGpio_configure(&GPP->pin_soc_pltrst_n, OCGPIO_CFG_INPUT);
    OcGpio_configure(&GPP->pin_soc_corepwr_ok, OCGPIO_CFG_INPUT);
    OcGpio_configure(&GPP->pin_msata_ec_das, OCGPIO_CFG_INPUT);
    OcGpio_configure(&GPP->pin_lt4256_ec_pwrgd, OCGPIO_CFG_INPUT);
    OcGpio_configure(&GPP->pin_ap_12v_onoff, OCGPIO_CFG_OUTPUT |
                                             OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&GPP->pin_ec_reset_to_proc, OCGPIO_CFG_OUTPUT |
                                                 OCGPIO_CFG_OUT_HIGH);
    return true;
}

/*****************************************************************************
 * gpp_post_init : power on devices required after GPP init is success.
 *****************************************************************************/
bool gpp_post_init(eSubSystemStates *ssState)
{
    bool ret = false;
    if (!gpp_pwrgd_protection()) {
        LOGGER_DEBUG("GPP:INFO:: LT4256 EC power good is for genration of 12V ok.\n");
    } else {
        *ssState = SS_STATE_FAULTY;
        return ret;
    }
    //Power on processor.
    if(gpp_pmic_control(OC_PMIC_ENABLE)) {
        *ssState = SS_STATE_CFG;
        ret = true;
    } else {
        *ssState = SS_STATE_FAULTY;
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
static bool gpp_ap_reset()
{
    const OcGpio_Pin *pin = &GPP->pin_ec_reset_to_proc;
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
bool GPP_ap_Reset(void *driver, void *params){
    return (gpp_ap_reset() == RETURN_OK);
}
