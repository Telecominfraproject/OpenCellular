/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_LED_H
#define _OCMP_LED_H

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable LED_fxnTable;
SCHEMA_IMPORT bool led_testpattern_control(void *driver, void *param);
static const Driver HCI_LED = {
    .name = "HCI_LED",
    .status = NULL,
    .config = NULL,
    .alerts = NULL,
    .commands = (Command[]){ {
                                     .name = "set",
                                     .cb_cmd = led_testpattern_control,
                             },
                             {} },
    .fxnTable = &LED_fxnTable,
};

#endif /* _OCMP_LED_H */
