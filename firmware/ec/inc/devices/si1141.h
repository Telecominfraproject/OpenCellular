/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef INC_DEVICES_SI1141_H_
#define INC_DEVICES_SI1141_H_

#include "inc/devices/si1141.h"
#include "devices/i2c/threaded_int.h"
#include "inc/common/byteorder.h"
#include "inc/common/global_header.h"
#include "helpers/memory.h"
#include "inc/common/i2cbus.h"

typedef enum SI1141_Event {
	SE98A_EVT = 1 << 2,
} SI1141_Event;

typedef void (*SI1141_CallbackFn) (SI1141_Event evt, uint16_t value,
                                   void *context);
typedef struct SI1141_Cfg {
    I2C_Dev dev;
    OcGpio_Pin *pin_alert;
} SI1141_Cfg;

typedef struct SI1141_Obj {
	SI1141_CallbackFn alert_cb;
    void *cb_context;
    SI1141_Event evt_to_monitor;
} SI1141_Obj;

typedef struct SI1141_Dev {
    const SI1141_Cfg cfg;
    SI1141_Obj obj;
} SI1141_Dev;

ReturnStatus si1141_getSeqId(SI1141_Dev *dev, uint16_t *seqId);
ReturnStatus si1141_getRevId(SI1141_Dev *dev, uint16_t *revId);
ReturnStatus si1141_getPartId(SI1141_Dev *dev, uint16_t *partId);

#endif /* INC_DEVICES_SI1141_H_ */
