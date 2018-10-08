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
#include "inc/subsystem/rffe/rffe.h"

#include "Board.h"
#include "inc/common/system_states.h"
#include "inc/subsystem/rffe/rffe_sensor.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "inc/subsystem/rffe/rffe_powermonitor.h"
#include "inc/subsystem/sdr/sdr.h"
#include "inc/utils/util.h"
#include "registry/SSRegistry.h"

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
/* Global Task Configuration Variables */

/*****************************************************************************
 **    FUNCTION NAME   : rffe_pwr_control
 **
 **    DESCRIPTION     : Power on the FE part.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
void rffe_pwr_control(Fe_gpioCfg *feCfg, uint8_t control)
{
    if (control == OC_FE_ENABLE) {
        OcGpio_write(&feCfg->pin_fe_12v_ctrl, true);
    } else {
        OcGpio_write(&feCfg->pin_fe_12v_ctrl, false);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : rffe_init
 **
 **    DESCRIPTION     : Initializes the RF FE Modules to default values.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
bool rffe_pre_init(void *driver, void *returnValue)
{
    Fe_Cfg *feCfg = (Fe_Cfg *)driver;
    /* Initialize IO pins */
    OcGpio_configure(&feCfg->fe_gpio_cfg->pin_rf_pgood_ldo, OCGPIO_CFG_INPUT);
    OcGpio_configure(&feCfg->fe_gpio_cfg->pin_fe_12v_ctrl,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);

    /* RF power on */
    rffe_pwr_control(feCfg->fe_gpio_cfg, OC_FE_ENABLE);

    NOP_DELAY();

    /* Check Powergood status(SDR_REG_LDO_PGOOD) */
    if (OcGpio_read(&feCfg->fe_gpio_cfg->pin_rf_pgood_ldo)) {
        LOGGER("RFFE:INFO:: PowerGood Status is OK.\n");
    } else {
        LOGGER("RFFE:INFO:: PowerGood Status is NOT OK.\n");
    }

    /* Initilize FE IO Expander GPIO Controls (those not already controlled
     * by a driver) */
    Fe_Ch2_Gain_Cfg *feCh2GainCfg = (Fe_Ch2_Gain_Cfg *)(feCfg->fe_ch2_gain_cfg);
    Fe_Ch2_Lna_Cfg *feCh2LnaCfg = (Fe_Ch2_Lna_Cfg *)(feCfg->fe_ch2_lna_cfg);
    Fe_Watchdog_Cfg *feWatchDogCfg =
            (Fe_Watchdog_Cfg *)(feCfg->fe_watchdog_cfg);
    OcGpio_configure(&feCh2GainCfg->pin_ch1_2g_lb_band_sel_l,
                     OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&feCh2LnaCfg->pin_ch1_rf_pwr_off,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    OcGpio_configure(&feWatchDogCfg->pin_ch2_rf_pwr_off,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);

    /* TODO: might be cleaner to move into watchdog driver (2x init is ok) */
    OcGpio_configure(&feWatchDogCfg->pin_aosel_fpga,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    OcGpio_configure(&feWatchDogCfg->pin_copol_fpga,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);

    return true;
}

bool rffe_post_init(void *driver, void *ssState)
{
    ReturnStatus status = RETURN_OK;
    eSubSystemStates *newState = (eSubSystemStates *)ssState;

    Fe_Ch_Pwr_Cfg fe_ch1_pwrcfg = { .channel = RFFE_CHANNEL1,
                                    .fe_Rffecfg = (Fe_Cfg *)driver };

    Fe_Ch_Pwr_Cfg fe_ch2_pwrcfg = { .channel = RFFE_CHANNEL2,
                                    .fe_Rffecfg = (Fe_Cfg *)driver };

    status |= rffe_ctrl_configure_power_amplifier(&fe_ch1_pwrcfg,
                                                  RFFE_ACTIVATE_PA);

    status |= rffe_ctrl_configure_power_amplifier(&fe_ch2_pwrcfg,
                                                  RFFE_ACTIVATE_PA);

    //rffe_powermonitor_createtask();

    /*Updating subsystem sate info*/
    LOGGER_DEBUG("RFFE:INFO:: Subsystem device check and configuration is %s\n",
                 ((status == RETURN_OK) ? "successful" : "unsuccessful"));

    if (status == RETURN_OK) {
        *newState = SS_STATE_CFG;
    } else {
        *newState = SS_STATE_FAULTY;
        return false;
    }
    return true;
}

bool RFFE_reset(void *driver, void *params)
{
    /* TODO: this is the same line we use to reset the SDR - is this really
     * what we want to be doing here? */
    const Sdr_gpioCfg *cfg = (Sdr_gpioCfg *)driver;
    if (OcGpio_write(&cfg->pin_ec_trxfe_reset, false) < OCGPIO_SUCCESS) {
        return false;
    }
    Task_sleep(100);
    if (OcGpio_write(&cfg->pin_ec_trxfe_reset, true) < OCGPIO_SUCCESS) {
        return false;
    }
    return true;
}
