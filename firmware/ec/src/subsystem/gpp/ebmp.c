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
#include "inc/subsystem/gpp/ebmp.h"

#include "Board.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/common/global_header.h"
#include "inc/subsystem/gpp/gpp.h"
#include "inc/utils/util.h"

#include <driverlib/gpio.h>
#include <inc/hw_memmap.h>
#include <ti/drivers/GPIO.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
/* Task Handle */
static Task_Struct ebmpTask;
static Char ebmpTaskStack[EBMP_TASK_STACK_SIZE];

/* Semaphore Handle */
Semaphore_Handle semStateHandle;

/* Clock Handle */
Clock_Handle bootProgressClk;

//*****************************************************************************
//                             LOCAL VARIABLES
//*****************************************************************************
/*
 * apUp - This varible will be set based on AP's boot progress pins are
 * set after watchdog process has been initiated in AP. Zero only when AP
 * is either in shutdown or under reset or booting up, or made to restart by EC
 */
extern uint8_t apUp;

/* apState&oldState - Holds any states from any of apStates T0-T7 states */
volatile int apState = STATE_INVALID;
volatile int oldState = STATE_INVALID;

volatile int secondIteration = 0;
volatile uint32_t stateChangeWaitTimeMax = 10000;
volatile uint32_t stateChangeTimeElapsed = 0;
static OcGpio_Pin *pin_soc_pltrst_n;
static OcGpio_Pin *pin_soc_corepwr_ok;
static OcGpio_Pin *pin_ap_boot_alert1;
static OcGpio_Pin *pin_ap_boot_alert2;

//*****************************************************************************
//                                EXTERN FUNCTIONS
//*****************************************************************************
extern void watchdog_reset_ap(void);

/******************************************************************************
 **    FUNCTION NAME   : ebmp_restart_timer
 **    DESCRIPTION     : Every time you reset the timer when
 **                         - New STATE has been changed and to configure the
 **                           timer for new timeout
 **                         - Watchdog on AP has sent the watchdog command
 **                           to state, it's alive so restarting the timer again.
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_restart_timer(uint32_t counts)
{
#if 0
    stateChangeWaitTimeMax = counts;
    stateChangeTimeElapsed = 0;
    if (Clock_isActive(bootProgressClk))
        Clock_stop(bootProgressClk);
    //Clock_setPeriod(bootProgressClk, 10);
    Clock_start(bootProgressClk);
#endif
}

/******************************************************************************
 **    FUNCTION NAME   : ebmp_boot_monitor_timeout
 **    DESCRIPTION     : This function will periodically checks following two
 **                      things:
 **                         - If the current state of GPP has exeeded the
 **                           time out?
 **                         - Within limit of time if watchdog command has been
 **                           recieved or not?
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_boot_monitor_timeout(void)
{
    if (apState != STATE_T7) {
        if (apState == oldState) {
            if (stateChangeTimeElapsed++ > stateChangeWaitTimeMax) {
                /**********************************************
                 * send the message to the Big Brother to take
                 * proper action. It may query about the System
                 * state and what went bad? And then stop the
                 * timer.
                 *********************************************/
                // send_msg_big_brother();
                Clock_stop(bootProgressClk);
                //watchdog_reset_ap();
            }
        } else {
            /*****************************************************
             * We are here if the state has been changed to the
             * new one.
             * Make sure the transition happens in sequential &
             * incremental manner only. T0 -> T1 -> T2 etc
             * If the state has been changed then stop the timer.
             *****************************************************/
            if (apState > oldState) {
                oldState = apState;
                Clock_stop(bootProgressClk);
            }
        }
    }
}

/******************************************************************************
 **    FUNCTION NAME   : ebmp_check_soc_plt_reset
 **    DESCRIPTION     : Callback function for the PLT RST to check whether
 **                      AP is out of reset or not.
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_check_soc_plt_reset(void)
{
    //Resetting Iteration counter for GPIO Toggle to zero.
    secondIteration = 0;
    if (OcGpio_read(pin_soc_pltrst_n)) {
        /*************************************************************
         *  Semaphore to be released to the EBMP telling the SoC is
         * out of reset
         *************************************************************/
        apState = oldState = STATE_T0;
        Semaphore_post(semStateHandle);
    } else {
        /*
         * If Power is OK and plt rst is low, it means someone or AP
         * has initiated the reset sequence. Make the State of the GPP,
         * initialized to STATE_T0 to indicate.
         */
#if 0
        apState = oldState = STATE_INVALID;
#else
        if (OcGpio_read(pin_soc_corepwr_ok)) {
            apState = oldState = STATE_T0;
        } else {
            apState = oldState = STATE_INVALID;
        }
#endif
    }
}

