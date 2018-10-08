/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "gsm.h"

#include "at_cmd.h"
#include "helpers/memory.h"
#include "helpers/array.h"
#include "inc/common/global_header.h"

#include <string.h>

/* Timeouts for slow commands */
#define CMGS_TIMEOUT 60000
#define CMGD_TIMEOUT 10000
#define CFUN_TIMEOUT 12000

#define TESTMOD_TASK_PRIORITY 2
#define TESTMOD_TASK_STACK_SIZE 2048
static Char testmodTaskStack[TESTMOD_TASK_STACK_SIZE];

static const char CTRL_Z = 26;

static GsmCallbackList gsmCallbackList = {}; // TODO: move into handle
static AT_Response s_AtRes;

// TODO: AT_Response const?
static bool creg(AT_Response *res, void *context)
{
    /* TODO: hack to detect if this was unsolicited or request by user */
    if (res->paramCount == 2 || res->paramCount == 4) {
        return false;
    }

    if (gsmCallbackList.creg) {
        if (res->paramCount == 1 && res->param[0].type == AT_PARAM_TYPE_INT) {
            gsmCallbackList.creg((GsmCregStat)res->param[0].vInt, context);
        } else {
            LOGGER_ERROR("Error parsing creg response\n");
        }
    }
    return true;
}

static bool simReady(AT_Response *res, void *context)
{
    if (gsmCallbackList.simReady) {
        gsmCallbackList.simReady(context);
    }
    return true;
}

static bool cmti(AT_Response *res, void *context)
{
    if (gsmCallbackList.cmti) {
        if (res->paramCount == 2 && res->param[0].type == AT_PARAM_TYPE_STR &&
            res->param[1].type == AT_PARAM_TYPE_INT) {
            GsmCmtiInfo info = {
                .mem = res->param[0].pStr,
                .index = res->param[1].vInt,
            };
            gsmCallbackList.cmti(&info, context);
        }
    }
    return true;
}

static bool ring(AT_Response *res, void *context)
{
    if (gsmCallbackList.ring) {
        gsmCallbackList.ring(context);
    }
    return true;
}

static char sms[160];
static GSM_Handle s_handle; // TODO: super duper temporary

static bool cmgr(AT_Response *res, void *context)
{
    sms[0] = '\0';

    /* Read size from +CMGR response */
    if (res->paramCount < 11) {
        DEBUG("Didn't receive length\n");
        return true;
    }

    const int length = res->param[10].vInt;
    if (length > sizeof(sms)) {
        DEBUG("payload too large\n");
        return true;
    }

    /* Read payload */
    if (AT_cmd_read_data(s_handle, sms, length) < length) {
        DEBUG("failed to read payload\n");
        return true;
    }

    /* Read CRLF */
    AT_cmd_clear_buf(s_handle, 2);
    sms[length] = '\0';
    return true;
}

static bool clcc(AT_Response *res, void *context)
{
    if (gsmCallbackList.clcc) {
        GsmClccInfo info = {
            .idx = res->param[0].vInt,
            .dir = res->param[1].vInt,
            .call_state = (GsmCallState)res->param[2].vInt,
        };
        gsmCallbackList.clcc(&info, context);
    }
    return true;
}

// TODO: make callback process less complicated
static const AT_UnsolicitedRes unsolicitedList[] = {
    { // TODO: I have no idea what this is, but it's undocumented and annoying
      .fmt = "^STN:" },
    {
            .fmt = "+CREG:",
            .cb = creg,
    },
    {
            .fmt = "+SIM READY",
            .cb = simReady,
    },
    {
            .fmt = "+SIM DROP",
    },
    {
            .fmt = "+CMTI:",
            .cb = cmti,
    },
    {
            .fmt = "+CMGR:",
            .cb = cmgr,
    },
    {
            .fmt = "RING",
            .cb = ring,
    },
    {
            .fmt = "NO CARRIER",
    },
    {
            .fmt = "BUSY",
    },
    {
            .fmt = "CONNECT",
    },
    {
            .fmt = "NO ANSWER",
    },
    {
            .fmt = "+CLCC",
            .cb = clcc,
    },
    {}
};

GSM_Handle GSM_init(UART_Handle hCom, const GsmCallbackList *cbList,
                    void *cbContext)
{
    if (cbList) {
        gsmCallbackList = *cbList;
    }

    GSM_Handle handle = AT_cmd_init(hCom, unsolicitedList, cbContext);

    if (!handle) {
        return NULL;
    }

    s_handle = handle;

    // We'll enable detailed error codes so we can more easily diagnose failure
    AT_cmd(handle, NULL, "+CMEE=1");

    // Return "CONNECT" instead of "OK" when call connects
    AT_cmd(handle, NULL, "+MDC=1");

    // TODO: SBD_k(hSbd, SBD_FLOW_CONTROL_HW);
    return handle;
}

