/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_LTC4274_H_
#define _OCMP_LTC4274_H_

#include "common/inc/global/Framework.h"

typedef union LTC4274_Config {
    struct {
        int8_t operatingMode;
        int8_t detectEnable;
        int8_t interruptMask;
        bool interruptEnable;
        int8_t pseHpEnable;
    };
} LTC4274_Config;

#ifdef UT_FRAMEWORK
extern const Driver_fxnTable LTC4274_fxnTable;
#else
SCHEMA_IMPORT const Driver_fxnTable LTC4274_fxnTable;
#endif
SCHEMA_IMPORT bool LTC4274_reset(void *driver, void *params);

static const Driver LTC4274 = {
    .name = "PSE",
    .status = (Parameter[]){ { .name = "detection", .type = TYPE_UINT16 },
                             { .name = "class", .type = TYPE_UINT16 },
                             { .name = "powerGood", .type = TYPE_UINT16 },
                             {} },
    .config = (Parameter[]){ { .name = "operatingMode", .type = TYPE_UINT16 },
                             { .name = "detectEnable", .type = TYPE_UINT16 },
                             { .name = "interruptMask", .type = TYPE_UINT16 },
                             { .name = "interruptEnable", .type = TYPE_UINT16 },
                             { .name = "enableHighpower", .type = TYPE_UINT16 },
                             {} },
    .alerts = (Parameter[]){ { .name = "NoAlert", .type = TYPE_UINT8 },
                             { .name = "PowerEnable", .type = TYPE_UINT8 },
                             { .name = "PowerGood", .type = TYPE_UINT8 },
                             { .name = "DiconnectAlert", .type = TYPE_UINT8 },
                             { .name = "DetectionAlert", .type = TYPE_UINT8 },
                             { .name = "ClassAlert", .type = TYPE_UINT8 },
                             { .name = "TCUTAler", .type = TYPE_UINT8 },
                             { .name = "TStartAlert", .type = TYPE_UINT8 },
                             { .name = "SupplyAlert", .type = TYPE_UINT8 },
                             {} },
    .commands = (Command[]){ {
                                     .name = "reset",
                                     .cb_cmd = LTC4274_reset,
                             },
                             {} },
    .fxnTable = &LTC4274_fxnTable,
};

#endif /* _OCMP_LTC4274_H_ */
