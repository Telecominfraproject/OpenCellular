/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_INA226_H
#define _OCMP_INA226_H

#include "common/inc/global/Framework.h"

typedef struct INA226_Config {
    uint16_t current_lim;
} INA226_Config;

SCHEMA_IMPORT const Driver_fxnTable INA226_fxnTable;

static const Driver INA226 = {
    .name = "INA226",
    .status = (Parameter[]){ { .name = "busvoltage", .type = TYPE_UINT16 },
                             { .name = "shuntvoltage", .type = TYPE_UINT16 },
                             { .name = "current", .type = TYPE_UINT16 },
                             { .name = "power", .type = TYPE_UINT16 },
                             {} },
    .config = (Parameter[]){ { .name = "currlimit", .type = TYPE_UINT16 }, {} },
    .alerts =
            (Parameter[]){ { .name = "Overcurrent", .type = TYPE_UINT16 }, {} },
    .fxnTable = &INA226_fxnTable,
};

#endif /* _OCMP_INA226_H */