bool GSM_cgsn(GSM_Handle handle, GsmCgsnInfo *info_out)
{
    if (AT_cmd(handle, &s_AtRes, "+CGSN")) {
        if (s_AtRes.paramCount != 1) {
            LOGGER_ERROR("Param count: %d != 1\n", s_AtRes.paramCount);
            return false;
        }

        if (s_AtRes.param[0].type != AT_PARAM_TYPE_STR) {
            LOGGER_ERROR("Param type: %d != %d\n", s_AtRes.param[0].type,
                         AT_PARAM_TYPE_STR);
            return false;
        }

        if (strlen(s_AtRes.param[0].pStr) >= sizeof(info_out->imei)) {
            LOGGER_ERROR("strlen: %u >= %u\n", strlen(s_AtRes.param[0].pStr),
                         sizeof(info_out->imei));
            return false;
        }

        strcpy(info_out->imei, s_AtRes.param[0].pStr);
        return true;
    }

    LOGGER_ERROR("General failure\n");
    return false;
}

bool GSM_cimi(GSM_Handle handle, uint64_t *imsi)
{
    if (AT_cmd(handle, &s_AtRes, "+CIMI")) {
        if (s_AtRes.paramCount != 1) {
            LOGGER_ERROR("Param count: %d != 1\n", s_AtRes.paramCount);
            return false;
        }

        if (s_AtRes.param[0].type != AT_PARAM_TYPE_INT64) {
            LOGGER_ERROR("Param type: %d != %d\n", s_AtRes.param[0].type,
                         AT_PARAM_TYPE_INT64);
            return false;
        }

        *imsi = *(s_AtRes.param[0].pInt64);
        return true;
    }

    LOGGER_ERROR("General failure\n");
    return false;
}

bool GSM_cmgf(GSM_Handle handle, GsmMessageFormat fmt)
{
    return AT_cmd(handle, NULL, "+CMGF=%u", fmt);
}

// TODO: most of these params should probably be enums or structs
bool GSM_csmp(GSM_Handle handle, int fo, int vp, int pid, int dcs)
{
    return AT_cmd(handle, NULL, "+CSMP=%u,%u,%u,%u", fo, vp, pid, dcs);
}

int GSM_cmgs(GSM_Handle handle, const char *number, const char *msg)
{
    if (AtCmd_enterBinaryMode(handle, "> ", "+CMGS=\"%s\"", number)) {
        if (!AT_cmd_write_data(handle, msg, strlen(msg)) ||
            !AT_cmd_write_data(handle, &CTRL_Z, sizeof(CTRL_Z))) {
            return -1;
        }

        char res_buf[20];
        AT_cmd_set_timeout(handle, CMGS_TIMEOUT);
        bool res = AT_cmd_get_response(handle, res_buf, sizeof(res_buf));
        AT_cmd_set_timeout(handle, AT_RES_DEFAULT_TIMEOUT);

        if (!res) {
            return false;
        }

        AT_cmd_parse_response(res_buf, "+CMGS=", &s_AtRes);
        if (s_AtRes.param[0].type == AT_PARAM_TYPE_INT) {
            return s_AtRes.param[0].vInt;
        }
    }
    return -1;
}

bool GSM_creg(GSM_Handle handle, GsmCregMode n)
{
    return AT_cmd(handle, NULL, "+CREG=%u", n);
}

bool GSM_cregRead(GSM_Handle handle, GsmCregInfo *info_out)
{
    if (!AT_cmd(handle, &s_AtRes, "+CREG?")) {
        return false;
    }

    if (s_AtRes.paramCount != 2 && s_AtRes.paramCount != 4) {
        return false;
    }

    if (s_AtRes.param[0].type != AT_PARAM_TYPE_INT ||
        s_AtRes.param[1].type != AT_PARAM_TYPE_INT) {
        return false;
    }

    *info_out = (GsmCregInfo){
        .n = (GsmCregMode)s_AtRes.param[0].vInt,
        .stat = (GsmCregStat)s_AtRes.param[1].vInt,
    };

    if (s_AtRes.paramCount == 4) {
        if (s_AtRes.param[2].type != AT_PARAM_TYPE_STR ||
            s_AtRes.param[3].type != AT_PARAM_TYPE_STR) {
            return false;
        }
        info_out->lac = strtoumax(s_AtRes.param[2].pStr, NULL, 16);
        info_out->cid = strtoumax(s_AtRes.param[3].pStr, NULL, 16);
    }

    return true;
}

bool GSM_cnmi(GSM_Handle handle, int mode, int mt, int bm, int ds, int bfr)
{
    return AT_cmd(handle, NULL, "+CNMI=%u,%u,%u,%u,%u", mode, mt, bm, ds, bfr);
}

