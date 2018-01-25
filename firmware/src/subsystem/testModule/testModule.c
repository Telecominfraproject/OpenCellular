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
#include "inc/subsystem/testModule/testModule.h"

#include "devices/uart/gsm.h"
#include "inc/common/global_header.h"
#include "helpers/array.h"
#include "helpers/math.h"
#include "helpers/uart.h"
#include "registry/SSRegistry.h"

#include <ti/drivers/UART.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <stdio.h>
#include <string.h>

/* TODO: move to helper? */
#define STATIC_STRLEN(s) (ARRAY_SIZE(s) - 1)

//*****************************************************************************
//                             MACROS DEFINITION
//*****************************************************************************
#define TESTMOD_TASK_PRIORITY     2
#define TESTMOD_TASK_STACK_SIZE   2048

#define G510_WRITE_TIMEOUT 500
#define G510_READ_TIMEOUT 5000

/* G510 enable line is active-low */
#define GSM_EN_ASSERT   (0)
#define GSM_EN_DEASSERT (1)

#define GSM_PWR_EN_ASSERT   (1)
#define GSM_PWR_EN_DEASSERT (0)

#define GSM_SHUTDOWN_TIME 200
#define GSM_COOLDOWN_TIME 50

typedef enum {
    TWOG_SIM_CALLSTATE_CHANGE   = 0,
    TWOG_SIM_INCOMING_MSG       = 1,
    TWOG_SIM_ALERT_PARAMS_MAX   /* Limiter */
} eTEST_MOD_ALERTParam;

typedef enum  {
    TWOG_CALL_EVT_RING     = 0,
    TWOG_CALL_EVT_CALL_END = 1,
} eTEST_MODE_CallEvent;

typedef enum {
    TWOG_IMEI               = 0,
    TWOG_IMSI               = 1,
    TWOG_GETMFG             = 2,
    TWOG_GETMODEL           = 3,
    TWOG_RSSI               = 4,
    TWOG_BER                = 5,
    TWOG_REGSTATUS          = 6,
    TWOG_NETWORK_OP_INFO    = 7,
    TWOG_CELLID             = 8,
    TWOG_BSIC               = 9,
    TWOG_LASTERR            = 10,
    TWOG_PARAM_MAX         /* Limiter */
} eTestModule_StatusParam;

//*****************************************************************************
//                             HANDLES DEFINITION
//*****************************************************************************
OCSubsystem ssTestmod = {
    .taskStackSize = TESTMOD_TASK_STACK_SIZE,
    .taskPriority = TESTMOD_TASK_PRIORITY,
};

static Char testmodTaskStack[TESTMOD_TASK_STACK_SIZE];

static UART_Handle uartGsm;
static GSM_Handle s_hGsm = NULL;

static Semaphore_Handle sem_simReady;
static Semaphore_Handle sem_sms;

static volatile int sms_idx = -1;

static void simReady_cb(void *context)
{
    Semaphore_post(sem_simReady);
}

static void cmti_cb(const GsmCmtiInfo *info, void *context)
{
    sms_idx = info->index;
    Semaphore_post(sem_sms);
}

static void call_state_cb(const GsmClccInfo *info, void *context)
{
    DEBUG("CLCC %u\n", info->call_state);
    switch (info->call_state) {
        case GSM_CALL_STATE_INCOMING: {
            eTEST_MODE_CallEvent callState = TWOG_CALL_EVT_RING;
            OCMP_GenerateAlert(context, TWOG_SIM_CALLSTATE_CHANGE, &callState);
            break;
        }
        case GSM_CALL_STATE_RELEASED: {
            eTEST_MODE_CallEvent callState = TWOG_CALL_EVT_CALL_END;
            OCMP_GenerateAlert(context, TWOG_SIM_CALLSTATE_CHANGE, &callState);
            break;
        }
    }
}

/* TODO: temporary */
extern const void *sys_config[];

