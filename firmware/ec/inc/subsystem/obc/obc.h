/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OBC_H_
#define OBC_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "devices/uart/sbd.h"
#include "drivers/OcGpio.h"
#include "helpers/attribute.h"

#include "src/registry/Framework.h"

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
/* Iridium config */
/* TODO: should move to a separate file */
typedef struct Iridium_Cfg {
    unsigned int uart;
    OcGpio_Pin pin_enable;
    OcGpio_Pin pin_nw_avail;
} Iridium_Cfg;

/* Subsystem config */
typedef struct Obc_Cfg {
    Iridium_Cfg iridium_cfg;
    OcGpio_Pin *pin_pwr_en;
} Obc_Cfg;

typedef enum {
    IRIDIUM_IMEI            = 0,
    IRIDIUM_MFG             = 1,
    IRIDIUM_MODEL           = 2,
    IRIDIUM_SIG_QUALITY     = 3,
    IRIDIUM_REGSTATUS       = 4,
    IRIDIUM_NO_OUT_MSG      = 5,
    IRIDIUM_LASTERR         = 6,
    IRIDIUM_PARAM_MAX       /* Limiter */
} eOBC_StatusParam;

typedef enum {
    ERR_RC_INTERNAL         = 0,
    ERR_SRC_CMS             = 1,
    ERR_SRC_CME             = 2
} eOBC_ErrorSource;

typedef struct OBC_lastError {
    eOBC_ErrorSource src;
    uint16_t code;
} OBC_lastError;

typedef union PACKED {
    uint64_t imei;
    char mfg[10];
    char model[5];
    uint8_t rssi;
    SbdRegStat regStat;
    uint8_t outQueueLen;
    OBC_lastError lastErr;
} OBC_Iridium_Status_Data;

bool obc_pre_init(void *returnValue);
extern const Driver OBC_Iridium;

#endif /* OBC_OBC_H_ */
