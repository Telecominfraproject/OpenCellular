/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_DATXXR5APP_H
#define _OCMP_DATXXR5APP_H

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable DATXXR5APP_fxnTable;

static const Driver DATXXR5APP = {
    .name = "DAT-XXR5A-PP+",
    .status = NULL,
    .config = (Parameter[]){ { .name = "atten", .type = TYPE_INT16 }, {} },
    .alerts = NULL,
    .fxnTable = &DATXXR5APP_fxnTable,
};

#endif /* _OCMP_DATXXR5APP_H */
