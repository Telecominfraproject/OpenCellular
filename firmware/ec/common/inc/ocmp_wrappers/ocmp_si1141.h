/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef COMMON_INC_OCMP_WRAPPERS_OCMP_SI1141_H_
#define COMMON_INC_OCMP_WRAPPERS_OCMP_SI1141_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable SI1141_fxnTable;

static const Driver SI1141 = {
    .name = "SI1141",
    .status = (Parameter[]){
        { .name = "partId", .type = TYPE_UINT16 },
        { .name = "revId", .type = TYPE_UINT8 },
        { .name = "seqId", .type = TYPE_UINT16 },
        {}
    },
    .config = (Parameter[]){
    },
    .alerts = (Parameter[]){
    },
    .fxnTable = &SI1141_fxnTable,
};

#endif /* COMMON_INC_OCMP_WRAPPERS_OCMP_SI1141_H_ */