/* Configures the various IO pins associated with this subsystem */
static bool configure_io(void) {
    const TestMod_Cfg *testmod_cfg = sys_config[OC_SS_TEST_MODULE];
    const G510_Cfg *cfg = &testmod_cfg->g510_cfg;

    OcGpio_configure(&cfg->pin_sim_present, OCGPIO_CFG_INPUT);
    OcGpio_configure(&cfg->pin_enable, OCGPIO_CFG_OUTPUT |
                                       OCGPIO_CFG_OUT_LOW);
    OcGpio_configure(&cfg->pin_pwr_en, OCGPIO_CFG_OUTPUT |
                                       OCGPIO_CFG_OUT_LOW);

    return true;
}

static UART_Handle open_comm(void)
{
    const TestMod_Cfg *testmod_cfg = sys_config[OC_SS_TEST_MODULE];

    // Open GSM UART
    UART_Params uartParams;
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 115200;
    uartParams.dataLength = UART_LEN_8;
    uartParams.parityType = UART_PAR_NONE;
    uartParams.stopBits = UART_STOP_ONE;
    uartParams.writeTimeout = G510_WRITE_TIMEOUT;
    uartParams.readTimeout = G510_READ_TIMEOUT;
    return UART_open(testmod_cfg->g510_cfg.uart, &uartParams);
}

/* General reset procedure:
 * - send soft shutdown reset to try "nice" shutdown
 * - kill power to unit in case soft shutdown didn't take
 * - reapply power and assert enable line to reboot module
 */
static bool g510_reset(void)
{
    const TestMod_Cfg *testmod_cfg = sys_config[OC_SS_TEST_MODULE];
    const G510_Cfg *cfg = &testmod_cfg->g510_cfg;

    /* Ensure the enable line is high (deasserted) so the module doesn't
     * restart itself */
    OcGpio_write(&cfg->pin_enable, GSM_EN_DEASSERT);

    /* Void any data already in modem, then send reset command */
    static const char CTRL_X = 0x18;
    static const char resetCmd[] = "AT+CFUN=0\r\n";
    UART_write(uartGsm, &CTRL_X, sizeof(CTRL_X));
    UART_write(uartGsm, resetCmd, STATIC_STRLEN(resetCmd));

    /* Wait for module to catch up */
    Task_sleep(GSM_SHUTDOWN_TIME);

    /* Kill power to be sure */
    OcGpio_write(&cfg->pin_pwr_en, GSM_PWR_EN_DEASSERT);
    UART_flush(uartGsm);
    Task_sleep(GSM_COOLDOWN_TIME);

    /* Enable the module. We can keep the line asserted - the module requires an
     * edge before it will look at this line again */
    OcGpio_write(&cfg->pin_enable, GSM_EN_ASSERT);
    OcGpio_write(&cfg->pin_pwr_en, GSM_PWR_EN_ASSERT);

    /* Wait for the module to be ready */
    const char bootStr[] = "AT command ready\r\n";
    char buf[STATIC_STRLEN(bootStr)];

    if (UART_read(uartGsm, buf, sizeof(buf)) < sizeof(buf)) {
        LOGGER_ERROR("Timeout waiting for G510\n");
        return false;
    }

    if (memcmp(buf, bootStr, sizeof(buf)) != 0) {
        LOGGER_ERROR("Unrecognized G510 boot str: %.*s\n", (int)sizeof(buf),
                     buf);
        return false;
    }

    return true;
}

