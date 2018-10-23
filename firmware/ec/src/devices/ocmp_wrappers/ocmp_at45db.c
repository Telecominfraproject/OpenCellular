/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*
*/

#include "common/inc/global/Framework.h"
#include "common/inc/ocmp_wrappers/ocmp_at45db.h"
#include "inc/devices/at45db.h"

static ePostCode _probe(void *driver, POSTData *postData)
{
    return at45db_probe(driver,postData);
}

const Driver_fxnTable AT45DB641E_fxnTable = {
    /* Message handlers */
        .cb_probe = _probe,
};
