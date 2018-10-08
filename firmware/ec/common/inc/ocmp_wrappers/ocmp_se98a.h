/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_SE98A_H
#define _OCMP_SE98A_H

#include "common/inc/global/Framework.h"

typedef union SE98A_Config {
    struct {
        int8_t lowlimit;
        int8_t highlimit;
        int8_t critlimit;
    };
    int8_t limits[3];
} SE98A_Config;

SCHEMA_IMPORT const Driver_fxnTable SE98_fxnTable;

static const Driver SE98A = {
    .name = "SE98A",
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
    .fxnTable = &SE98_fxnTable,
};

#endif /* _OCMP_SE98A_H */
