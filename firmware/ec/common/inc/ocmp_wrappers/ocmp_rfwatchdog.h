/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_RFWATCHDOG_H_
#define OCMP_RFWATCHDOG_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable RFFEWatchdogP_fxnTable;

static const Driver RFFEWatchdog = {
    .name = "RFFE Watchdog",
    .alerts =
            (Parameter[]){ { .name = "LB_R_PWR" }, { .name = "HB_R_PWR" }, {} },
    .fxnTable = &RFFEWatchdogP_fxnTable,
};

#endif /* OCMP_RFWATCHDOG_H_ */
