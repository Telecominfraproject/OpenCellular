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
#include "inc/subsystem/rffe/rffe_sensor.h"
#include "inc/subsystem/rffe/rffe_ctrl.h"
#include "inc/subsystem/rffe/rffe_powermonitor.h"

#include "Board.h"
#include "inc/subsystem/sdr/sdr.h"
#include "inc/common/system_states.h"
#include "inc/utils/util.h"
#include "registry/SSRegistry.h"

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
/* Global Task Configuration Variables */
static Char rffeTaskStack[RFFE_TASK_STACK_SIZE];

OCSubsystem ssRf = {
    .taskStackSize = RFFE_TASK_STACK_SIZE,
    .taskPriority = RFFE_TASK_PRIORITY,
    .taskStack = rffeTaskStack,
};

/* RFFE device config */
extern void *sys_config[];
#define RFFE ((Fe_Cfg *)sys_config[OC_SS_RF])

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
void rffe_pwr_control(uint8_t control)
{
    if (control == OC_FE_ENABLE) {
        OcGpio_write(&RFFE->pin_fe_12v_ctrl, true);
    } else {
        OcGpio_write(&RFFE->pin_fe_12v_ctrl, false);
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
bool rffe_pre_init(void *returnValue)
{
    /* Initialize IO pins */
    OcGpio_configure(&RFFE->pin_rf_pgood_ldo, OCGPIO_CFG_INPUT);
    OcGpio_configure(&RFFE->pin_fe_12v_ctrl, OCGPIO_CFG_OUTPUT |
                                             OCGPIO_CFG_OUT_LOW);

    /* Read EEPROM */
    eeprom_init(RFFE->eeprom_inventory);

    /* RF power on */
    rffe_pwr_control(OC_FE_ENABLE);

    NOP_DELAY();

    /* Check Powergood status(SDR_REG_LDO_PGOOD) */
    if(OcGpio_read(&RFFE->pin_rf_pgood_ldo)) {
        LOGGER("RFFE:INFO:: PowerGood Status is OK.\n");
    }
    else {
        LOGGER("RFFE:INFO:: PowerGood Status is NOT OK.\n");
    }

    /* Initilize FE IO Expander GPIO Controls (those not already controlled
     * by a driver) */
    const Fe_Cfg *fe_cfg = sys_config[OC_SS_RF];

    OcGpio_configure(&fe_cfg->fe_ch2_gain_cfg.pin_ch1_2g_lb_band_sel_l,
                     OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&fe_cfg->fe_ch2_lna_cfg.pin_ch1_rf_pwr_off,
                     OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&fe_cfg->fe_watchdog_cfg.pin_ch2_rf_pwr_off,
                     OCGPIO_CFG_OUTPUT);

    /* TODO: might be cleaner to move into watchdog driver (2x init is ok) */
    OcGpio_configure(&fe_cfg->fe_watchdog_cfg.pin_aosel_fpga,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    OcGpio_configure(&fe_cfg->fe_watchdog_cfg.pin_copol_fpga,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);

    return true;
}

bool rffe_post_init(eSubSystemStates *ssState)
{
    ReturnStatus status = RETURN_OK;

    status |= rffe_ctrl_configure_power_amplifier(RFFE_CHANNEL1,
                                                  RFFE_ACTIVATE_PA);

    status |= rffe_ctrl_configure_power_amplifier(RFFE_CHANNEL2,
                                                  RFFE_ACTIVATE_PA);

    //rffe_powermonitor_createtask();

    /*Updating subsystem sate info*/
    LOGGER_DEBUG("RFFE:INFO:: Subsystem device check and configuration is %s\n",
            ((status == RETURN_OK) ? "successful" : "unsuccessful"));

    if (status == RETURN_OK) {
        *ssState = SS_STATE_CFG;
    } else {
        *ssState = SS_STATE_FAULTY;
        return false;
    }

    return true;
}

bool RFFE_reset(void *driver, void *params)
{
    /* TODO: this is the same line we use to reset the SDR - is this really
     * what we want to be doing here? */
    const Sdr_Cfg *cfg = sys_config[OC_SS_SDR];
    if (OcGpio_write(&cfg->pin_ec_trxfe_reset, false) < OCGPIO_SUCCESS) {
        return false;
    }
    Task_sleep(100);
    if (OcGpio_write(&cfg->pin_ec_trxfe_reset, true) < OCGPIO_SUCCESS) {
        return false;
    }
    return true;
}

bool RFFE_InventoryGetStatus(void *driver, unsigned int param_id,
                             void *return_buf) {
    switch (param_id) {
        case 0: /* TODO: gross magic number */
            memset(return_buf, '\0', OC_RFFE_BOARD_INFO_SIZE + 1);
            if (eeprom_read_board_info((OCMPSubsystem)driver, return_buf)
                    == RETURN_OK) {
                return true;
            }
            LOGGER_DEBUG("RFFE:INFO:: Board id: %s\n", return_buf);
            break;
        default:
            LOGGER_ERROR("RFFE:ERROR::Unknown param %u\n", param_id);
            break;
    }
    return false;
}
