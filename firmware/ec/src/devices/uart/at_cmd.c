/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "at_cmd.h"

#include "helpers/array.h"
#include "helpers/math.h"
#include "inc/common/global_header.h"

#include <driverlib/emac.h> /* TODO: for htons - clean up this random include */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Task.h>
#include <xdc/runtime/System.h>
#include <inc/common/byteorder.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// Disable debugging for this module
#undef DEBUG
#define DEBUG(...)
#define FLUSH(...)
// Enable faster debugging (only flush once every line read)
//#define DEBUG System_printf
//#define FLUSH System_flush

#define CMD_MAX_LEN 127
#define CMD_PREFIX "AT"
#define CMD_SUFFIX "\r"

// NOTE: these only apply when verbose mode is enabled (V1)
#define RES_PREFIX "\r\n"
#define RES_SUFFIX "\r\n"

typedef struct AT_Info {
    UART_Handle uartHandle;

    // Method to do a multi-peek at the data
    char s_buf[10]; // TODO: idk what this should be
    int s_bufLen;

    const char *AtPrompt; // Write mode prompt
    AtBinaryReadHandler binaryReadHandler;

    Mailbox_Handle inbox;

    const AT_UnsolicitedRes *unsolicitedResponses;
    void *cbContext;

    uint32_t responseTimeout;
} AT_Info;

typedef enum AtResultCode {
    AT_RESULT_CODE_INVALID = -1,
    AT_RESULT_CODE_OK = '0',
    AT_RESULT_CODE_ERROR = '4',
    AT_RESULT_CODE_ERROR_CUSTOM,
} AtResultCode;

// Mapping result strings to result code
typedef struct AtResultString {
    const char *str;
    AtResultCode code;
} AtResultString;

// These are AT standard - we want to do a full string match
static const AtResultString AtResultStringMap[] = {
    {
            .str = "OK",
            .code = AT_RESULT_CODE_OK,
    },
    {
            .str = "ERROR",
            .code = AT_RESULT_CODE_ERROR,
    },
};

// These are special result codes that can be returned, partially match
// TODO: these probably shouldn't be hardcoded in this module
static const AtResultString AtCustomResultStrings[] = {
    {
            .str = "+CMS ERROR:",
            .code = AT_RESULT_CODE_ERROR_CUSTOM,
    },
    {
            .str = "+CME ERROR:",
            .code = AT_RESULT_CODE_ERROR_CUSTOM,
    },
};

#define AT_READ_TASK_PRIORITY 6
#define AT_READ_TASK_STACK_SIZE 1024

bool AT_cmd_write_data(AT_Handle handle, const void *data, size_t data_len)
{
    if (!handle) {
        return false;
    }

    //DEBUG("Write: ");
    for (int i = 0; i < data_len; ++i) {
        //    DEBUG("%x ", ((uint8_t *)data)[i]);
    }
    //DEBUG("\n");
    return (UART_write(handle->uartHandle, data, data_len) == data_len);
}

bool AT_cmd_write16(AT_Handle handle, uint16_t data)
{
    size_t size = sizeof(data);
    data = htobe16(data);
    return AT_cmd_write_data(handle, &data, size);
}

int AT_cmd_read_data(AT_Handle handle, void *data, size_t data_len)
{
    if (!handle) {
        return -1;
    }

    int readNum = 0;
    // TODO: this logic is so wrong - what if data_len < s_bufLen?
    if (handle->s_bufLen > 0) {
        readNum = MIN(handle->s_bufLen, data_len);
        memcpy(data, handle->s_buf, readNum);
        data = (uint8_t *)data + readNum;
        data_len -= readNum;
        handle->s_bufLen -= readNum;
    }
    int res = UART_read(handle->uartHandle, data, data_len) + readNum;
    //    if (res > 0) {
    //        if (*(char *)data == '\r') {
    //            System_printf("\\r\n");
    //        } else if (*(char *)data == '\n') {
    //            System_printf("\\n\n");
    //        } else {
    //            System_printf("%.*s\n", res, data);
    //        }
    //    }
    if (res < 0) {
        LOGGER_ERROR("Fatal - unable to read from UART\n");
    }
    return res;
}

