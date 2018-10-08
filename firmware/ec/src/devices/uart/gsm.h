/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once

#ifndef GSM_H_
#define GSM_H_

#include <ti/drivers/UART.h>

#include <stdbool.h>
#include <inttypes.h>

typedef struct AT_Info *AT_Handle;
typedef AT_Handle GSM_Handle;

typedef enum GsmCregStat {
    GSM_CREG_STAT_NREG_NSEARCH = 0x00,
    GSM_CREG_STAT_REG_HOME = 0x01,
    GSM_CREG_STAT_NREG_SEARCH = 0x02,
    GSM_CREG_STAT_DENIED = 0x03,
    GSM_CREG_STAT_UNKNOWN = 0x04,
    GSM_CREG_STAT_REG_ROAMING = 0x05,
} GsmCregStat;

typedef struct GsmCmtiInfo {
    const char *mem;
    unsigned int index;
} GsmCmtiInfo;

typedef enum GsmCallState {
    GSM_CALL_STATE_ACTIVE = 0,
    GSM_CALL_STATE_HELD = 1,

    GSM_CALL_STATE_DIALING = 2, /* MO */
    GSM_CALL_STATE_ALERTING = 3, /* MO */

    GSM_CALL_STATE_INCOMING = 4, /* MT */
    GSM_CALL_STATE_WAITING = 5, /* MT */

    GSM_CALL_STATE_RELEASED = 6,
} GsmCallState;

typedef struct GsmClccInfo {
    uint8_t idx;
    uint8_t dir;
    GsmCallState call_state;
    uint8_t mode;
    uint8_t mpty;
    char number[33];
    uint8_t type;
} GsmClccInfo;

typedef void (*GsmCregCb)(const GsmCregStat stat, void *context);
typedef void (*GsmSimReadyCb)(void *context);
typedef void (*GsmCmtiCb)(const GsmCmtiInfo *info, void *context);
typedef void (*GsmRingCb)(void *context);
typedef void (*GsmClccCb)(const GsmClccInfo *info, void *context);
typedef struct GsmCallbackList {
    GsmCregCb creg;
    GsmSimReadyCb simReady;
    GsmCmtiCb cmti;
    GsmRingCb ring;
    GsmClccCb clcc;
} GsmCallbackList;

GSM_Handle GSM_init(UART_Handle hCom, const GsmCallbackList *cbList,
                    void *cbContext);

typedef struct GsmCgsnInfo {
    char imei[16];
} GsmCgsnInfo;
bool GSM_cgsn(GSM_Handle handle, GsmCgsnInfo *info_out);

bool GSM_cimi(GSM_Handle handle, uint64_t *imsi);

typedef enum GsmMessageFormat {
    GSM_MSG_FMT_PDU = 0,
    GSM_MSG_FMT_TEXT = 1
} GsmMessageFormat;
bool GSM_cmgf(GSM_Handle handle, GsmMessageFormat fmt);

// TODO: most of these params should probably be enums or structs
bool GSM_csmp(GSM_Handle handle, int fo, int vp, int pid, int dcs);

// Returns the message index if successful
int GSM_cmgs(GSM_Handle handle, const char *number, const char *msg);

// Enables/disables unsolicited creg message
// TODO: I'm not sold on the enum naming
typedef enum GsmCregMode {
    GSM_CREG_STATUS_DISABLE = 0x0,
    GSM_CREG_STATUS_ENABLE = 0x1,
    GSM_CREG_STATUS_ENABLE_LOC = 0x2,
} GsmCregMode;
bool GSM_creg(GSM_Handle handle, GsmCregMode n);

typedef struct GsmCregInfo {
    GsmCregMode n;
    GsmCregStat stat;
    uint16_t lac;
    uint16_t cid;
} GsmCregInfo;
bool GSM_cregRead(GSM_Handle handle, GsmCregInfo *info_out);

bool GSM_cnmi(GSM_Handle handle, int mode, int mt, int bm, int ds, int bfr);

typedef struct GsmCmgrInfo {
    char stat[12];
    char oa[15];
    char alpha
            [5]; // TODO: this isn't present with our module, what should it be?
    char scts[16]; // service center timestamp
} GsmCmgrInfo;
bool GSM_cmgr(GSM_Handle handle, unsigned int index, char *sms_out,
              GsmCmgrInfo *info_out);

bool GSM_csdh(GSM_Handle handle, bool show);

typedef struct GsmCsqInfo {
    uint8_t rssi;
    uint8_t ber;
} GsmCsqInfo;
bool GSM_csq(GSM_Handle handle, GsmCsqInfo *info_out);

typedef struct GsmCgmiInfo {
    char mfgId[10];
} GsmCgmiInfo;
bool GSM_cgmi(GSM_Handle handle, GsmCgmiInfo *info_out);

typedef struct GsmCgmmInfo {
    char tech[20];
    char model[5];
} GsmCgmmInfo;
bool GSM_cgmm(GSM_Handle handle, GsmCgmmInfo *info_out);

typedef enum GsmCopsMode {
    GSM_COPS_MODE_AUTO = 0,
    GSM_COPS_MODE_MANUAL = 1,
    GSM_COPS_MODE_DEREG = 2,
    GSM_COPS_MODE_SET_FMT = 3,
    GSM_COPS_MODE_MANUAL_AUTO = 4,
} GsmCopsMode;

typedef enum GsmCopsFmt {
    GSM_COPS_FMT_LONG_ALPHA = 0,
    GSM_COPS_FMT_SHORT_ALPHA = 1,
    GSM_COPS_FMT_NUMERIC = 2,
} GsmCopsFmt;
bool GSM_cops(GSM_Handle handle, GsmCopsMode mode, GsmCopsFmt format,
              const char *oper);

typedef enum GsmCopsStat {
    GSM_COPS_STAT_UNKNOWN = 0,
    GSM_COPS_STAT_AVAILABLE = 1,
    GSM_COPS_STAT_CURRENT = 2,
    GSM_COPS_STAT_FORBIDDEN = 3
} GsmCopsStat;

typedef struct GsmOperator {
    GsmCopsStat stat;
    char oper_long[16];
    char oper_short[8];
} GsmOperator;

typedef struct GsmCopsTestInfo {
    int ops_count;
    GsmOperator ops[5];
} GsmCopsTestInfo;

bool GSM_copsTest(GSM_Handle handle, GsmCopsTestInfo *info_out);

bool GSM_a(GSM_Handle handle);

bool GSM_d(GSM_Handle handle, const char *number);

bool GSM_h(GSM_Handle handle);

bool GSM_cnma(GSM_Handle handle);

typedef enum GsmCmgdFlag {
    GSM_CMGD_DELETE_AT_INDEX = 0,
    GSM_CMGD_DELETE_ALL_READ = 1,
    GSM_CMGD_DELETE_ALL_READ_AND_SENT_MO = 2,
    GSM_CMGD_DELETE_ALL_READ_AND_MO = 3,
    GSM_CMGD_DELETE_ALL = 4,
} GsmCmgdFlag;

bool GSM_cmgd(GSM_Handle handle, int index, GsmCmgdFlag flag);

typedef enum GsmCFun {
    GSM_CFUN_OFF = 0, /*!< Power off module */
    GSM_CFUN_FULL = 1, /*!< Enable full radio functionality */
    GSM_CFUN_AIRPLANE = 4, /*!< Turn off radio functionality */
    GSM_CFUN_RESET = 15 /*!< Hardware reset */
} GsmCFun;
bool GSM_cfun(GSM_Handle handle, GsmCFun fun);

bool GSM_clccSet(GSM_Handle handle, bool state);

#endif /* GSM_H_ */
