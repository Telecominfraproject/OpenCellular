/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef INC_DEVICES_SLB9645_H_
#define INC_DEVICES_SLB9645_H_

#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/i2cbus.h"

typedef enum SLB9645_Event {

	EVENT = 0,
} SLB9645_Event;

typedef void (*SLB9645_CallbackFn) (SLB9645_Event evt, int8_t temperature,
                                  void *context);
typedef struct SLB9645_Cfg {
    I2C_Dev dev;
    OcGpio_Pin *pin_evt;
} SLB9645_Cfg;

typedef struct SLB9645_Obj {
	SLB9645_CallbackFn alert_cb;
    void *cb_context;
    //GateMutex_Handle mutex;
} SLB9645_Obj;

typedef struct SLB9645_Dev {
    const SLB9645_Cfg cfg;
    SLB9645_Obj obj;
} SLB9645_Dev;

#endif /* INC_DEVICES_SLB9645_H_ */
