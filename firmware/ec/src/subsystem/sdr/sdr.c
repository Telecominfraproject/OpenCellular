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
#include "inc/subsystem/sdr/sdr.h"

#include "Board.h"
#include "registry/SSRegistry.h"

#include <driverlib/sysctl.h>

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
/* Global Task Configuration Variables */

/* SDR device config */

/*****************************************************************************
 **    FUNCTION NAME   : sdr_pwr_control
 **
 **    DESCRIPTION     : Power on the SDR part.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
void sdr_pwr_control(Sdr_gpioCfg *sdr_gpioCfg, uint8_t control)
{
    /* Using configure instead of 'write' here since this pin is shared with
     * OBC and we don't want to configure then write if OBC has already
     * configured it (it'll reset Iridium that way) */
    if (control == OC_SDR_ENABLE) {
        OcGpio_configure(&sdr_gpioCfg->pin_trxfe_12v_onoff,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    } else {
        OcGpio_configure(&sdr_gpioCfg->pin_trxfe_12v_onoff,
                         OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : sdr_control_ioexpander
 **
 **    DESCRIPTION     : Power on the SDR part.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
static void sdr_control_ioexpander(Sdr_gpioCfg *sdr_gpioCfg, uint8_t control)
{
    if (control == OC_SDR_ENABLE) {
        OcGpio_write(&sdr_gpioCfg->pin_rf_fe_io_reset, true);
    } else {
        OcGpio_write(&sdr_gpioCfg->pin_rf_fe_io_reset, false);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : sdr_control_device
 **
 **    DESCRIPTION     : Power on the SDR part.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
static void sdr_control_device(Sdr_gpioCfg *sdr_gpioCfg, uint8_t control)
{
    if (control == OC_SDR_ENABLE) {
        OcGpio_write(&sdr_gpioCfg->pin_sdr_reset_in, true);
    } else {
        OcGpio_write(&sdr_gpioCfg->pin_sdr_reset_in, false);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : sdr_control_reset
 **
 **    DESCRIPTION     : Power on the SDR part.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ReturnStatus
 **
 *****************************************************************************/
static void sdr_control_reset(Sdr_gpioCfg *sdr_gpioCfg, uint8_t control)
{
    if (control == OC_SDR_ENABLE) {
        OcGpio_write(&sdr_gpioCfg->pin_ec_trxfe_reset, true);
    } else {
        OcGpio_write(&sdr_gpioCfg->pin_ec_trxfe_reset, false);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : sdr_fx3_reset
 **
 **    DESCRIPTION     : reset the FX3 interface.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : Success or Failure
 **
 *****************************************************************************/
static ReturnStatus sdr_fx3_reset(Sdr_gpioCfg *fx3_cfg)
{
    /*TODO: We need to figure out a way for configuring PCA pins on Intel reset.*/
    OcGpio_configure(&fx3_cfg->pin_fx3_reset, OCGPIO_CFG_OUTPUT);

    if (OcGpio_write(&fx3_cfg->pin_fx3_reset, false) < OCGPIO_SUCCESS) {
        return RETURN_NOTOK;
    }

    /* Provide small delay */
    SysCtlDelay(5);

    if (OcGpio_write(&fx3_cfg->pin_fx3_reset, true) < OCGPIO_SUCCESS) {
        return RETURN_NOTOK;
    }
    return RETURN_OK;
}

bool SDR_fx3Reset(void *driver, void *params)
{
    return (sdr_fx3_reset(driver) == RETURN_OK);
}

/*****************************************************************************
 **    FUNCTION NAME   : sdr_init
 **
 **    DESCRIPTION     : Initializes the SDR Modules to default values.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : ePostCode
 **
 *****************************************************************************/
bool SDR_Init(void *driver, void *return_buf)
{
    Sdr_gpioCfg *sdr_gpioCfg = (Sdr_gpioCfg *)driver;
    /* Initialize IO pins */
    OcGpio_configure(&sdr_gpioCfg->pin_sdr_reg_ldo_pgood, OCGPIO_CFG_INPUT);
    OcGpio_configure(&sdr_gpioCfg->pin_rf_fe_io_reset,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&sdr_gpioCfg->pin_sdr_reset_in,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&sdr_gpioCfg->pin_ec_trxfe_reset,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);

    /* Power On SDR  */
    sdr_pwr_control(sdr_gpioCfg, OC_SDR_ENABLE);

    NOP_DELAY();

    /* Move IO expander out of reset*/
    sdr_control_ioexpander(sdr_gpioCfg, OC_SDR_ENABLE);

    /*Enable SDR devices*/
    sdr_control_device(sdr_gpioCfg, OC_SDR_ENABLE);

    /* Added in SDR beacuse of U30 (SN74LV08A) */
    /*RF_IO_RESET signal depends on this TRXFE_RESET*/
    /*Enable TRX_FE_RESET*/
    sdr_control_reset(sdr_gpioCfg, OC_SDR_ENABLE);
    /*Move*/

    NOP_DELAY();

    /* Check Powergood status(SDR_REG_LDO_PGOOD) */
    if (OcGpio_read(&sdr_gpioCfg->pin_sdr_reg_ldo_pgood)) {
        LOGGER("SDR:INFO:: PowerGood Status is OK.\n");
    } else {
        LOGGER("SDR:INFO:: PowerGood Status is NOT OK.\n");
    }

    /* Make FX3 out of reset mode */
    ReturnStatus status = sdr_fx3_reset(driver);
    NOP_DELAY();

    LOGGER("SDR:INFO:: FX3 Reset is %s.\n",
           ((status == RETURN_OK) ? "done" : "not done"));

    return true;
}

bool SDR_reset(void *driver, void *params)
{
    Sdr_gpioCfg *sdr_gpioCfg = (Sdr_gpioCfg *)driver;
    if (OcGpio_write(&sdr_gpioCfg->pin_sdr_reset_in, false) <= OCGPIO_FAILURE) {
        return false;
    }
    Task_sleep(100);
    if (OcGpio_write(&sdr_gpioCfg->pin_sdr_reset_in, true) <= OCGPIO_FAILURE) {
        return false;
    }
    return true;
}
