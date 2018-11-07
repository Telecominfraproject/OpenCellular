/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once

#ifndef AT_CMD_H_
#    define AT_CMD_H_

#    include <ti/drivers/UART.h>

#    include <inttypes.h>
#    include <stdbool.h>

typedef struct AT_Info *AT_Handle;

// TODO: this needs to be more like 22
#    define AT_MAX_PARAMS 12
#    define AT_STR_BUF_SIZE 100

/* TODO: timeouts are in sys ticks, not ms - macro? */
#    define AT_RES_DEFAULT_TIMEOUT 5000

typedef enum AT_ParamType {
    AT_PARAM_TYPE_INT,
    AT_PARAM_TYPE_INT64,
    AT_PARAM_TYPE_STR,
} AT_ParamType;

typedef struct AT_Param {
    AT_ParamType type;
    union {
        char *pStr;
        long vInt;
        uint64_t *pInt64;
    };
} AT_Param;

typedef struct AT_Response {
    int paramCount;
    AT_Param param[AT_MAX_PARAMS];
    char strBuf[AT_STR_BUF_SIZE];
} AT_Response;

/* TODO: change this back to void and add new function to insert response
 * back into queue since it's a rare case (although not easy right now given how
 * parsing works)
 */
/* Return true if response handled, false if response should be added back
 * into response queue (in cases when a response can be either unsolicited or
 * an info response)
 */
typedef bool (*unsolicitedCallback)(AT_Response *res, void *context);
typedef struct AT_UnsolicitedRes {
    const char *fmt;
    unsolicitedCallback cb;
} AT_UnsolicitedRes;

AT_Handle AT_cmd_init(UART_Handle hCom, const AT_UnsolicitedRes *resList,
                      void *context);

// Parse the response and optionally verify that the response is from the
// expected command
// Note: it will only verify up to the colon, so cmd formats are fine in
// Example: +SBDCMD%u will match the response +SBDCMD: 1, 2, 3
bool AT_cmd_parse_response(const char *str, const char *cmd,
                           AT_Response *res_out);

bool AT_cmd_write_command(AT_Handle handle, const char *cmd_fmt, ...);

bool AT_cmd_write_data(AT_Handle handle, const void *data, size_t data_len);

// Writes 16 bytes of data, accounting for endianness
bool AT_cmd_write16(AT_Handle handle, uint16_t data);

int AT_cmd_read_data(AT_Handle handle, void *data, size_t data_len);

// Reads 16 bytes of data, accounting for endianness
bool AT_cmd_read16(AT_Handle handle, uint16_t *data);

// Reads n bytes from the buffer and discards them, or if bytes is zero, reads
// from the buffer until it is empty
void AT_cmd_clear_buf(AT_Handle handle, int bytes);

typedef int (*resCallback)(const char *buf, void *res_buf, size_t resp_len);

/* Sets the timeout for AT command responses */
void AT_cmd_set_timeout(AT_Handle handle, uint32_t timeout);

bool AT_cmd_get_response(AT_Handle handle, void *res_buf, size_t res_len);

bool AtCmd_enterBinaryMode(AT_Handle handle, const char *prompt,
                           const char *cmd_fmt, ...);

bool AT_cmd(AT_Handle handle, AT_Response *res_out, const char *cmd_fmt, ...);

bool AT_cmd_raw(AT_Handle handle, void *res_buf, size_t res_len,
                const char *cmd_fmt, ...);

// NOTE: resList bust end with blank entry & be always available
// (i.e. it's not copied right now)
void AT_cmd_register_unsolicited(AT_Handle handle,
                                 const AT_UnsolicitedRes *resList,
                                 void *context);

// NOTE: handler automatically unregistered after transfer
typedef int (*AtBinaryReadHandler)(AT_Handle handle, void *buf);
void AT_cmd_register_binary_handler(AT_Handle handle,
                                    AtBinaryReadHandler handler);

#endif /* AT_CMD_H_ */
