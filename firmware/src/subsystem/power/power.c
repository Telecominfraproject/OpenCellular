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
#include "Board.h"
#include "src/registry/SSRegistry.h"
#include <ti/sysbios/BIOS.h>

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define POWERCTL_TASK_PRIORITY           2
#define POWERCTL_TASK_STACK_SIZE        1024

/*****************************************************************************
 *                             HANDLE DEFINITIONS
 *****************************************************************************/
/* Global Task Configuration Variables */

static Char POWERCTLTaskStack[POWERCTL_TASK_STACK_SIZE];

OCSubsystem ssPower = {
    .taskStackSize = POWERCTL_TASK_STACK_SIZE,
    .taskPriority = POWERCTL_TASK_PRIORITY,
    .taskStack = POWERCTLTaskStack,
};

extern void *sys_config[];
#define PWR ((Power_Cfg *)sys_config[OC_SS_PWR])

/*****************************************************************************
 **    FUNCTION NAME   : pwr_pre_init
 **
 **    DESCRIPTION     : Initializes the power subsystem gpio's.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : bool
 **
 *****************************************************************************/
bool pwr_pre_init()
{
       /* Initialize IO pins */
        OcGpio_configure(&PWR->pin_int_bat_prsnt, OCGPIO_CFG_INPUT);
        OcGpio_configure(&PWR->pin_ext_bat_prsnt, OCGPIO_CFG_INPUT);
        OcGpio_configure(&PWR->pin_ec_pd_pwrgd_ok, OCGPIO_CFG_INPUT);
        OcGpio_configure(&PWR->pin_solar_aux_prsnt_n, OCGPIO_CFG_INPUT);
        OcGpio_configure(&PWR->pin_poe_prsnt_n, OCGPIO_CFG_INPUT);
        OcGpio_configure(&PWR->pin_lt4275_ec_nt2p, OCGPIO_CFG_INPUT);
        OcGpio_configure(&PWR->pin_necpse_rst, OCGPIO_CFG_OUTPUT |
                                               OCGPIO_CFG_OUT_HIGH);
        OcGpio_configure(&PWR->pin_lt4015_i2c_sel, OCGPIO_CFG_OUTPUT);
        OcGpio_configure(&PWR->pin_int_bat_prsnt, OCGPIO_CFG_INPUT);
        OcGpio_configure(&PWR->pin_ext_bat_prsnt, OCGPIO_CFG_INPUT);

        //Enable PSE device.
        ltc4274_enable(true);
        return true;
}

/*****************************************************************************
 **    FUNCTION NAME   : pwr_post_init
 **
 **    DESCRIPTION     : Initializes the power subsystem gpio's.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : bool
 **
 *****************************************************************************/
bool pwr_post_init()
{
    return true;
}