// TODO: maybe just make this a UART helper function
void AT_cmd_clear_buf(AT_Handle handle, int bytes)
{
    if (!handle) {
        return;
    }

    uint8_t tmp;
    if (bytes) {
        while (bytes--) {
            AT_cmd_read_data(handle, &tmp, sizeof(tmp));
        }
    } else {
        // We can't do this with a file
#if !FILE_DEBUG
        bool avail;
        while ((UART_control(handle->uartHandle, UART_CMD_ISAVAILABLE,
                             &avail) == UART_STATUS_SUCCESS) &&
               avail) {
            AT_cmd_read_data(handle, &tmp, sizeof(tmp));
        }
#endif
    }
}

bool AT_cmd_read16(AT_Handle handle, uint16_t *data)
{
    size_t size = sizeof(*data);
    bool res = (AT_cmd_read_data(handle, data, size) == size);
    *data = ntohs(*data);
    return res;
}

typedef struct DefLineType {
    const char *pfx;
    const char *sfx;
} DefLineType;

typedef enum AtLineType {
    AT_LINE_TYPE_INVALID = -1,

    AT_LINE_TYPE_RESPONSE = 0,
    AT_LINE_TYPE_CMD_ECHO,

    AT_LINE_TYPE_BINARY,
    COUNT_AT_LINE_TYPE = AT_LINE_TYPE_BINARY,

    AT_LINE_TYPE_PROMPT, // TODO: this enum is gross
} AtLineType;

// TODO: what should this be?
#define AT_MAX_LINE_LEN 100

typedef struct At_RawResponse {
    AtLineType type;
    size_t size;
    char data[AT_MAX_LINE_LEN];
} At_RawResponse;

static const DefLineType LINE_TYPES[COUNT_AT_LINE_TYPE] = {
    [AT_LINE_TYPE_RESPONSE] = { .pfx = RES_PREFIX, .sfx = RES_SUFFIX },
    [AT_LINE_TYPE_CMD_ECHO] =
            {
                    .pfx = CMD_PREFIX,
                    .sfx = CMD_SUFFIX,
            },
};

static AtLineType get_line_type(AT_Handle handle, char *buf, size_t buf_size,
                                int *idx_out)
{
    int idx = -1; // Index of the character we're checking
    bool ignore[COUNT_AT_LINE_TYPE] = {};
    int remaining; // Number of types remaining to check
    do {
        if (++idx > buf_size) {
            LOGGER_ERROR("Fatal: read line buffer overflow\n");
            return AT_LINE_TYPE_INVALID;
        }

        if (AT_cmd_read_data(handle, &buf[idx], sizeof(*buf)) <= 0) {
            return AT_LINE_TYPE_INVALID;
        }

        remaining = 0;
        for (int type = 0; type < COUNT_AT_LINE_TYPE; ++type) {
            if (ignore[type]) {
                continue;
            }

            const char *pfx = LINE_TYPES[type].pfx;
            if (buf[idx] != pfx[idx]) {
                ignore[type] = true;
            } else {
                if (idx >= (strlen(pfx) - 1)) {
                    DEBUG("Found prefix: %d\n", type);
                    *idx_out = idx + 1;
                    return (AtLineType)type;
                }
                ++remaining;
            }
        }
    } while (remaining);

    // We didn't recognize the prefix, we'll assume it's a binary response
    return AT_LINE_TYPE_BINARY;
}

void AT_cmd_register_binary_handler(AT_Handle handle,
                                    AtBinaryReadHandler handler)
{
    if (!handle) {
        return;
    }

    // TODO: this can be accessed atomically, but is it proper to?
    handle->binaryReadHandler = handler;
}