/******************************************************************************
 **    FUNCTION NAME   : ebmp_check_boot_pin_status
 **    DESCRIPTION     : Check the status of the Pins PL3 and PE3 and desides
 **                      state as AP. State is reflected on apState.
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_check_boot_pin_status(void)
{
    int32_t bootstatus_1 = 0;
    int32_t bootstatus_2 = 0;

    bootstatus_1 = OcGpio_read(pin_ap_boot_alert1);
    bootstatus_2 = OcGpio_read(pin_ap_boot_alert2);
    if (!secondIteration) {
        if (bootstatus_2 == 0) {
            if (bootstatus_1) {
                apState = STATE_T2; //  s5_09(PL3) = 0 , s0_59(PE3) = 1
            } else {
                apState = STATE_T1; // s5_09(PL3) = 0 , s0_59(PE3) = 0
            }
        } else {
            if (bootstatus_1) {
                apState = STATE_T3; // s5_09(PL3) = 1 , s0_59(PE3) = 1
            } else {
                apState = STATE_T4; // s5_09(PL3) = 1 , s0_59(PE3) = 0,
                secondIteration = 1;
            }
        }
    } else {
        if (bootstatus_2 == 0) {
            if (bootstatus_1) {
                apState = STATE_T6; // 5_09(PL3) = 0, s0_59(PE3) = 1
            } else {
                apState = STATE_T5; // s5_09(PL3) = 0, s0_59(PE3) = 0
            }
        } else {
            if (bootstatus_1) {
                apState = STATE_T7; // s5_09(PL3) = 1, s0_59(PE3) = 1
                secondIteration = 0;
            } else {
                apState = STATE_INVALID; // s5_09(PL3) = 1, s0_59(PE3) = 0,
            }
        }
    }
}

/*****************************************************************************
 * Internal IRQ handler - reads in triggered interrupts and dispatches CBs
 *****************************************************************************/
