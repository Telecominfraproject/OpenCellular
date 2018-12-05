/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef LTC4295_H_
#define LTC4295_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/global_header.h"

#include <ti/sysbios/gates/GateMutex.h>

typedef enum {
    LTC4295_STATUS_CLASS = 0x00,
    LTC4295_STATUS_POWERGOOD = 0x01,
}eltc4295StatusParamId;

typedef enum {
    LTC4295_POWERGOOD = 0,
    LTC4295_POWERGOOD_NOTOK
} ePDPowerState;

typedef enum {
    LTC4295_CLASSTYPE_UNKOWN = 0,
    LTC4295_CLASSTYPE_1,
    LTC4295_CLASSTYPE_2,
    LTC4295_CLASSTYPE_3,
    LTC4295_CLASSTYPE_POEPP
} ePDClassType;

typedef enum {
    LTC4295_STATE_OK = 0,
    LTC4295_STATE_NOTOK
} ePDState;

typedef enum {
    LTC4295_CONNECT_ALERT = 1,
    LTC4295_DISCONNECT_ALERT,
    LTC4295_INCOMPATIBLE_ALERT
} ePDAlert;

typedef enum {
    LTC4295_CONNECT_EVT = 1 << 2,       /* PD device Connected. */
    LTC4295_DISCONNECT_EVT = 1 << 1,    /* PD device removed.   */
    LTC4295_INCOMPATIBLE_EVT = 1 << 0,  /* Incomaptible device  */
} LTC4295_Event;

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t     classStatus;
    uint8_t     powerGoodStatus;
}tPower_PDStatus;

typedef struct __attribute__((packed, aligned(1))) {
    tPower_PDStatus pdStatus;
    ePDState        state;
    ePDAlert        pdalert;
}tPower_PDStatus_Info;

typedef struct LTC4295_Cfg {
    OcGpio_Pin *pin_evt;
    OcGpio_Pin *pin_detect;
} LTC4295_Cfg;

typedef void (*LTC4295_CallbackFn) (LTC4295_Event evt,
                                  void *context);
typedef struct LTC4295_Obj {
    LTC4295_CallbackFn alert_cb;
    void *cb_context;
    GateMutex_Handle mutex;
} LTC4295_Obj;

typedef struct LTC4295A_Dev {
    const LTC4295_Cfg cfg;
    LTC4295_Obj obj;
} LTC4295_Dev;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
void ltc4295_config(const LTC4295_Dev *dev);
ePostCode ltc4295_probe(const LTC4295_Dev *dev, POSTData *postData);
ReturnStatus ltc4295_init(LTC4295_Dev *dev);
void ltc4295_set_alert_handler(LTC4295_Dev *dev, LTC4295_CallbackFn alert_cb, void *cb_context);
ReturnStatus ltc4295_get_power_good(const LTC4295_Dev *dev, ePDPowerState *val);
ReturnStatus ltc4295_get_class(const LTC4295_Dev *dev, ePDClassType *val);
void ltc4295_update_status(const LTC4295_Dev *dev);

#endif /* LTC4295_H_ */