static bool AT_cmd_read_line(AT_Handle handle, At_RawResponse *res)
{
    // Figure out what type of response this is
    int lineLen = 0;
    res->type = get_line_type(handle, handle->s_buf, sizeof(handle->s_buf),
                              &lineLen);

    if (res->type == AT_LINE_TYPE_INVALID) {
        return false;
    }

    int promptLen = 0;

    switch (res->type) {
        case AT_LINE_TYPE_BINARY:
            if (handle->binaryReadHandler) {
                handle->s_bufLen =
                        lineLen +
                        1; // TODO: this is dumb, get_line_type should just know about the temp buf
                res->size = handle->binaryReadHandler(handle, res->data);
                handle->binaryReadHandler = NULL;
                return (res->size >= 0);
            }
            LOGGER_ERROR("FATAL: Unhandled binary data: %.*s\n", lineLen + 1,
                         handle->s_buf);
            return false;
        case AT_LINE_TYPE_RESPONSE:
            if (handle->AtPrompt) {
                promptLen = strlen(handle->AtPrompt);
            }
            // Fallthrough
        case AT_LINE_TYPE_CMD_ECHO:
            // Omit the response suffix
            lineLen = 0;
            break;
    }

    // Read the rest of the line until we hit the suffix
    const char *sfx = LINE_TYPES[res->type].sfx;
    int idx = 0;
    while (idx < strlen(sfx)) {
        if (lineLen > AT_MAX_LINE_LEN) {
            LOGGER_ERROR("Fatal: read line buffer overflow\n");
            return false;
        }

        // We could do something to avoid using the buffer, but it should be
        // sufficiently large to hold the suffix anyway so we save some cycles
        // this way by not having to read and then copy into the buffer
        if (AT_cmd_read_data(handle, &res->data[lineLen], 1) <= 0) {
            LOGGER_ERROR("Fatal - unable to read from UART\n");
            return false;
        }
        if (promptLen && lineLen == (promptLen - 1) &&
            strncmp(res->data, handle->AtPrompt, promptLen) == 0) {
            DEBUG("Found prompt\n");
            res->type = AT_LINE_TYPE_PROMPT;
            return true;
        } else if (res->data[lineLen] == sfx[idx]) {
            idx++;
        } else {
            idx = 0;
        }

        lineLen++;
    }

    // TODO: detect empty responses - most likely parser failure (out of sync)
    if (lineLen <= strlen(sfx)) {
        return -1;
    }
    lineLen -= strlen(sfx);

    // Terminate the string for easy handling
    res->data[lineLen] = '\0';
    res->size = lineLen + 1;
    DEBUG("Response: %s\n", res->data);
    return true;
}

#include "helpers/memory.h"

void AT_cmd_register_unsolicited(AT_Handle handle,
                                 const AT_UnsolicitedRes *resList,
                                 void *context)
{
    if (!handle) {
        return;
    }

    handle->unsolicitedResponses = resList;
    handle->cbContext = context;
}

// TODO: more consistent naming for string buffers (str, buf, etc.)
// TODO: should this really be calling the unsolicited response cb too?
static bool check_unsolicited(AT_Handle handle, At_RawResponse *rawRes)
{
    if (!handle->unsolicitedResponses) {
        return false;
    }

    if (rawRes->type != AT_LINE_TYPE_RESPONSE) {
        return false;
    }

    int i = 0;
    const char *str = (char *)rawRes->data;
    while (handle->unsolicitedResponses[i].fmt) {
        const AT_UnsolicitedRes *curItem = &handle->unsolicitedResponses[i];
        // TODO: can probably clean up a bit to avoid strlen, but I'm not worried
        if (strncmp(curItem->fmt, str, strlen(curItem->fmt)) == 0) {
            if (curItem->cb) {
                // See if there's more than just the prefix (did we get data?)
                AT_Response res;
                if (strlen(str) > strlen(curItem->fmt)) {
                    AT_cmd_parse_response(str, curItem->fmt, &res);
                } else {
                    res.paramCount = 0;
                }
                return curItem->cb(&res, handle->cbContext);
            }

            return true;
        }
        ++i;
    }
    return false;
}

