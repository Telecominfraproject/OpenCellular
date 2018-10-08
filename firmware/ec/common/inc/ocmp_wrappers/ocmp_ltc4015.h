/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_LTC4015_H
#define _OCMP_LTC4015_H

#include "common/inc/global/Framework.h"

typedef struct LTC4015_Config {
    int16_t batteryVoltageLow;
    int16_t batteryVoltageHigh;
    int16_t batteryCurrentLow;
    int16_t inputVoltageLow;
    int16_t inputCurrentHigh;
    uint16_t inputCurrentLimit;
    uint16_t icharge;
    uint16_t vcharge;
} LTC4015_Config;

SCHEMA_IMPORT const Driver_fxnTable LTC4015_fxnTable;

static const Driver LTC4015 = {
    .name = "LTC4015",
    .status = (Parameter[]){ { .name = "batteryVoltage", .type = TYPE_INT16 },
                             { .name = "batteryCurrent", .type = TYPE_INT16 },
                             { .name = "systemVoltage", .type = TYPE_INT16 },
                             { .name = "inputVoltage", .type = TYPE_INT16 },
                             { .name = "inputCurrent", .type = TYPE_INT16 },
                             { .name = "dieTemperature", .type = TYPE_INT16 },
                             { .name = "ichargeDAC", .type = TYPE_INT16 },
                             {} },
    .config =
            (Parameter[]){ { .name = "batteryVoltageLow", .type = TYPE_INT16 },
                           { .name = "batteryVoltageHigh", .type = TYPE_INT16 },
                           { .name = "batteryCurrentLow", .type = TYPE_INT16 },
                           { .name = "inputVoltageLow", .type = TYPE_INT16 },
                           { .name = "inputCurrentHigh", .type = TYPE_INT16 },
                           { .name = "inputCurrentLimit", .type = TYPE_UINT16 },
                           { .name = "icharge", .type = TYPE_UINT16 },
                           { .name = "vcharge", .type = TYPE_UINT16 },
                           { .name = "dieTemperature", .type = TYPE_INT16 },
                           {} },
    .alerts = (Parameter[]){ { .name = "BVL", .type = TYPE_INT16 },
                             { .name = "BVH", .type = TYPE_INT16 },
                             { .name = "BCL", .type = TYPE_INT16 },
                             { .name = "IVL", .type = TYPE_INT16 },
                             { .name = "ICH", .type = TYPE_INT16 },
                             { .name = "DTH", .type = TYPE_INT16 },
                             {} },
    .fxnTable = &LTC4015_fxnTable,
};

#endif /* _OCMP_LTC4015_H */
