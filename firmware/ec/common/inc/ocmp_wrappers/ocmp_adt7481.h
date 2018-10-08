/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_ADT7481_H
#define _OCMP_ADT7481_H

#include "common/inc/global/Framework.h"

typedef union ADT7481_Config {
    struct {
        int8_t lowlimit;
        int8_t highlimit;
        int8_t critlimit;
    };
    int8_t limits[3];
} ADT7481_Config;

#ifdef UT_FRAMEWORK
extern const Driver_fxnTable ADT7481_fxnTable;
#else
SCHEMA_IMPORT const Driver_fxnTable ADT7481_fxnTable;
#endif

static const Driver ADT7481 = {
    .name = "ADT7481",
    .status =
            (Parameter[]){ { .name = "temperature", .type = TYPE_UINT8 }, {} },
    .config = (Parameter[]){ { .name = "lowlimit", .type = TYPE_INT8 },
                             { .name = "highlimit", .type = TYPE_UINT8 },
                             { .name = "critlimit", .type = TYPE_UINT8 },
                             {} },
    .alerts = (Parameter[]){ { .name = "BAW", .type = TYPE_UINT8 },
                             { .name = "AAW", .type = TYPE_UINT8 },
                             { .name = "ACW", .type = TYPE_UINT8 },
                             {} },
    .fxnTable = &ADT7481_fxnTable,
};

#endif /* _OCMP_ADT7481_H */