void ReadThread(UArg data, UArg unused)
{
    AT_Handle handle = (AT_Handle)(uintptr_t)data;
    while (true) {
        At_RawResponse res;
        if (AT_cmd_read_line(handle, &res)) {
            // see if this is an unsolicited response
            if (!check_unsolicited(handle, &res)) {
                Mailbox_post(handle->inbox, &res, BIOS_WAIT_FOREVER);
            }
        }
        FLUSH();
    }
}

AT_Handle AT_cmd_init(UART_Handle hCom, const AT_UnsolicitedRes *resList,
                      void *context)
{
    AT_Handle handle = (AT_Info *)zalloc(sizeof(AT_Info));
    handle->uartHandle = hCom;
    handle->responseTimeout = AT_RES_DEFAULT_TIMEOUT;

    AT_cmd_register_unsolicited(handle, resList, context);

    // Flush the UART as best we can
    // TODO: this might cause morebugs than it fixes
    //AT_cmd_clear_buf(handle, 0);

    handle->inbox = Mailbox_create(sizeof(At_RawResponse), 10, NULL, NULL);

    // Create the input reader task
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = AT_READ_TASK_STACK_SIZE;
    taskParams.priority = AT_READ_TASK_PRIORITY;
    taskParams.arg0 = (uintptr_t)handle;
    Task_Handle thread = Task_create(ReadThread, &taskParams, NULL);

    if (!thread) {
        LOGGER_ERROR("Fatal - unable to start input thread\n");
        free(handle);
        return NULL;
    }

    // Make sure responses will be formatted as expected
    if (AT_cmd(handle, NULL, "V1") && AT_cmd(handle, NULL, "E0")) {
        return handle;
    }

    free(handle);
    return NULL;
}

static AtResultCode get_result_code(const char *str)
{
    for (int i = 0; i < ARRAY_SIZE(AtResultStringMap); ++i) {
        if (strcmp(str, AtResultStringMap[i].str) == 0) {
            return AtResultStringMap[i].code;
        }
    }

    for (int i = 0; i < ARRAY_SIZE(AtCustomResultStrings); ++i) {
        // TODO: this is kind of gross. Time for a custom function?
        if (strncmp(str, AtCustomResultStrings[i].str,
                    strlen(AtCustomResultStrings[i].str)) == 0) {
            return AtCustomResultStrings[i].code;
        }
    }
    return AT_RESULT_CODE_INVALID;
}

#include <ctype.h>

