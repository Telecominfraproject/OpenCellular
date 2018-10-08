/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_OCGPIO_H_
#define OCMP_OCGPIO_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT bool ocgpio_get(void *driver, void *data);
SCHEMA_IMPORT bool ocgpio_set(void *driver, void *data);

SCHEMA_IMPORT const Driver_fxnTable DEBUG_OCGPIO_fxnTable;

static const Driver OC_GPIO = {
    .name = "OC_GPIO",
    .argList = (Parameter[]){ { .name = "pin", .type = TYPE_UINT8 },
                              { .name = "value", .type = TYPE_UINT8 },
                              {} },
    .commands = (Command[]){ {
                                     .name = "get",
                                     .cb_cmd = ocgpio_get,
                             },
                             {
                                     .name = "set",
                                     .cb_cmd = ocgpio_set,
                             },
                             {} },
    .fxnTable = &DEBUG_OCGPIO_fxnTable,
};

#endif /* OCMP_OCGPIO_H_ */
