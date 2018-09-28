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
#include "common/inc/global/Framework.h"
#include "devices/uart/sbd.h"
#include "drivers/OcGpio.h"
#include "helpers/attribute.h"
#include "inc/devices/sbd.h"

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
typedef struct Obc_gpioCfg {
    OcGpio_Pin *pin_pwr_en;
} Obc_gpioCfg;

typedef union PACKED {
    uint64_t imei;
    char mfg[10];
    char model[5];
    uint8_t rssi;
    SbdRegStat regStat;
    uint8_t outQueueLen;
    OBC_lastError lastErr;
} OBC_Iridium_Status_Data;

bool obc_pre_init(void *driver, void *returnValue);
extern const Driver OBC_Iridium;

#endif /* OBC_OBC_H_ */