bool AT_cmd_parse_response(const char *str, const char *cmd,
                           AT_Response *res_out)
{
    // Information messages with numerical data are usually of the form
    // +<CMD>: <d0>, <d1>, ...<dn>

    // NOTE: to avoid passing extra strings we can compare to
    // cmd_fmt - the string before the ':' should exactly match the first x
    // characters from the cmd_fmt, regardless of whether or not the fmt has
    // paramaters after the command
    const char *cur = strchr(str, ':');
    if (!cur) {
        return false;
    }

    // Make sure this is from the command we expect
    if (cmd) {
        int cmd_len = (cur - str);
        // TODO: double-check the strncmp's here - can probably use memcmp instead
        if ((strlen(cmd) < cmd_len) || (strncmp(str, cmd, cmd_len) != 0)) {
            DEBUG("Response %.*s is not for %s\n", cmd_len, str, cmd);
            return false;
        }
    }

    const char *next = cur; // Skip over colon

    char *pStrBuf = res_out->strBuf;

    res_out->paramCount = 0;
    // Pull out numerical data
    DEBUG("Parsed info: ");
    int i;
    for (i = 0; i < AT_MAX_PARAMS; ++i) {
        cur = next + 1; // We want characters AFTER the delimeter

        // Get rid of any leading whitespace
        while (isspace(*(cur))) {
            cur++;
        }

        // See if this is a string
        if (*cur == '"') {
            res_out->param[i] = (AT_Param){
                .type = AT_PARAM_TYPE_STR,
                .pStr = pStrBuf,
            };

            // Find the next quote
            cur++;
            next = strchr(cur, '"');
            if (!next) {
                DEBUG("Parsing error - couldn't find enclosing quote\n");
                return false;
            }
            size_t strLen = (next - cur);
            memcpy(pStrBuf, cur, strLen);
            pStrBuf[strLen] = '\0';
            pStrBuf += strLen + 1; // +1 to account for terminator
            next++;

            DEBUG("S:\"%s\" ", res_out->param[i].pStr);
        } else {
            /* TODO: do we need to handle negative values? */
            uint64_t val = strtoull(cur, (char **)&next, 10);
            if (val > UINT32_MAX) {
                memcpy(pStrBuf, &val, sizeof(val));
                res_out->param[i] = (AT_Param){
                    .type = AT_PARAM_TYPE_INT64,
                    .pInt64 = (uint64_t *)pStrBuf,
                };
                pStrBuf += sizeof(val);
                DEBUG("I64:%" PRIu64 " ", val);
            } else {
                res_out->param[i] = (AT_Param){
                    .type = AT_PARAM_TYPE_INT,
                    .vInt = (uint32_t)val,
                };
                DEBUG("I:%d ", res_out->param[i].vInt);
            }
        }

        if (!(*next)) {
            break;
        }
    };
    DEBUG("\n");

    if (i >= AT_MAX_PARAMS) {
        DEBUG("WARN: too many parameters to parse\n");
        --i;
    }
    res_out->paramCount = i + 1;
    return true;
}

// TODO: slightly more efficient to combine pfx&sfx into cmd, but code
// will probably look more ugly :(
static bool v_write_command(AT_Handle handle, const char *cmd_fmt, va_list argv)
{
    // Add arguments to command
    char cmd[CMD_MAX_LEN];
    int cmd_len = vsnprintf(cmd, CMD_MAX_LEN, cmd_fmt, argv);
    if ((cmd_len > CMD_MAX_LEN) || (cmd_len < 0)) {
        LOGGER_ERROR("Error forming command. vsnprintf returned %d\n", cmd_len);
        return false;
    }

    DEBUG("---------------------------------\n");
    DEBUG("%s%s%s\n", CMD_PREFIX, cmd, CMD_SUFFIX);

    return AT_cmd_write_data(handle, CMD_PREFIX, strlen(CMD_PREFIX)) &&
           AT_cmd_write_data(handle, cmd, cmd_len) &&
           AT_cmd_write_data(handle, CMD_SUFFIX, strlen(CMD_SUFFIX));
}

// TODO: should probably ensure rx buffer is empty before sending new command, in case parser messed up
// TODO: I think this is dead code now
bool AT_cmd_write_command(AT_Handle handle, const char *cmd_fmt, ...)
{
    if (!handle) {
        return false;
    }

    bool res;
    va_list argv;
    va_start(argv, cmd_fmt);
    res = v_write_command(handle, cmd_fmt, argv);
    va_end(argv);

    return res;
}

// TODO: this & function below are way too similar
bool AtCmd_enterBinaryMode(AT_Handle handle, const char *prompt,
                           const char *cmd_fmt, ...)
{
    if (!handle) {
        return false;
    }

    // TODO: this should be set to NULL at some point
    handle->AtPrompt = prompt;

    bool res;
    va_list argv;
    va_start(argv, cmd_fmt);
    res = v_write_command(handle, cmd_fmt, argv);
    va_end(argv);

    if (!res) {
        return false;
    }

    // Process response - loop until we get a result code or our prompt
    // sometimes we'll get an info response, but we don't care right now
    At_RawResponse at_res;
    while (Mailbox_pend(handle->inbox, &at_res, handle->responseTimeout)) {
        switch (at_res.type) {
            case AT_LINE_TYPE_PROMPT:
                return true;
            case AT_LINE_TYPE_RESPONSE:
                if (get_result_code((char *)at_res.data) !=
                    AT_RESULT_CODE_INVALID) {
                    return false;
                }
                break;
            default:
                // some other response
                break;
        }
    }

    return false;
}

