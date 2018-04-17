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
#include "inc/subsystem/obc/obc.h"

#include "Board.h"
#include "devices/uart/sbd.h"
#include "inc/common/global_header.h"
#include "inc/subsystem/sdr/sdr.h" /* temporary - Only required for 12v enable */
#include "helpers/array.h"
#include "registry/SSRegistry.h"

#include <ti/drivers/UART.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>

#include <stdlib.h>
#include <string.h>

//*****************************************************************************
//                             MACROS DEFINITION
//*****************************************************************************
#define OBC_TASK_PRIORITY     2
#define OBC_TASK_STACK_SIZE   2048

#define SBD_WRITE_TIMEOUT     500

/* TODO: move to helper? */
#define STATIC_STRLEN(s) (ARRAY_SIZE(s) - 1)

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
OCSubsystem ssObc = {
    .taskStackSize = OBC_TASK_STACK_SIZE,
    .taskPriority = OBC_TASK_PRIORITY,
};

static UART_Handle uartIridium;
static SBD_Handle s_hSbd = NULL;

bool obc_pre_init(void *returnValue)
{
    /* TODO: temporary */
    extern const void *sys_config[];
    const Obc_Cfg *obc_cfg = sys_config[OC_SS_OBC];

    /* TODO: this is a problem - need 12V for Iridium (plus 5v reg enabled)
     * I'm not sold on OBC directly enabling these lines, but there isn't
     * a great alternative at this point */
    sdr_pwr_control(OC_SDR_ENABLE);
    if (obc_cfg->pin_pwr_en) {
        if (OcGpio_configure(obc_cfg->pin_pwr_en,
                             OCGPIO_CFG_OUTPUT | OCGPIO_CFG_OUT_HIGH)
                < OCGPIO_SUCCESS) {
            return false;
        }
    }
    return true;
}

static UART_Handle open_comm(const Iridium_Cfg *iridium)
{
    DEBUG("Resetting Iridium module\n");

    OcGpio_configure(&iridium->pin_enable, OCGPIO_CFG_OUTPUT |
                                           OCGPIO_CFG_OUT_LOW);
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

static ReturnStatus sbd_init(const Iridium_Cfg *iridium)
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
    if (!SBD_k(s_hSbd, SBD_FLOW_CONTROL_HW)             /* Enable HW flow control */
            || !SBD_sbdmta(s_hSbd, true)                /* Ring indication enable */
            || !SBD_sbdareg(s_hSbd, SBD_AREG_MODE_AUTO) /* Auto registration */
            || !SBD_cier(s_hSbd, true, false, true, false, false)) { /* Service change indications */
        /* TODO: handle cleanup */
        s_hSbd = NULL;
        return RETURN_NOTOK;
    }

    return RETURN_OK;
}

static ReturnStatus iridium_get_statusParams(
        eOBC_StatusParam paramID,
        OBC_Iridium_Status_Data *pIridiumStatusData)
{
    ReturnStatus status = RETURN_OK;

    /* See if this is a param that doesn't need to talk to Iridium */
    switch (paramID) {
        case IRIDIUM_NO_OUT_MSG:
            pIridiumStatusData->outQueueLen = 25;
            break;
        case IRIDIUM_LASTERR:
            pIridiumStatusData->lastErr = (OBC_lastError) {
                .src = ERR_RC_INTERNAL,
                .code = 5,
            };
            break;
        default:
            /* We don't care about the rest at this point */
            break;
    }

    /* If Iridium isn't initialized, don't try to talk to it */
    if (!s_hSbd) {
        return RETURN_NOTOK;
    }

    /* Check the params that DO require Iridium communication */
    switch (paramID) {
        case IRIDIUM_IMEI: {
            SbdcgsnInfo cgsnInfo;
            if (!SBD_cgsn(s_hSbd, &cgsnInfo)) {
                return RETURN_NOTOK;
            }
            pIridiumStatusData->imei = strtoull(cgsnInfo.imei, NULL, 10);
            break;
        }
        case IRIDIUM_MFG: {
            SbdCgmiInfo cgmiInfo;
            if (!SBD_cgmi(s_hSbd, &cgmiInfo)) {
                return RETURN_NOTOK;
            }
            strncpy(pIridiumStatusData->mfg, cgmiInfo.mfg,
                    sizeof(pIridiumStatusData->mfg));
            break;
        }
        case IRIDIUM_MODEL: {
            SbdCgmmInfo cgmmInfo;
            if (!SBD_cgmm(s_hSbd, &cgmmInfo)) {
                return RETURN_NOTOK;
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
            break;
        }
        case IRIDIUM_SIG_QUALITY: {
            SbdcsqInfo csqInfo;
            if (!SBD_csqf(s_hSbd, &csqInfo)) {
                return RETURN_NOTOK;
            }
            pIridiumStatusData->rssi = csqInfo.rssi;
            break;
        }
        case IRIDIUM_REGSTATUS: {
            if (!SBD_sbdregRead(s_hSbd, &pIridiumStatusData->regStat)) {
                return RETURN_NOTOK;
            }
            break;
        }
        default:
            DEBUG("OBC::ERROR: Unknown param %d\n", paramID);
            break;
    }
    return status;
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

    char ret[sizeof(msg)] = { };
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

    /* FIXME: hack to keep thread alive so we keep looping */
    Semaphore_post(ssObc.sem);
}

static ePostCode _probe(void *driver)
{
    /* TODO: this is a problem: we can't probe until we've initialized, but we
     * don't init until after probing */
    return POST_DEV_FOUND;
}

static ePostCode _init(void *driver, const void *config,
                       const void *alert_token)
{
    if (sbd_init(driver) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    return POST_DEV_CFG_DONE;
}

static bool _get_status(void *driver, unsigned int param_id,
                        void *return_buf) {
    return (iridium_get_statusParams((eOBC_StatusParam)param_id, return_buf)
            == RETURN_OK);
}

const Driver OBC_Iridium = {
    .name = "Iridium 96xx",
    .status = (Parameter[]){
        { .name = "imei", .type = TYPE_UINT64 },
        { .name = "mfg", .type = TYPE_STR, .size = 10 },
        { .name = "model", .type = TYPE_STR, .size = 5 },
        { .name = "signal.quality", .type = TYPE_UINT8 },
        { .name = "registration", .type = TYPE_ENUM,
          .values = (Enum_Map[]){
              { 0, "Detached" },
              { 1, "None" },
              { 2, "Registered" },
              { 3, "Registration Denied" },
              {}
            },
        },
        { .name = "numberofoutgoingmessage", .type = TYPE_UINT8 },
        { .name = "lasterror" }, /* TODO: this is a complex type */
        {}
    },

    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,

    .payload_fmt_union = true, /* OBC breaks serialization pattern :( */
};
