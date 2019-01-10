/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

#ifndef COMMON_INC_OCMP_WRAPPERS_OCMP_MP2951_H_
#define COMMON_INC_OCMP_WRAPPERS_OCMP_MP2951_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable MP2951_fxnTable;

static const Driver MP2951 = {
    .name = "MP2951",
    .status = (Parameter[]){
        { .name = "vendorId", .type = TYPE_UINT8 },
        { .name = "productId", .type = TYPE_UINT8 },
        {}
    },
    .config = (Parameter[]){
		{ .name = "inputVoltageOnLimit", .type = TYPE_UINT8 },
        { .name = "inputVoltageOffLimit", .type = TYPE_UINT8 },
    },
    .alerts = (Parameter[]){
    },
    .fxnTable = &MP2951_fxnTable,
};

#endif /* COMMON_INC_OCMP_WRAPPERS_OCMP_MP2951_H_ */
