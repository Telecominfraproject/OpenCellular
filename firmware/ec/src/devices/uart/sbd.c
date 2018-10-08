/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "sbd.h"

#include "at_cmd.h"
#include "helpers/array.h"
#include "helpers/memory.h"
#include "inc/common/global_header.h"

#include <xdc/runtime/System.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Disable debugging for this module
//#define DEBUG(...)

/* Timeouts for slow commands */
#define SBDIX_TIMEOUT 60000

// Tells us the number of members in a response struct
#define NUM_RESPONSES(x) (sizeof(*x) / sizeof(int))

static SbdCallbackList sbdCallbackList = {}; // TODO: move into handle
static AT_Response s_AtRes; // No point having this large struct on the stack

// Helper function to take care of all-integer responses
static bool copy_int_responses(AT_Response *atRes, int *info_out, int infoSize)
{
    if (atRes->paramCount != infoSize) {
        return false;
    }
    // TODO: Should I be so strict about number of params?
    for (int i = 0; i < infoSize; ++i) {
        if (atRes->param[i].type != AT_PARAM_TYPE_INT) {
            return false;
        }
        info_out[i] = atRes->param[i].vInt;
    }
    return true;
}

static bool sbdring(AT_Response *res, void *context)
{
    if (sbdCallbackList.sbdring) {
        sbdCallbackList.sbdring(context);
    }
    return true;
}

static bool ciev(AT_Response *res, void *context)
{
    if (sbdCallbackList.ciev) {
        SbdCievInfo info_out;
        copy_int_responses(res, (int *)&info_out, NUM_RESPONSES(&info_out));
        sbdCallbackList.ciev(&info_out, context);
    }
    return true;
}

static const AT_UnsolicitedRes unsolicitedList[] = { {
                                                             .fmt = "SBDRING",
                                                             .cb = sbdring,
                                                     },
                                                     {
                                                             .fmt = "+CIEV:",
                                                             .cb = ciev,
                                                     },
                                                     {} };

SBD_Handle SBD_init(UART_Handle hCom, const SbdCallbackList *cbList,
                    void *cbContext)
{
    if (cbList) {
        sbdCallbackList = *cbList;
    }

    SBD_Handle hSbd = AT_cmd_init(hCom, NULL, NULL);

    if (!hSbd) {
        return NULL;
    }

    AT_cmd_register_unsolicited(hSbd, unsolicitedList, cbContext);
    return hSbd;
}

bool SBD_sbdix(SBD_Handle handle, SbdixInfo *info_out, bool alert_response)
{
    const char *cmd_fmt = (alert_response) ? "+SBDIXA" : "+SBDIX";
    AT_cmd_set_timeout(handle, SBDIX_TIMEOUT);
    bool res = AT_cmd(handle, &s_AtRes, cmd_fmt) &&
               copy_int_responses(&s_AtRes, (int *)info_out,
                                  NUM_RESPONSES(info_out));
    AT_cmd_set_timeout(handle, AT_RES_DEFAULT_TIMEOUT);
    return res;
}

bool SBD_sbds(SBD_Handle handle, SbdsInfo *info_out)
{
    const char *cmd_fmt = "+SBDS";
    return AT_cmd(handle, &s_AtRes, cmd_fmt) &&
           copy_int_responses(&s_AtRes, (int *)info_out,
                              NUM_RESPONSES(info_out));
}

bool SBD_sbdsx(SBD_Handle handle, SbdsxInfo *info_out)
{
    const char *cmd_fmt = "+SBDSX";
    return AT_cmd(handle, &s_AtRes, cmd_fmt) &&
           copy_int_responses(&s_AtRes, (int *)info_out,
                              NUM_RESPONSES(info_out));
}

bool SBD_cgsn(SBD_Handle handle, SbdcgsnInfo *info_out)
{
    return AT_cmd_raw(handle, info_out->imei, sizeof(info_out->imei), "+CGSN");
}

bool SBD_cgmi(SBD_Handle handle, SbdCgmiInfo *info_out)
{
    return AT_cmd_raw(handle, info_out->mfg, sizeof(info_out->mfg), "+CGMI");
}

bool SBD_cgmm(SBD_Handle handle, SbdCgmmInfo *info_out)
{
    return AT_cmd_raw(handle, info_out->model, sizeof(info_out->model),
                      "+CGMM");
}

bool SBD_k(SBD_Handle handle, SbdFlowControl flowControl)
{
    return AT_cmd(handle, NULL, "&K%u", flowControl);
}

bool SBD_sbdd(SBD_Handle handle, SbdDeleteType deleteType)
{
    char resCode;
    bool res = AT_cmd_raw(handle, &resCode, sizeof(resCode), "+SBDD%u",
                          deleteType);
    return (res && resCode == '0');
}

bool SBD_csq(SBD_Handle handle, SbdcsqInfo *info_out)
{
    const char *cmd_fmt = "+CSQ";
    return AT_cmd(handle, &s_AtRes, cmd_fmt) &&
           copy_int_responses(&s_AtRes, (int *)info_out,
                              NUM_RESPONSES(info_out));
}

