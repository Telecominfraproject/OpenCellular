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
static Char sdrTaskStack[SDR_TASK_STACK_SIZE];

OCSubsystem ssSdr = {
    .taskStackSize = SDR_TASK_STACK_SIZE,
    .taskPriority = SDR_TASK_PRIORITY,
    .taskStack = sdrTaskStack,
};

/* SDR device config */
extern void *sys_config[];
#define SDR ((Sdr_Cfg *)sys_config[OC_SS_SDR])

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
void sdr_pwr_control(uint8_t control)
{
    /* Using configure instead of 'write' here since this pin is shared with
     * OBC and we don't want to configure then write if OBC has already
     * configured it (it'll reset Iridium that way) */
    if (control == OC_SDR_ENABLE) {
        OcGpio_configure(&SDR->pin_trxfe_12v_onoff, OCGPIO_CFG_OUTPUT |
                                                    OCGPIO_CFG_OUT_HIGH);
    } else {
        OcGpio_configure(&SDR->pin_trxfe_12v_onoff, OCGPIO_CFG_OUTPUT |
                                                    OCGPIO_CFG_OUT_LOW);
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
static void sdr_control_ioexpander(uint8_t control)
{
    if (control == OC_SDR_ENABLE) {
        OcGpio_write(&SDR->pin_rf_fe_io_reset, true);
    } else {
        OcGpio_write(&SDR->pin_rf_fe_io_reset, false);
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
static void sdr_control_device(uint8_t control)
{
    if (control == OC_SDR_ENABLE) {
        OcGpio_write(&SDR->pin_sdr_reset_in, true);
    } else {
        OcGpio_write(&SDR->pin_sdr_reset_in, false);
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
static void sdr_control_reset(uint8_t control)
{
    if (control == OC_SDR_ENABLE) {
        OcGpio_write(&SDR->pin_ec_trxfe_reset, true);
    } else {
        OcGpio_write(&SDR->pin_ec_trxfe_reset, false);
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
static ReturnStatus sdr_fx3_reset(void)
{
    /*TODO: We need to figure out a way for configuring PCA pins on Intel reset.*/
    OcGpio_configure(&SDR->pin_fx3_reset, OCGPIO_CFG_OUTPUT);

    if (OcGpio_write(&SDR->pin_fx3_reset, false) < OCGPIO_SUCCESS) {
        return RETURN_NOTOK;
    }

    /* Provide small delay */
    SysCtlDelay(5);

    if (OcGpio_write(&SDR->pin_fx3_reset, true) < OCGPIO_SUCCESS) {
        return RETURN_NOTOK;
    }
    return RETURN_OK;
}

bool SDR_fx3Reset(void *driver, void *params) {
    return (sdr_fx3_reset() == RETURN_OK);
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
bool SDR_Init(void *return_buf)
{
    /* Initialize IO pins */
    OcGpio_configure(&SDR->pin_sdr_reg_ldo_pgood, OCGPIO_CFG_INPUT);
    OcGpio_configure(&SDR->pin_rf_fe_io_reset, OCGPIO_CFG_OUTPUT |
                                                OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&SDR->pin_sdr_reset_in, OCGPIO_CFG_OUTPUT |
                                             OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&SDR->pin_ec_trxfe_reset, OCGPIO_CFG_OUTPUT |
                                                OCGPIO_CFG_OUT_LOW);

    /* Power On SDR  */
    sdr_pwr_control(OC_SDR_ENABLE);

    /* Read EEPROM */
    eeprom_init(SDR->eeprom_inventory);

    NOP_DELAY();

    /* Move IO expander out of reset*/
    sdr_control_ioexpander(OC_SDR_ENABLE);

    /*Enable SDR devices*/
    sdr_control_device(OC_SDR_ENABLE);

    /* Added in SDR beacuse of U30 (SN74LV08A) */
    /*RF_IO_RESET signal depends on this TRXFE_RESET*/
    /*Enable TRX_FE_RESET*/
    sdr_control_reset(OC_SDR_ENABLE);
    /*Move*/

    NOP_DELAY();

    /* Check Powergood status(SDR_REG_LDO_PGOOD) */
    if(OcGpio_read(&SDR->pin_sdr_reg_ldo_pgood)) {
        LOGGER("SDR:INFO:: PowerGood Status is OK.\n");
    }
    else {
        LOGGER("SDR:INFO:: PowerGood Status is NOT OK.\n");
    }

    /* Make FX3 out of reset mode */
    ReturnStatus status = sdr_fx3_reset();
    NOP_DELAY();

    LOGGER("SDR:INFO:: FX3 Reset is %s.\n",
           ((status == RETURN_OK) ? "done" : "not done"));

    return true;
}

bool Sdr_InventoryGetStatus(void *driver, unsigned int param_id,
                            void *return_buf) {

    OCMPSubsystem *ss = (OCMPSubsystem *)driver;

    switch (param_id) {
        case 0: /* TODO: gross magic number */
            memset(return_buf, '\0', OC_SDR_BOARD_INFO_SIZE + 1);
            if (eeprom_read_board_info(*ss, return_buf) == RETURN_OK) {
                return true;
            }
            LOGGER_DEBUG("SDR:INFO:: Board id: %s\n", return_buf);
            break;
        default:
            LOGGER_ERROR("SDR:ERROR::Unknown param %u\n", param_id);
            break;
    }
    return false;
}

bool SDR_reset(void *driver, void *params) {
    if (OcGpio_write(&SDR->pin_sdr_reset_in, false) <= OCGPIO_FAILURE) {
        return false;
    }
    Task_sleep(100);
    if (OcGpio_write(&SDR->pin_sdr_reset_in, true) <= OCGPIO_FAILURE) {
        return false;
    }
    return true;
}