void AT_cmd_set_timeout(AT_Handle handle, uint32_t timeout)
{
    if (handle) {
        handle->responseTimeout = timeout;
    }
}

// TODO: this function needs some love
bool AT_cmd_get_response(AT_Handle handle, void *res_buf, size_t res_len)
{
    if (!handle) {
        return false;
    }

    // Process response - loop until we get a result code
    bool info_resp = false;
    At_RawResponse res; // TODO: Doesn't really need to be on the stack

    // Wait for next line
    while (Mailbox_pend(handle->inbox, &res, handle->responseTimeout)) {
        if (res.type != AT_LINE_TYPE_RESPONSE ||
            (get_result_code((char *)res.data) == AT_RESULT_CODE_INVALID)) {
            // TODO: should verify response somehow

            if (res.type != AT_LINE_TYPE_BINARY) {
                DEBUG("info: %s\n", res.data);
            }
            info_resp = true;

            if (res_buf) {
                // TODO: bounds checking
                // TODO: I probably shouldn't copy in data for a response I'm not expecting (eg unhandled unsolicited response)
                memcpy(res_buf, res.data, MIN(res_len, res.size));
            } else {
                DEBUG("Wasn't expecting information response but got one anyway\n");
            }
            continue;
        }

        // See if this was a result code
        switch (get_result_code((char *)res.data)) {
            case AT_RESULT_CODE_ERROR_CUSTOM:
            case AT_RESULT_CODE_ERROR:
                LOGGER_ERROR("%s: ERROR\n", res.data);
                return false;
            case AT_RESULT_CODE_OK:
                if (res_buf && !info_resp) {
                    DEBUG("was expecting information response but didn't get one\n");
                }
                DEBUG("%s: OK\n", res.data);
                return true;
            case AT_RESULT_CODE_INVALID:
                LOGGER_ERROR("Fatal error - unknown result code: %s\n",
                             res.data);
                return false;
        }
    }

    /* Timeout */
    LOGGER_ERROR("Timeout waiting for AT response\n");

    return false;
}

static void empty_response_buffer(AT_Handle handle)
{
    At_RawResponse res;
    while (Mailbox_pend(handle->inbox, &res, BIOS_NO_WAIT)) {
        LOGGER_WARNING("Unhandled AT response: %s\n", res.data);
    }
}

static bool v_at_cmd_raw(AT_Handle handle, void *res_buf, size_t res_len,
                         const char *cmd_fmt, va_list argv)
{
    empty_response_buffer(handle); /* Just in case we missed a response */
    if (!v_write_command(handle, cmd_fmt, argv)) {
        LOGGER_ERROR("Error writing command :O\n");
        return false;
    }
    return AT_cmd_get_response(handle, res_buf, res_len);
}

bool AT_cmd(AT_Handle handle, AT_Response *res_out, const char *cmd_fmt, ...)
{
    if (!handle) {
        return false;
    }

    bool res;
    char tmpBuf[100]; // TODO: magic number

    void *buf = res_out ? tmpBuf : NULL; // TODO: gross
    const size_t bufLen = res_out ? sizeof(tmpBuf) : 0;

    va_list argv;
    va_start(argv, cmd_fmt);
    res = v_at_cmd_raw(handle, buf, bufLen, cmd_fmt, argv);
    va_end(argv);

    if (!buf) {
        return res;
    }

    if (res) {
        return AT_cmd_parse_response(buf, cmd_fmt, res_out);
    }
    return false;
}

bool AT_cmd_raw(AT_Handle handle, void *res_buf, size_t res_len,
                const char *cmd_fmt, ...)
{
    if (!handle) {
        return false;
    }

    bool res;
    va_list argv;
    va_start(argv, cmd_fmt);
    res = v_at_cmd_raw(handle, res_buf, res_len, cmd_fmt, argv);
    va_end(argv);

    return res;
}