static ReturnStatus g510_init(const void *alert_token)
{
    if (!configure_io()) {
        return RETURN_NOTOK;
    }

    uartGsm = open_comm();
    if (!uartGsm) {
        return RETURN_NOTOK;
    }

    DEBUG("Resetting GSM module\n");
    if (!g510_reset()) {
        return RETURN_NOTOK;
    }
    Task_sleep(100); // FIXME: this lets things catch up when using the emulator
    DEBUG("GSM module ready\n");

    // Configure SBD module
    const GsmCallbackList cbList = {
        .creg = NULL,
        .simReady = simReady_cb,
        .cmti = cmti_cb,
        .ring = NULL, /* We're using clcc, so ignore RING alerts */
        .clcc = call_state_cb,
    };

    GsmCgsnInfo cgsnInfo;
    s_hGsm = GSM_init(uartGsm, &cbList, (void *)alert_token);

    if (!s_hGsm) {
        return RETURN_NOTOK;
    }

    GSM_cgsn(s_hGsm, &cgsnInfo);
    DEBUG("IMEI: %s\n", cgsnInfo.imei);

    // TODO: call cfun to ensure radio is active (this setting is sticky)

    GSM_creg(s_hGsm, GSM_CREG_STATUS_ENABLE_LOC);

    DEBUG("Test module ready\n");

    return RETURN_OK;
}

static bool testModule_get_statusParams(eTestModule_StatusParam paramID,
                                        TestMod_2G_Status_Data *p2gStatusData)
{
    switch (paramID) {
        case TWOG_IMEI:
        {
            GsmCgsnInfo cgsnInfo;
            if (!GSM_cgsn(s_hGsm, &cgsnInfo)) {
                return false;
            }
            p2gStatusData->imei = strtoull(cgsnInfo.imei, NULL, 10);
            break;
        }
        case TWOG_IMSI:
        {
            uint64_t imsi;
            if (!GSM_cimi(s_hGsm, &imsi)) {
                return false;
            }
            p2gStatusData->imsi = imsi;
            break;
        }
        case TWOG_GETMFG:
        {
            GsmCgmiInfo cgmiInfo;
            if (!GSM_cgmi(s_hGsm, &cgmiInfo)) {
                return false;
            }
            /* TODO: idea - make safe strncpy that always terminates str */
            strncpy(p2gStatusData->mfg, cgmiInfo.mfgId,
                    sizeof(p2gStatusData->mfg));
            break;
        }
        case TWOG_GETMODEL:
        {
            GsmCgmmInfo cgmmInfo;
            if (!GSM_cgmm(s_hGsm, &cgmmInfo)) {
                return false;
            }
            strncpy(p2gStatusData->model, cgmmInfo.model,
                    sizeof(p2gStatusData->model));
            break;
        }
        /* TODO: optimize this - no reason to call CSQ twice */
        case TWOG_RSSI:
        {
            GsmCsqInfo csqInfo;
            if (!GSM_csq(s_hGsm, &csqInfo)) {
                return false;
            }
            p2gStatusData->rssi = csqInfo.rssi;
            break;
        }
        case TWOG_BER:
        {
            GsmCsqInfo csqInfo;
            if (!GSM_csq(s_hGsm, &csqInfo)) {
                return false;
            }
            p2gStatusData->ber = csqInfo.ber;
            break;
        }
        case TWOG_REGSTATUS:
        {
            GsmCregInfo cregInfo;
            if (!GSM_cregRead(s_hGsm, &cregInfo)) {
                return false;
            }
            p2gStatusData->regStat = cregInfo.stat;
            break;
        }
        case TWOG_NETWORK_OP_INFO:
            /* TODO: from +COPS=? */
            return false;
        case TWOG_CELLID:
        {
            /* NOTE: requires CREG mode 2 (unsolicited + location info) */
            GsmCregInfo cregInfo;
            if (!GSM_cregRead(s_hGsm, &cregInfo)) {
                return false;
            }
            p2gStatusData->cid = cregInfo.cid;
            break;
        }
        case TWOG_BSIC:
            /* TODO: from +MCELL? */
            return false;
        case TWOG_LASTERR:
            /* TODO: implement last error */
            return false;
        default:
            DEBUG("TESTMOD::ERROR: Unknown param %d\n", paramID);
            return false;
    }
    return true;
}

