/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef LTC4275_H_
#define LTC4275_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/global_header.h"

#include <ti/sysbios/gates/GateMutex.h>

typedef enum {
    LTC4275_STATUS_CLASS = 0x00,
    LTC4275_STATUS_POWERGOOD = 0x01,
} eltc4275StatusParamId;

typedef enum { LTC4275_POWERGOOD = 0, LTC4275_POWERGOOD_NOTOK } ePDPowerState;

typedef enum {
    LTC4275_CLASSTYPE_UNKOWN = 0,
    LTC4275_CLASSTYPE_1,
    LTC4275_CLASSTYPE_2,
    LTC4275_CLASSTYPE_3,
    LTC4275_CLASSTYPE_POEPP
} ePDClassType;

typedef enum { LTC4275_STATE_OK = 0, LTC4275_STATE_NOTOK } ePDState;

typedef enum {
    LTC4275_CONNECT_ALERT = 1,
    LTC4275_DISCONNECT_ALERT,
    LTC4275_INCOMPATIBLE_ALERT
} ePDAlert;

typedef enum {
    LTC4275_CONNECT_EVT = 1 << 2, /* PD device Connected. */
    LTC4275_DISCONNECT_EVT = 1 << 1, /* PD device removed.   */
    LTC4275_INCOMPATIBLE_EVT = 1 << 0, /* Incomaptible device  */
} LTC4275_Event;

typedef struct __attribute__((packed, aligned(1))) {
    uint8_t classStatus;
    uint8_t powerGoodStatus;
} tPower_PDStatus;

typedef struct __attribute__((packed, aligned(1))) {
    tPower_PDStatus pdStatus;
    ePDState state;
    ePDAlert pdalert;
} tPower_PDStatus_Info;

typedef struct LTC4275_Cfg {
    OcGpio_Pin *pin_evt;
    OcGpio_Pin *pin_detect;
} LTC4275_Cfg;

typedef void (*LTC4275_CallbackFn)(LTC4275_Event evt, void *context);
typedef struct LTC4275_Obj {
    LTC4275_CallbackFn alert_cb;
    void *cb_context;
    GateMutex_Handle mutex;
} LTC4275_Obj;

typedef struct LTC4275A_Dev {
    const LTC4275_Cfg cfg;
    LTC4275_Obj obj;
} LTC4275_Dev;

/*****************************************************************************
 *                             FUNCTION DECLARATIONS
 *****************************************************************************/
void ltc4275_config(const LTC4275_Dev *dev);
ePostCode ltc4275_probe(const LTC4275_Dev *dev, POSTData *postData);
ReturnStatus ltc4275_init(LTC4275_Dev *dev);
void ltc4275_set_alert_handler(LTC4275_Dev *dev, LTC4275_CallbackFn alert_cb,
                               void *cb_context);
ReturnStatus ltc4275_get_power_good(const LTC4275_Dev *dev, ePDPowerState *val);
ReturnStatus ltc4275_get_class(const LTC4275_Dev *dev, ePDClassType *val);
void ltc4275_update_status(const LTC4275_Dev *dev);

#endif /* LTC4275_H_ */
