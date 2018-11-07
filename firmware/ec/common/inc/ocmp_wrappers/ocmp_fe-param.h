/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_FE_PARAM_H_
#define OCMP_FE_PARAM_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable FE_PARAM_fxnTable;

static const Driver FE_Param = {
    .name = "FE_parametrs",
    .config = (Parameter[]){ { .name = "band", .type = TYPE_UINT16 },
                             { .name = "arfcn", .type = TYPE_UINT16 },
                             {} },
    .fxnTable = &FE_PARAM_fxnTable,
};

#endif /* OCMP_FE_PARAM_H_ */