bool SBD_csqf(SBD_Handle handle, SbdcsqInfo *info_out)
{
    const char *cmd_fmt = "+CSQF";
    return AT_cmd(handle, &s_AtRes, cmd_fmt) &&
           copy_int_responses(&s_AtRes, (int *)info_out,
                              NUM_RESPONSES(info_out));
}

bool SBD_sbdareg(SBD_Handle handle, SbdAregMode mode)
{
    return AT_cmd(handle, NULL, "+SBDAREG=%u", mode);
}

bool SBD_sbdregRead(SBD_Handle handle, SbdRegStat *status_out)
{
    if (!AT_cmd(handle, &s_AtRes, "+SBDREG?")) {
        return false;
    }
    if (s_AtRes.paramCount != 1 || s_AtRes.param[0].type != AT_PARAM_TYPE_INT) {
        return false;
    }
    *status_out = (SbdRegStat)s_AtRes.param[0].vInt;
    return true;
}

bool SBD_sbdtc(SBD_Handle handle)
{
    // We will get an info response, but we don't really care about it
    char res[100];
    return AT_cmd_raw(handle, res, sizeof(res), "+SBDTC");
}

bool SBD_cier(SBD_Handle handle, bool mode, bool sigind, bool svcind,
              bool antind, bool sv_beam_coords_ind)
{
    return AT_cmd(handle, NULL, "+CIER=%d,%d,%d,%d,%d", mode, sigind, svcind,
                  antind, sv_beam_coords_ind);
}

bool SBD_sbdmta(SBD_Handle handle, bool mode)
{
    return AT_cmd(handle, NULL, "+SBDMTA=%d", mode);
}

// Read and write functions - more complicated than the other commands
// ============================================================================
static uint16_t checksum(const void *data, size_t len)
{
    uint16_t sum = 0; // uints wrap around on overflow, so no need to worry
    for (int i = 0; i < len; ++i) {
        sum += ((uint8_t *)data)[i];
    }
    return sum;
}

bool SBD_sbdwb(SBD_Handle handle, const void *data, int data_len)
{
    // Enter data mode
    // (parse the response separately, since it's very nonstandard)
    if (!AtCmd_enterBinaryMode(handle, "READY\r\n", "+SBDWB=%d", data_len)) {
        DEBUG("Error entering data mode: no prompt\n");
        return false;
    }

    // Successfully in data mode, write data
    if (!AT_cmd_write_data(handle, data, data_len)) {
        return false;
    }

    // Write 2-byte checksum
    uint16_t cksm = checksum(data, data_len);
    if (!AT_cmd_write16(handle, cksm)) {
        return false;
    }

    // Check response code
    char res_buf[2];
    if (AT_cmd_get_response(handle, &res_buf, sizeof(res_buf))) {
        if (strcmp(res_buf, "0") == 0) {
            return true;
        } else {
            DEBUG("Error sending data: %s\n", res_buf);
            return false;
        }
    }

    return false;
}

typedef struct SbdBinaryRes {
    uint16_t size;
    uint16_t checksum;
    uint8_t *data;
} SbdBinaryRes;

static int read_binary_data(AT_Handle handle, void *buf)
{
    SbdBinaryRes *res = buf;

    // Read length (2B)
    AT_cmd_read16(handle, &res->size);
    //DEBUG("Payload len: %d\n", res->size);

    // TODO: sanity check the payload length
    res->data = malloc(res->size);
    if (!res->data) {
        System_abort("SBD: System OOM");
    }

    // Read payload
    if (AT_cmd_read_data(handle, res->data, res->size) < res->size) {
        DEBUG("failed to read payload\n");
        return -1;
    }

    // Read checksum (2B)
    if (!AT_cmd_read16(handle, &res->checksum)) {
        DEBUG("failed to read checksum\n");
        return -1;
    }

    return sizeof(SbdBinaryRes);
}

// We have to make the assumption that the start of this (the length) won't
// overlap with <cr><lf> - luckily, it can't
int SBD_sbdrb(SBD_Handle handle, void *buffer, int buffer_len)
{
    SbdBinaryRes binRes = {};
    int res = -1;
    AT_cmd_register_binary_handler(handle, read_binary_data);
    if (!AT_cmd_raw(handle, &binRes, sizeof(binRes), "+SBDRB")) {
        DEBUG("Error writing command :O\n");
        goto cleanup;
    }

    // Compare checksums
    uint16_t cksm = checksum(binRes.data, binRes.size);
    if (binRes.checksum != cksm) {
        DEBUG("Checksum mismatch :(\n");
        printMemory(&cksm, sizeof(cksm));
        printMemory(&binRes.checksum, sizeof(binRes.checksum));
        goto cleanup;
    }

    // TODO: what do we do if the buffer is too small?
    if (buffer_len < binRes.size) {
        goto cleanup;
    }
    memcpy(buffer, binRes.data, binRes.size);
    res = binRes.size;

cleanup:
    free(binRes.data); // Malloc'd by read_binary_data
    return res;
}

void SBD_test_invalid(SBD_Handle handle)
{
    char response[100];
    AT_cmd_raw(handle, response, sizeof(response), "+CGsM");
}
