/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <ti/sysbios/knl/clock.h>
#include "unity.h"

xdc_Void ti_sysbios_knl_Clock_stop__E( ti_sysbios_knl_Clock_Handle __inst )
{
    TEST_ASSERT(__inst);
    return;
}