/* Command handling */
bool TestMod_cmdEnable(void *driver, void *params)
{
    DEBUG("TESTMOD 2G Enable\n");
    return GSM_cfun(s_hGsm, GSM_CFUN_FULL);
}

bool TestMod_cmdDisable(void *driver, void *params)
{
    DEBUG("TESTMOD 2G Disable\n");
    return GSM_cfun(s_hGsm, GSM_CFUN_AIRPLANE);
}

bool TestMod_cmdDisconnect(void *driver, void *params)
{
    DEBUG("TESTMOD 2G Disconnect\n");
    return GSM_cops(s_hGsm, GSM_COPS_MODE_DEREG, GSM_COPS_FMT_NUMERIC, "");
}

bool TestMod_cmdConnect(void *driver, void *params)
{
    DEBUG("TESTMOD 2G Connect\n");
    return GSM_cops(s_hGsm, GSM_COPS_MODE_AUTO, GSM_COPS_FMT_NUMERIC, "");
}

bool TestMod_cmdSendSms(void *driver, void *params)
{
    DEBUG("TESTMOD 2G SMS\n");
    /* TODO: we assume number is null terminated, should have check */
    TestModule_sms *sms = params;
    return GSM_cmgs(s_hGsm, sms->number, sms->msg);
}

bool TestMod_cmdDial(void *driver, void *params)
{
    /* TODO: we assume number is null terminated, should have check */
    char *number = params;
    /* TODO: watch for call state change to determine if successful */
    DEBUG("TESTMOD 2G Dial\n");
    return GSM_d(s_hGsm, number);
}

bool TestMod_cmdAnswer(void *driver, void *params)
{
    DEBUG("TESTMOD 2G answer\n");
    return GSM_a(s_hGsm);
}

bool TestMod_cmdHangup(void *driver, void *params)
{
    DEBUG("TESTMOD 2G hangup\n");
    return GSM_h(s_hGsm);
}

bool TestMod_cmdReset(void *driver, void *params)
{
    DEBUG("TESTMOD Reset\n");
    return false; /* Not yet implemented */
}

