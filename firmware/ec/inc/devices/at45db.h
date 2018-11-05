/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#ifndef INC_DEVICES_AT45DB_H_
#define INC_DEVICES_AT45DB_H_

#include "common/inc/global/post_frame.h"
#include "drivers/OcGpio.h"
#include "inc/common/spibus.h"
#include "inc/common/global_header.h"

/*****************************************************************************
 *                         STRUCT/ENUM DEFINITIONS
 *****************************************************************************/
typedef enum AT45DB_Event {
    AT45DB_READ_EVENT = 0,
} AT45DB_Event;

typedef void (*AT45DB_CallbackFn)(AT45DB_Event evt, uint16_t value,
                                  void *context);

typedef struct AT45DB_Cfg {
    SPI_Dev dev;
    OcGpio_Pin *pin_alert;
} AT45DB_Cfg;

typedef struct AT45DB_Obj {
    AT45DB_CallbackFn alert_cb;
    void *cb_context;
    AT45DB_Event evt_to_monitor;
} AT45DB_Obj;

typedef struct AT45DB_Dev {
    const AT45DB_Cfg cfg;
    AT45DB_Obj obj;
} AT45DB_Dev;

ePostCode at45db_probe(AT45DB_Dev *dev, POSTData *postData);
ReturnStatus at45db_data_read(AT45DB_Dev *dev, uint8_t *data,
                              uint32_t data_size, uint32_t byte, uint32_t page);
ReturnStatus at45db_data_write(AT45DB_Dev *dev, uint8_t *data,
                               uint32_t data_size, uint32_t byte,
                               uint32_t page);
ReturnStatus at45db_erasePage(AT45DB_Dev *dev, uint32_t page);
uint8_t at45db_readStatusRegister(AT45DB_Dev *dev);

#endif /* INC_DEVICES_AT45DB_H_ */
