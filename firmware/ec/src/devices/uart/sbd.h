/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once

#include <stdbool.h>

#include <ti/drivers/UART.h>

// TODO: this feels wrong
typedef struct AT_Info *AT_Handle;
typedef AT_Handle SBD_Handle;

typedef enum SbdMoStatus {
    SBD_MO_STATUS_SUCCESS = 0,
    SBD_MO_STATUS_MT_TOO_BIG = 1,
    SBD_MO_STATUS_LOC_NOT_ACCEPTED = 2,
    // 3..4 - reserved, but MO success

    SBD_MO_STATUS_FAILURE = 5,
    // 5..8 - reserved, but MO failure
    SBD_MO_STATUS_GSS_TIMEOUT = 10,
    SBD_MO_STATUS_GSS_QUEUE_FULL = 11,
    SBD_MO_STATUS_TOO_MANY_SEGMENTS = 12,
    SBD_MO_STATUS_INCOMPLETE_SESSION = 13,
    SBD_MO_STATUS_INVALID_SEGMENT_SIZE = 14,
    SBD_MO_STATUS_ACCESS_DENIED = 15,

    SBD_MO_STATUS_ISU_LOCKED = 16,
    SBD_MO_STATUS_ISU_TIMEOUT = 17,
    SBD_MO_STATUS_RF_DROP = 18,
    SBD_MO_STATUS_LINK_FAILURE = 19,
    // 20..31 reserved, but MO failure
    SBD_MO_STATUS_NO_NETWORK = 32,
    SBD_MO_STATUS_ANTENNA_FAULT = 33,
    SBD_MO_STATUS_RADIO_DISABLED = 34,
    SBD_MO_STATUS_ISU_BUSY = 35,
    SBD_MO_STATUS_REGISTRATION_COOLDOWN = 36,
    SBD_MO_STATUS_SERVICE_DISABLED = 37,
    // 39..63 reserved, but MO failure
    SBD_MO_STATUS_BAND_VIOLATION = 64,
    SBD_MO_STATUS_PLL_LOCK_FAILURE = 65,
} SbdMoStatus;

typedef enum SbdMtStatus {
    SBD_MT_STATUS_NO_MSG = 0,
    SBD_MT_STATUS_SUCCESS = 1,
    SBD_MT_STATUS_FAILURE = 2,
} SbdMtStatus;

typedef struct SbdixInfo {
    int moStatus;
    int moMsn;

    int mtStatus;
    int mtMsn;
    int mtLength;
    int mtQueued;
} SbdixInfo;

typedef struct SbdsInfo {
    int moFlag; //!< Message in mobile originated buffer
    int moMsn; //!< MO message sequence number

    int mtFlag; //!< Message in mobile terminated buffer
    int mtMsn; //!< MT Message sequence number
} SbdsInfo;

typedef struct SbdsxInfo {
    SbdsInfo sbdsInfo; //!< Regular SBD status info
    int raFlag; //!< Ring alert still needs to be answered
    int msgWaiting; //!< Number of MT messages at gateway
            //!< (updated every SBD session)
} SbdsxInfo;

typedef enum SbdCiev {
    SBD_CIEV_SIGIND = 0,
    SBD_CIEV_SVCIND,
    SBD_CIEV_ANTIND,
    SBD_CIEV_SV_BEAM_COORDS_IND,
} SbdCiev;

typedef struct SbdCievInfo {
    SbdCiev event : sizeof(int);
    union {
        int rssi;
        int value;
    };
    // TODO: for event 3, there's lots more data - worth dealing with?
} SbdCievInfo;

typedef void (*SbdRingCb)(void *context);
typedef void (*SbdCievCb)(const SbdCievInfo *info, void *context);
typedef struct SbdCallbackList {
    SbdRingCb sbdring;
    SbdCievCb ciev;
} SbdCallbackList;

SBD_Handle SBD_init(UART_Handle hCom, const SbdCallbackList *cbList,
                    void *cbContext);

bool SBD_sbdix(SBD_Handle handle, SbdixInfo *info_out, bool alert_response);

bool SBD_sbdwb(SBD_Handle handle, const void *data, int data_len);

int SBD_sbdrb(SBD_Handle handle, void *buffer, int buffer_len);

bool SBD_sbds(SBD_Handle handle, SbdsInfo *info_out);

bool SBD_sbdsx(SBD_Handle handle, SbdsxInfo *info_out);

typedef struct SbdcgsnInfo {
    char imei[15];
} SbdcgsnInfo;
bool SBD_cgsn(SBD_Handle handle, SbdcgsnInfo *info_out);

typedef struct SbdCgmiInfo {
    char mfg[10];
} SbdCgmiInfo;
bool SBD_cgmi(SBD_Handle handle, SbdCgmiInfo *info_out);

typedef struct SbdCgmmInfo {
    char model[40];
} SbdCgmmInfo;

bool SBD_cgmm(SBD_Handle handle, SbdCgmmInfo *info_out);

typedef enum SbdFlowControl {
    SBD_FLOW_CONTROL_DISABLED = 0,
    SBD_FLOW_CONTROL_HW = 3,
    SBD_FLOW_CONTROL_SW = 4,
    SBD_FLOW_CONTROL_ALL = 6
} SbdFlowControl;
bool SBD_k(SBD_Handle handle, SbdFlowControl flowControl);

typedef enum SbdDeleteType {
    SBD_DELETE_TYPE_MO = 0,
    SBD_DELETE_TYPE_MT = 1,
    SBD_DELETE_TYPE_ALL = 2
} SbdDeleteType;
bool SBD_sbdd(SBD_Handle handle, SbdDeleteType deleteType);

typedef struct SbdcsqInfo {
    int rssi;
} SbdcsqInfo;

// NOTE: CSQ turns on the antenna, using more power. Use sparingly.
// Alternatively use CSQF which returns immediately but is slower to update
bool SBD_csq(SBD_Handle handle, SbdcsqInfo *info_out);
bool SBD_csqf(SBD_Handle handle, SbdcsqInfo *info_out);

typedef enum SbdAregMode {
    SBD_AREG_MODE_DISABLED = 0,
    SBD_AREG_MODE_AUTO = 1,
    SBD_AREG_MODE_ASK = 2,
    SBD_AREG_MODE_AUTO_EVT3 = 3,
    SBD_AREG_MODE_ASK_EVT3 = 4,
} SbdAregMode;
bool SBD_sbdareg(SBD_Handle handle, SbdAregMode mode);

typedef enum SbdRegStat {
    SBD_REG_DETACHED = 0x00,
    SBD_REG_NONE = 0x01,
    SBD_REG_REGISTERED = 0x02,
    SBD_REG_DENIED = 0x03,
} SbdRegStat;
bool SBD_sbdregRead(SBD_Handle handle, SbdRegStat *status_out);

bool SBD_sbdtc(SBD_Handle handle);

// TODO: this doesn't appear to be SBD-specific, should it be here? Same goes
// for csq, k, etc.
bool SBD_cier(SBD_Handle handle, bool mode, bool sigind, bool svcind,
              bool antind, bool sv_beam_coords_ind);

// SBD Mobile Terminated Alert (enable/disable SBDRING)
bool SBD_sbdmta(SBD_Handle handle, bool mode);

// TODO: just for testing - remove/move to unit tests when they're set up
void SBD_test_invalid(SBD_Handle);
