/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <ti/sysbios/knl/Semaphore.h>
#include "unity.h"

xdc_Void ti_sysbios_knl_Semaphore_post__E( ti_sysbios_knl_Semaphore_Handle __inst )
{
    return;
}

ti_sysbios_knl_Semaphore_Handle ti_sysbios_knl_Semaphore_create( xdc_Int count,
                                const ti_sysbios_knl_Semaphore_Params *__prms,
                                xdc_runtime_Error_Block *__eb )
{
    return(ti_sysbios_knl_Semaphore_Handle)1;
}

xdc_Bool ti_sysbios_knl_Semaphore_pend__E( ti_sysbios_knl_Semaphore_Handle __inst, xdc_UInt32 timeout )
{
    return(1);
}
