/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef COMMON_INC_OCMP_WRAPPERS_OCMP_LTC4295_H_
#define COMMON_INC_OCMP_WRAPPERS_OCMP_LTC4295_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable LTC4295_fxnTable;

static const Driver LTC4295 = {
    .name = "LTC4295",
    .status = (Parameter[]){
        { .name = "class", .type = TYPE_ENUM },
        { .name = "powerGoodState", .type = TYPE_ENUM },
        {}
    },
    .alerts = (Parameter[]){
        { .name = "INCOMPATIBLE", .type = TYPE_ENUM },
        { .name = "DISCONNECT", .type = TYPE_ENUM },
        { .name = "CONNECT", .type = TYPE_ENUM },
        {}
    },
    .fxnTable = &LTC4295_fxnTable,
};

#endif /* COMMON_INC_OCMP_WRAPPERS_OCMP_LTC4295_H_ */
