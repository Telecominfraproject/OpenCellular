/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef OCMP_RFPOWERMONITOR_H_
#define OCMP_RFPOWERMONITOR_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable RFPowerMonitor_fxnTable;

static const Driver RFPowerMonitor = {
    .status = (Parameter[]){ { .name = "forward", .type = TYPE_UINT16 },
                             { .name = "reverse", .type = TYPE_UINT16 },
                             {} },
    .fxnTable = &RFPowerMonitor_fxnTable,
};

#endif /* OCMP_RFPOWERMONITOR_H_ */