static void testModule_task(UArg a0, UArg a1)
{
    const void *alert_token = (const void *)a0;

    /* Wait for our SIM card to be ready and then finish init */
    Semaphore_pend(sem_simReady, BIOS_WAIT_FOREVER);
    DEBUG("TESTMOD::SIM Ready\n");

    uint64_t imsi;
    GSM_cimi(s_hGsm, &imsi);
    /* TODO: hack because System_printf is crappy and doesn't support %llu */
    char imsiStr[16];
    snprintf(imsiStr, sizeof(imsiStr), "%"PRIu64, imsi);
    DEBUG("IMSI: %s\n", imsiStr);

    /* NOTE: if the message storage fills up, the G510 will just
     * reject any SMS from the network, so we need to make sure
     * it's clear
     */
    GSM_cmgd(s_hGsm, 1, GSM_CMGD_DELETE_ALL);

    GSM_clccSet(s_hGsm, true); /* Enable clcc (call state) msg */

    /* Finish device configuration */
    if (!GSM_cnmi(s_hGsm, 2, 1, 0, 0 , 0) ||   /* enable sms arrival notif */
        !GSM_cmgf(s_hGsm, GSM_MSG_FMT_TEXT) || /* set to text mode */
        !GSM_csmp(s_hGsm, 17, 167, 0, 0) ||    /* text mode parameters */
        !GSM_csdh(s_hGsm, true)) {             /* display extra info for cgmr */
        s_hGsm = NULL; /* TODO: proper teardown of handle */
    }

    while (Semaphore_pend(sem_sms, BIOS_WAIT_FOREVER)) {
        if (sms_idx >= 0) {
            GSM_cnma(s_hGsm);
            static char sms[160];
            if (GSM_cmgr(s_hGsm, sms_idx, sms, NULL)) {
                DEBUG("SMS: %.*s\n", 50, sms); // System_printf has a limited buffer
                OCMP_GenerateAlert(alert_token, TWOG_SIM_INCOMING_MSG, sms);
            } else {
                LOGGER_ERROR("TESTMOD:Failed to read SMS\n");
            }

            GSM_cmgd(s_hGsm, sms_idx, GSM_CMGD_DELETE_AT_INDEX);
            sms_idx = -1;

            //if (GSM_cmgs(s_hGsm, "29913", "Hello from GSM :)") < 0) {
            //  LOGGER_ERROR("TESTMOD:Error sending SMS\n");
            //}
        }
    }
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
    /* TODO: there's probably a better way to wait on the sim card */
    sem_simReady = Semaphore_create(0, NULL, NULL);
    sem_sms = Semaphore_create(0, NULL, NULL);
    if (!sem_simReady || !sem_sms) {
        LOGGER_ERROR("TESTMOD:ERROR:: Failed creating semaphores\n");
        Semaphore_delete(&sem_simReady);
        Semaphore_delete(&sem_sms);
        return POST_DEV_CFG_FAIL;
    }

    /* TODO: it's less than ideal to have a dedicated task for this */
    /* Create a task */
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stack = testmodTaskStack;
    taskParams.stackSize = TESTMOD_TASK_STACK_SIZE;
    taskParams.priority = TESTMOD_TASK_PRIORITY;
    taskParams.arg0 = (intptr_t)alert_token;
    Task_Handle task = Task_create(testModule_task, &taskParams, NULL);
    if (!task) {
        DEBUG("TESTMOD::FATAL: Unable to start G510 task\n");
        Semaphore_delete(&sem_simReady);
        Semaphore_delete(&sem_sms);
        return POST_DEV_CFG_FAIL;
    }

    if (g510_init(alert_token) != RETURN_OK) {
        return POST_DEV_CFG_FAIL;
    }
    return POST_DEV_CFG_DONE;
}

static bool _get_status(void *driver, unsigned int param_id, void *return_buf)
{
    return testModule_get_statusParams((eTestModule_StatusParam)param_id,
                                       return_buf);
}

const Driver Testmod_G510 = {
    .name = "Fibocom G510",
    .status = (Parameter[]){
        { .name = "imei", .type = TYPE_UINT64 },
        { .name = "imei", .type = TYPE_UINT64 },
        { .name = "mfg", .type = TYPE_STR, .size = 10 },
        { .name = "model", .type = TYPE_STR, .size = 5 },
        { .name = "rssi", .type = TYPE_UINT8 },
        { .name = "ber", .type = TYPE_UINT8 },
        { .name = "registration", .type = TYPE_ENUM,
          .values = (Enum_Map[]){
              { 0, "Not Registered, Not Searching" },
              { 1, "Registered, Home Network" },
              { 2, "Not Registered, Searching" },
              { 3, "Registration Denied" },
              { 4, "Status Unknown" },
              { 5, "Registered, Roaming" },
              {}
            },
        },
        { .name = "network_operatorinfo" }, /* TODO: this is a complex type */
        { .name = "cellid", TYPE_UINT32 },
        { .name = "bsic", TYPE_UINT8 },
        { .name = "lasterror" }, /* TODO: this is a complex type */
        {}
    },
    .alerts = (Parameter[]){
        { .name = "Call State Changed", .type = TYPE_ENUM,
            .values = (Enum_Map[]){
                { 0, "Ringing" },
                { 1, "Call End" },
                {}
            },
        },
        /* TODO: var len str */
        { .name = "Incoming SMS", .type = TYPE_STR, .size = 20 },
        {}
    },

    /* Message handlers */
    .cb_probe = _probe,
    .cb_init = _init,
    .cb_get_status = _get_status,

    .payload_fmt_union = true, /* Testmodule breaks serialization pattern :( */
};