bool GSM_csdh(GSM_Handle handle, bool show)
{
    return AT_cmd(handle, NULL, "+CSDH=%u", show);
}

bool GSM_csq(GSM_Handle handle, GsmCsqInfo *info_out)
{
    if (!AT_cmd(handle, &s_AtRes, "+CSQ")) {
        return false;
    }

    if (s_AtRes.paramCount != 2) {
        return false;
    }

    if (s_AtRes.param[0].type != AT_PARAM_TYPE_INT ||
        s_AtRes.param[1].type != AT_PARAM_TYPE_INT) {
        return false;
    }

    *info_out = (GsmCsqInfo){
        .rssi = s_AtRes.param[0].vInt,
        .ber = s_AtRes.param[1].vInt,
    };

    return true;
}

bool GSM_cgmi(GSM_Handle handle, GsmCgmiInfo *info_out)
{
    if (!AT_cmd(handle, &s_AtRes, "+CGMI")) {
        return false;
    }

    if (s_AtRes.paramCount != 1) {
        return false;
    }

    if (s_AtRes.param[0].type != AT_PARAM_TYPE_STR) {
        return false;
    }

    strncpy(info_out->mfgId, s_AtRes.param[0].pStr, sizeof(info_out->mfgId));
    info_out->mfgId[sizeof(info_out->mfgId) - 1] = '\0';

    return true;
}

bool GSM_cgmm(GSM_Handle handle, GsmCgmmInfo *info_out)
{
    if (!AT_cmd(handle, &s_AtRes, "+CGMM")) {
        return false;
    }

    if (s_AtRes.paramCount != 2) {
        return false;
    }

    if (s_AtRes.param[0].type != AT_PARAM_TYPE_STR ||
        s_AtRes.param[1].type != AT_PARAM_TYPE_STR) {
        return false;
    }

    strncpy(info_out->tech, s_AtRes.param[0].pStr, sizeof(info_out->tech));
    info_out->tech[sizeof(info_out->tech) - 1] = '\0';

    strncpy(info_out->model, s_AtRes.param[1].pStr, sizeof(info_out->model));
    info_out->model[sizeof(info_out->model) - 1] = '\0';

    return true;
}

bool GSM_cops(GSM_Handle handle, GsmCopsMode mode, GsmCopsFmt format,
              const char *oper)
{
    switch (mode) {
        case GSM_COPS_MODE_AUTO:
        case GSM_COPS_MODE_DEREG:
            return AT_cmd(handle, NULL, "+COPS=%u", mode);
        case GSM_COPS_MODE_SET_FMT:
            return AT_cmd(handle, NULL, "+COPS=%u,%u", mode, format);
        default:
            return AT_cmd(handle, NULL, "+COPS=%u,%u,\"%s\"", mode, format,
                          oper);
    }
}

bool GSM_copsTest(GSM_Handle handle, GsmCopsTestInfo *info_out)
{
    return false;
}

bool GSM_a(GSM_Handle handle)
{
    // TODO: actually handle error cases
    return AT_cmd(handle, NULL, "A");
}

bool GSM_d(GSM_Handle handle, const char *number)
{
    return AT_cmd(handle, NULL, "D%s;", number);
}

bool GSM_h(GSM_Handle handle)
{
    return AT_cmd(handle, NULL, "H");
}

bool GSM_cfun(GSM_Handle handle, GsmCFun fun)
{
    AT_cmd_set_timeout(handle, CFUN_TIMEOUT);
    bool res = AT_cmd(handle, NULL, "+CFUN=%u", fun);
    AT_cmd_set_timeout(handle, AT_RES_DEFAULT_TIMEOUT);
    return res;
}

bool GSM_cnma(GSM_Handle handle)
{
    return AT_cmd(handle, NULL, "+CNMA");
}

bool GSM_cmgd(GSM_Handle handle, int index, GsmCmgdFlag flag)
{
    AT_cmd_set_timeout(handle, CMGD_TIMEOUT);
    bool res = AT_cmd(handle, NULL, "+CMGD=%u,%u", index, flag);
    AT_cmd_set_timeout(handle, AT_RES_DEFAULT_TIMEOUT);
    return res;
}

bool GSM_clccSet(GSM_Handle handle, bool state)
{
    return AT_cmd(handle, NULL, "+CLCC=%u", state);
}

// Read and write functions - more complicated than the other commands
// ============================================================================
bool GSM_cmgr(GSM_Handle handle, unsigned int index, char *sms_out,
              GsmCmgrInfo *info_out)
{
    if (!AT_cmd(handle, NULL, "+CMGR=%u", index)) {
        return false;
    }

    strcpy(sms_out, sms);

    return true;
}
