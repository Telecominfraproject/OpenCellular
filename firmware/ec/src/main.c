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
#include "Board.h"
#include "inc/common/bigbrother.h"
#include "inc/common/global_header.h"

#include <driverlib/sysctl.h>
#include <ti/sysbios/BIOS.h>

#define xstr(a) str(a)
#define str(a) #a

//*****************************************************************************
//                             FUNCTION DECLARATIONS
//*****************************************************************************
extern int ethernet_start(void);

static void openCellular_init(void)
{
    LOGGER_DEBUG(
            "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    LOGGER_DEBUG(
            "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    LOGGER_DEBUG(
            "||||        ||||        ||||       |||||    ||||||  ||||       ||||       ||||  |||||||||  |||||||||  ||||  ||||  |||||||||        ||||        ||||\n");
    LOGGER_DEBUG(
            "||||  ||||  ||||  ||||  ||||  ||||||||||  |  |||||  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  ||||  ||||\n");
    LOGGER_DEBUG(
            "||||  ||||  ||||  ||||  ||||  ||||||||||  ||  ||||  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  ||||  ||||\n");
    LOGGER_DEBUG(
            "||||  ||||  ||||        ||||       |||||  |||  |||  ||||  |||||||||       ||||  |||||||||  |||||||||  ||||  ||||  |||||||||        ||||        ||||\n");
    LOGGER_DEBUG(
            "||||  ||||  ||||  ||||||||||  ||||||||||  ||||  ||  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  ||  ||||||\n");
    LOGGER_DEBUG(
            "||||  ||||  ||||  ||||||||||  ||||||||||  |||||  |  ||||  |||||||||  |||||||||  |||||||||  |||||||||  ||||  ||||  |||||||||  ||||  ||||  |||  |||||\n");
    LOGGER_DEBUG(
            "||||        ||||  ||||||||||       |||||  ||||||    ||||       ||||       ||||       ||||       ||||        ||||       ||||  ||||  ||||  ||||  ||||\n");
    LOGGER_DEBUG(
            "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    LOGGER_DEBUG(
            "|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||\n");
    LOGGER_DEBUG(
            "\nOCWare v" xstr(_FW_REV_MAJOR_) "." xstr(_FW_REV_MINOR_) "." xstr(
                    _FW_REV_BUGFIX_) "-" xstr(_FW_REV_TAG_) "\n");
    LOGGER_DEBUG("Build Date: " __DATE__ " " __TIME__ "\n\n");
}

static void exit_handler(int unused)
{
    /* Perform a full system reset if we fault,
     * hope it doesn't happen again */
    SysCtlReset();
}

/*Main Function */
int main(void)
{
    /* Install an exit handler to catch if we fault out */
    System_atexit(exit_handler);
    openCellular_init();
    /* Call board init functions */
    Board_initGeneral();
    Board_initGPIO();
    Board_initI2C();
    Board_initUSB(Board_USBDEVICE);
    Board_initUART();
    ethernet_start();
    bigbrother_createtask();
    /* Start BIOS */
    BIOS_start();
    return (0);
}
