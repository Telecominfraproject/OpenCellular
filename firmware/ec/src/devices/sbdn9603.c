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
#include "devices/uart/sbd.h"
#include "inc/common/global_header.h"
#include "inc/subsystem/obc/obc.h"
#include "inc/subsystem/sdr/sdr.h" /* temporary - Only required for 12v enable */
#include "helpers/array.h"
#include "platform/oc-sdr/schema/schema.h"
#include "registry/SSRegistry.h"

#include <ti/drivers/UART.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             MACROS DEFINITION
//*****************************************************************************
#define OBC_TASK_PRIORITY 2
#define OBC_TASK_STACK_SIZE 2048

#define SBD_WRITE_TIMEOUT 500

/* TODO: move to helper? */
#define STATIC_STRLEN(s) (ARRAY_SIZE(s) - 1)

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************

extern OCSubsystem *ss_reg[];
static UART_Handle uartIridium;
static SBD_Handle s_hSbd = NULL;

static UART_Handle open_comm(const Iridium_Cfg *iridium)
{
    DEBUG("Resetting Iridium module\n");

    OcGpio_configure(&iridium->pin_enable,
                     OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&iridium->pin_nw_avail, OCGPIO_CFG_INPUT);

    /* reset - for proper reset, Iridium should be disabled for ~2s */
    OcGpio_write(&iridium->pin_enable, false); /* Just to be sure it's low */
    Task_sleep(2100); // TODO: should be ~2s
    OcGpio_write(&iridium->pin_enable, true);
    Task_sleep(200); // TODO: idk...probably doesn't need to be long

    // Open Iridium UART
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 19200;
    uartParams.dataLength = UART_LEN_8;
    uartParams.parityType = UART_PAR_NONE;
    uartParams.stopBits = UART_STOP_ONE;
    uartParams.writeTimeout = SBD_WRITE_TIMEOUT;
    return UART_open(iridium->uart, &uartParams);
}

ReturnStatus sbd_init(const Iridium_Cfg *iridium)
{
    uartIridium = open_comm(iridium);
    if (!uartIridium) {
        return RETURN_NOTOK;
    }

    /* Initialize SBD layers */
    //    const SbdCallbackList cbList = {
    //        .sbdring = sbdring_cb,
    //        .ciev = sbdciev_cb,
    //    };
    s_hSbd = SBD_init(uartIridium, NULL, NULL);
    if (!s_hSbd) {
        return RETURN_NOTOK;
    }

    /* TODO: module verification? */
    if (!SBD_k(s_hSbd, SBD_FLOW_CONTROL_HW) /* Enable HW flow control */
        || !SBD_sbdmta(s_hSbd, true) /* Ring indication enable */
        || !SBD_sbdareg(s_hSbd, SBD_AREG_MODE_AUTO) /* Auto registration */
        || !SBD_cier(s_hSbd, true, false, true, false,
                     false)) { /* Service change indications */
        /* TODO: handle cleanup */
        s_hSbd = NULL;
        return RETURN_NOTOK;
    }

    return RETURN_OK;
}

bool sbd9603_get_queueLength(OBC_Iridium_Status_Data *pIridiumStatusData)
{
    pIridiumStatusData->outQueueLen = 25;
    return true;
}

bool sbd9603_get_lastError(OBC_Iridium_Status_Data *pIridiumStatusData)
{
    pIridiumStatusData->lastErr = (OBC_lastError){
        .src = ERR_RC_INTERNAL,
        .code = 5,
    };
    return true;
}

bool sbd9603_get_imei(OBC_Iridium_Status_Data *pIridiumStatusData)
{
    bool ret = true;
    if (!s_hSbd) {
        ret = false;
    }
    SbdcgsnInfo cgsnInfo;
    if (!SBD_cgsn(s_hSbd, &cgsnInfo)) {
        ret = false;
    }
    pIridiumStatusData->imei = strtoull(cgsnInfo.imei, NULL, 10);
    return ret;
}

bool sbd9603_get_mfg(OBC_Iridium_Status_Data *pIridiumStatusData)
{
    bool ret = true;
    if (!s_hSbd) {
        ret = false;
    }
    SbdCgmiInfo cgmiInfo;
    if (!SBD_cgmi(s_hSbd, &cgmiInfo)) {
        ret = false;
    }
    strncpy(pIridiumStatusData->mfg, cgmiInfo.mfg,
            sizeof(pIridiumStatusData->mfg));
    return ret;
}

bool sbd9603_get_model(OBC_Iridium_Status_Data *pIridiumStatusData)
{
    bool ret = true;
    if (!s_hSbd) {
        ret = false;
    }
    SbdCgmmInfo cgmmInfo;
    if (!SBD_cgmm(s_hSbd, &cgmmInfo)) {
        ret = false;
    }
    /* Model string is verbose - if it's 9600 fam, replace with shorter
	 * model number since we only have 4 characters */
    char *model = cgmmInfo.model;
    const char fam_str[] = "IRIDIUM 9600 Family";
    if (strncmp(model, fam_str, STATIC_STRLEN(fam_str)) == 0) {
        model = "96xx";
    }
    strncpy(pIridiumStatusData->model, model,
            sizeof(pIridiumStatusData->model));
    return ret;
}

bool sbd9603_get_signalqual(OBC_Iridium_Status_Data *pIridiumStatusData)
{
    bool ret = true;
    if (!s_hSbd) {
        ret = false;
    }
    SbdcsqInfo csqInfo;
    if (!SBD_csqf(s_hSbd, &csqInfo)) {
        ret = false;
    }
    pIridiumStatusData->rssi = csqInfo.rssi;
    return ret;
}

bool sbd9603_get_regStatus(OBC_Iridium_Status_Data *pIridiumStatusData)
{
    bool ret = true;
    if (!s_hSbd) {
        ret = false;
    }
    if (!SBD_sbdregRead(s_hSbd, &pIridiumStatusData->regStat)) {
        ret = false;
    }
    return ret;
}

#include "helpers/memory.h"

static void loopback_test(SBD_Handle hSbd, bool debugLogs)
{
    static int loopCount = 1;
    static unsigned int ticks = 0;

    if (ticks == 0)
        ticks = Clock_getTicks();

    if (debugLogs) {
        System_printf("Loop %d\n", loopCount);
    }

    // Generate a random string
    static char msg[300];
    const int msgLen = 1 + (rand() % (sizeof(msg)));

    for (int i = 0; i < msgLen; ++i) {
        msg[i] = rand() % 256;
    }

    if (!SBD_sbdwb(hSbd, msg, msgLen)) {
        System_abort("SBDWB Failed");
    }

    if (!SBD_sbdtc(hSbd)) {
        System_abort("SBDTC Failed");
    }

    char ret[sizeof(msg)] = {};
    if (SBD_sbdrb(hSbd, ret, msgLen) < msgLen) {
        System_abort("SBDRB Failed");
    }

    if (memcmp(msg, ret, msgLen) != 0) {
        printMemory(msg, msgLen);
        System_printf("\n");
        printMemory(ret, msgLen);
        System_abort("Messages don't match");
    }

    // Only output every few iterations so printfs don't slow down test
    if (loopCount % 10 == 0) {
        if (debugLogs) {
            System_printf("Time: %d\n", Clock_getTicks() - ticks);
            System_flush();
        }
        ticks = Clock_getTicks();
    }
    loopCount++;
    OCSubsystem *ss = ss_reg[OC_SS_OBC];
    /* FIXME: hack to keep thread alive so we keep looping */
    Semaphore_post(ss->sem);
}
