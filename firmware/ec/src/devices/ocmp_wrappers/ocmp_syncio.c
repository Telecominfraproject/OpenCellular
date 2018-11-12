/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "common/inc/ocmp_wrappers/ocmp_syncio.h"

#include "common/inc/global/Framework.h"
#include "inc/subsystem/sync/sync.h"

const Driver_fxnTable SYNC_fxnTable = {
    /* Message handlers */
    .cb_get_status = SYNC_GpsStatus,
};
