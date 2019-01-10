/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef INC_DEVICES_MP2951_H_
#define INC_DEVICES_MP2951_H_

#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/i2cbus.h"

typedef enum MP2951_Event {
	EVENT_MP2951 = 0,
} MP2951_Event;

typedef void (*MP2951_CallbackFn) (MP2951_Event evt, int8_t temperature,
                                  void *context);

typedef struct MP2951_Cfg {
    I2C_Dev dev;
    OcGpio_Pin *pin_evt;
} MP2951_Cfg;

typedef struct MP2951_Obj {
	MP2951_CallbackFn alert_cb;
    void *cb_context;
    //GateMutex_Handle mutex;
} MP2951_Obj;

typedef struct MP2951_Dev {
    const MP2951_Cfg cfg;
    MP2951_Obj obj;
} MP2951_Dev;

ReturnStatus MP2951_setInputVolOnLim(MP2951_Dev *dev, uint16_t voltLim);
ReturnStatus MP2951_setInputVolOffLim(MP2951_Dev *dev, uint16_t voltLim);
ReturnStatus MP2951_readInputVolOnLim(MP2951_Dev *dev, uint16_t *voltLim);
ReturnStatus MP2951_readInputVolOffLim(MP2951_Dev *dev, uint16_t *voltLim);
ReturnStatus mp2951_getDevId(MP2951_Dev *dev, uint8_t *devID);
ReturnStatus mp2951_getVendId(MP2951_Dev *dev, uint8_t *vId);
#endif /* INC_DEVICES_MP2951_H_ */
