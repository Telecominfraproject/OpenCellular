/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "common/inc/ocmp_wrappers/ocmp_slb9645.h"
#include "common/inc/global/Framework.h"
#include "helpers/array.h"
#include "helpers/math.h"
#include "inc/devices/slb9645.h"

static ePostCode _probe(void *driver, POSTData *postData)
{
    return slb9645_probe(driver,postData);
}

const Driver_fxnTable SLB9645_fxnTable = {
    /* Message handlers */
    .cb_probe = _probe,
};