static void ebmp_handle_irq(void *context)
{
    apBootMonitor alertPin = (apBootMonitor)context;
    if (alertPin == AP_RESET) {
        ebmp_check_soc_plt_reset();
    } else {
        ebmp_check_boot_pin_status();
        Semaphore_post(semStateHandle);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : ebmp_init
 **    DESCRIPTION     : Function to register GPIO interrupts
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_init(Gpp_gpioCfg *driver)
{
    pin_ap_boot_alert1 = &driver->pin_ap_boot_alert1;
    pin_ap_boot_alert2 = &driver->pin_ap_boot_alert2;
    pin_soc_pltrst_n = &driver->pin_soc_pltrst_n;
    pin_soc_corepwr_ok = &driver->pin_soc_corepwr_ok;

    if (pin_ap_boot_alert1->port) {
        const uint32_t pin_evt_cfg =
                OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES;
        if (OcGpio_configure(pin_ap_boot_alert1, pin_evt_cfg) <
            OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }
        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(pin_ap_boot_alert1, ebmp_handle_irq,
                         (void *)AP_BOOT_PROGRESS_MONITOR_1);
    }

    if (pin_ap_boot_alert2->port) {
        const uint32_t pin_evt_cfg =
                OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES;
        if (OcGpio_configure(pin_ap_boot_alert2, pin_evt_cfg) <
            OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }
        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(pin_ap_boot_alert2, ebmp_handle_irq,
                         (void *)AP_BOOT_PROGRESS_MONITOR_2);
    }

    if (pin_soc_pltrst_n->port) {
        const uint32_t pin_evt_cfg =
                OCGPIO_CFG_INPUT | OCGPIO_CFG_INT_BOTH_EDGES;
        if (OcGpio_configure(pin_soc_pltrst_n, pin_evt_cfg) < OCGPIO_SUCCESS) {
            return RETURN_NOTOK;
        }
        /* Use a threaded interrupt to handle IRQ */
        ThreadedInt_Init(pin_soc_pltrst_n, ebmp_handle_irq, (void *)AP_RESET);
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : ebmp_task_init
 **    DESCRIPTION     : Function to setup pre-requisites of the task
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_task_init(void)
{
    semStateHandle = Semaphore_create(0, NULL, NULL);
}

/*****************************************************************************
 **    FUNCTION NAME   : ebmp_task_fxn
 **    DESCRIPTION     : Function that maintains the state of the SOC.
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_task_fxn(UArg a0, UArg a1)
{
    ebmp_task_init();

    /* Regsiter GPIO interrupts */
    //ebmp_init();

    while (1) {
        Semaphore_pend(semStateHandle, BIOS_WAIT_FOREVER);
        /*
         * Check what is the state of the GPP AP and if it is fully
         * booted then spawn another semaphore for gppMonitoring task.
         */
        switch (apState) {
            /*
             * We are doing only one thing in states T0 to T6.
             * Just keep checking if the state transitioning happens in
             * time or not by starting timer. If the time elapses beyond
             * some point, reset the AP to make it reboot.
             */
            // What is the difference between T0 to T1? Nothing! So better
            // to start counter with T1
            case STATE_T0: {
                //oldState = apState;
                ebmp_restart_timer(500);
                break;
            }
            case STATE_T1: {
                DEBUG("EBMP:INFO::STATE_T1 AP out of reset.\n");
                ebmp_restart_timer(500);
                break;
            }
            case STATE_T2: {
                DEBUG("EBMP:INFO::STATE_T2 Coreboot ROM Stage.\n");
                ebmp_restart_timer(500);
                break;
            }
            case STATE_T3: {
                DEBUG("EBMP:INFO::STATE_T3 Coreboot RAM stage.\n");
                ebmp_restart_timer(500);
                break;
            }
            case STATE_T4: {
                DEBUG("EBMP:INFO::STATE_T4 Coreboot configures SeaBIOS to load OS.\n");
                ebmp_restart_timer(1000);
                /*
                 * Based on Boot_From_Recovery (true or false) we will drive
                 * GPIO_S5[09].
                 * GPIO_S5[09] -> 1. Recovery boot.
                 */
                //bootOption();
                break;
            }
            case STATE_T5: {
                DEBUG("EBMP:INFO::STATE_T5 SeaBIOS loads OS.\n");
                ebmp_restart_timer(10000);
                /*
                 * Based on Boot_From_Recovery (true or false) we will drive
                 * GPIO_S5[09].
                 * GPIO_S5[09] -> 1. Recovery boot.
                 */
                //bootOption();
                break;
            }
            case STATE_T6: {
                DEBUG("EBMP:INFO::STATE_T6 Watchdog Daemon started.\n");
                /* OC-Watchdog started */
                ebmp_restart_timer(10000);
                break;
            }
                /*Fully booted */
            case STATE_T7: {
                DEBUG("EBMP:INFO:: STATE_T7 AP Watchdog daemon process responds to EP request.\n");
                //Semaphore_post(apStateSem);
                /* Stop timer. It will be used by OCWatchdog */
                // stop_timer();
                apUp = 1;
                // restart_timer(100000);
                // Semaphore_OCWatchdog();
                break;
            }
            default: {
                DEBUG("EBMP:ERROR:: Invalid state\n");
            }
        }
        DEBUG("EBMP:INFO:: Boot Monitor Pin 1 [PE3] : %d Boot Monitor Pin 2 [PL3] : %d SOC PLTRST : %d.\n",
              OcGpio_read(pin_ap_boot_alert1) ? 1 : 0,
              OcGpio_read(pin_ap_boot_alert2) ? 1 : 0,
              OcGpio_read(pin_soc_pltrst_n) ? 1 : 0);
        oldState = apState;
    }
}

/*****************************************************************************
 **    FUNCTION NAME   : ebmp_create_task
 **    DESCRIPTION     : Task creation function for the EBMP.
 **    ARGUMENTS       : None
 **    RETURN TYPE     : None
 *****************************************************************************/
void ebmp_create_task(void)
{
    Task_Params taskParams;

    Task_Params_init(&taskParams);
    taskParams.stack = ebmpTaskStack;
    taskParams.stackSize = EBMP_TASK_STACK_SIZE;
    taskParams.priority = EBMP_TASK_PRIORITY;
    Task_construct(&ebmpTask, ebmp_task_fxn, &taskParams, NULL);
}
