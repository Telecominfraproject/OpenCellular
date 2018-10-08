/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_SYNCIO_H_
#define OCMP_SYNCIO_H_

#include "common/inc/global/Framework.h"

SCHEMA_IMPORT const Driver_fxnTable SYNC_fxnTable;
static const Driver Sync_IO = {
    .name = "sync_ioexp",
    .status =
            (Parameter[]){
                    {
                            .name = "gps_lock",
                            .type = TYPE_ENUM,
                            .values = (Enum_Map[]){ { 0, "Gps Not Locked" },
                                                    { 1, "Gps Locked" },
                                                    {} },
                    },
                    {} },
    .fxnTable = &SYNC_fxnTable,
};

#endif /* OCMP_SYNCIO_H_ */
