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
#include "inc/subsystem/sync/sync.h"

#include "inc/utils/util.h"
#include "registry/SSRegistry.h"

#include <ti/sysbios/BIOS.h>

#include <stdlib.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
static Sync_gpioCfg s_sync_gpiocfg;

/* TODO: Change current implementaion of Polling based approach to interrupt
 * based driven in future */
static Char syncGpsTaskStack[SYNC_GPS_TASK_STACK_SIZE];
Semaphore_Handle gpsSem;

static ReturnStatus SYNC_GpsCheckLock(Sync_gpioCfg *sync_gpiocfg,
                                      gpsStatus *gpsStatus)
{
    /* Get the "lock OK" status from LTE-LITE GPIO pin */
    int locked = OcGpio_read(&(sync_gpiocfg->pin_r_lock_ok_ioexp));
    if (locked < OCGPIO_SUCCESS) {
        return RETURN_NOTOK;
    }

    if (locked) {
        DEBUG("SYNC:INFO:: GPS is locked\n");
        *gpsStatus = GPS_LOCKED;
    } else {
        DEBUG("SYNC:INFO:: GPS is not locked\n");
        *gpsStatus = GPS_NOTLOCKED;
    }

    return RETURN_OK;
}

/*****************************************************************************
 **    FUNCTION NAME   : SYNC_GpsClkFxn
 **
 **    DESCRIPTION     : Handles the Clock Function for SYNC GPS polling.
 **
 **    ARGUMENTS       : a0 - not used
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void SYNC_GpsClkFxn(UArg arg0)
{
    Semaphore_post(gpsSem);
}

/*****************************************************************************
 **    FUNCTION NAME   : SYNC_GpsTaskFxn
 **
 **    DESCRIPTION     : Handles SYNC GPS polling.
 **
 **    ARGUMENTS       : a0, a1 - not used
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void SYNC_GpsTaskFxn(UArg a0, UArg a1)
{
    while (true) {
        if (Semaphore_pend(gpsSem, BIOS_WAIT_FOREVER)) {
            gpsStatus gpsStatus;
            SYNC_GpsCheckLock(&s_sync_gpiocfg, &gpsStatus);
            DEBUG("SYNC:INFO:: GPS is %s.\n",
                  (gpsStatus == GPS_LOCKED) ? "Locked" : "Not Locked");
        }
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : SYNC_GpsTaskInit
 **
 **    DESCRIPTION     : Initializes the SYNC GPS task.
 **
 **    ARGUMENTS       : None
 **
 **    RETURN TYPE     : None
 **
 *****************************************************************************/
static void SYNC_GpsTaskInit(void)
{
    /* Create a semaphore */
    gpsSem = Semaphore_create(0, NULL, NULL);
    if (!gpsSem) {
        DEBUG("SYNC::Can't create GPS Polling semaphore\n");
        return;
    }

    /* Create a task */
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = syncGpsTaskStack;
    taskParams.stackSize = SYNC_GPS_TASK_STACK_SIZE;
    taskParams.priority = SYNC_GPS_TASK_PRIORITY;
    Task_Handle task = Task_create(SYNC_GpsTaskFxn, &taskParams, NULL);
    if (!task) {
        DEBUG("SYNC::FATAL: Unable to start Gps Polling task\n");
        Semaphore_delete(&gpsSem);
        return;
    }

    /* Create a clock */
    Clock_Params clkParams;
    Clock_Params_init(&clkParams);
    clkParams.period = 10000;
    clkParams.startFlag = FALSE;
    /* Create a periodic Clock Instance with initial timeout= 10000(10 Secs)
     * and period = 10000(10 Secs) system time units */
    Clock_Handle clk = Clock_create((Clock_FuncPtr)SYNC_GpsClkFxn, 10000,
                                    &clkParams, NULL);
    if (!clk) {
        DEBUG("SYNC::Can't create GPS Polling clock\n");
        Semaphore_delete(&gpsSem);
        Task_delete(&task);
        return;
    }

    /*Start the Clock */
    Clock_start(clk);
}

bool SYNC_GpsStatus(void *driver, unsigned int param_id, void *return_buf)
{
    switch (param_id) {
        case 0: /* TODO: gross magic number */
            if (SYNC_GpsCheckLock(driver, return_buf) == RETURN_OK) {
                DEBUG("SYNC:INFO:: GPS is %s.\n",
                      (*(gpsStatus *)return_buf == GPS_LOCKED) ? "Locked" :
                                                                 "Not Locked");
                return true;
            }
            break;
        default:
            LOGGER_ERROR("SYNC:ERROR::Unknown param %u\n", param_id);
            break;
    }
    return false;
}

bool SYNC_reset(void *driver, void *params)
{
    Sync_gpioCfg *sync_gpiocfg = (Sync_gpioCfg *)driver;
    if (OcGpio_write(&sync_gpiocfg->pin_ec_sync_reset, false) <=
        OCGPIO_FAILURE) {
        return false;
    }
    Task_sleep(100);
    if (OcGpio_write(&sync_gpiocfg->pin_ec_sync_reset, true) <=
        OCGPIO_FAILURE) {
        return false;
    }
    return true;
}

bool SYNC_Init(void *driver, void *return_buf)
{
    Sync_gpioCfg *sync_gpiocfg = (Sync_gpioCfg *)driver;
    /* Initialize IO pins */
    OcGpio_configure(&sync_gpiocfg->pin_ec_sync_reset,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    OcGpio_configure(&sync_gpiocfg->pin_spdt_cntrl_lvl, OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&sync_gpiocfg->pin_warmup_survey_init_sel,
                     OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&sync_gpiocfg->pin_r_phase_lock_ioexp, OCGPIO_CFG_INPUT);
    OcGpio_configure(&sync_gpiocfg->pin_r_lock_ok_ioexp, OCGPIO_CFG_INPUT);
    OcGpio_configure(&sync_gpiocfg->pin_r_alarm_ioexp, OCGPIO_CFG_INPUT);
    OcGpio_configure(&sync_gpiocfg->pin_12v_reg_enb,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH);
    OcGpio_configure(&sync_gpiocfg->pin_temp_alert, OCGPIO_CFG_INPUT);
    OcGpio_configure(&sync_gpiocfg->pin_spdt_cntrl_lte_cpu_gps_lvl,
                     OCGPIO_CFG_OUTPUT);
    OcGpio_configure(&sync_gpiocfg->pin_init_survey_sel, OCGPIO_CFG_OUTPUT);

    /*Initiaize the local static driver config for the task*/
    s_sync_gpiocfg = *(Sync_gpioCfg *)driver;
    /* TODO: Launch task for GPS */
    SYNC_GpsTaskInit();
    return true;
}
