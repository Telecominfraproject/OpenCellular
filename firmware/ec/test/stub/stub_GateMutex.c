/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "helpers/attribute.h"

#include <ti/sysbios/gates/GateMutex.h>

ti_sysbios_gates_GateMutex_Handle ti_sysbios_gates_GateMutex_create(
        const ti_sysbios_gates_GateMutex_Params *__paramsPtr,
        xdc_runtime_Error_Block *__eb)
{
    UNUSED(__paramsPtr);
    UNUSED(__eb);

    /* Doesn't matter, as long as it's not NULL */
    return (ti_sysbios_gates_GateMutex_Handle)1;
}
xdc_IArg
ti_sysbios_gates_GateMutex_enter__E(ti_sysbios_gates_GateMutex_Handle __inst)
{
    UNUSED(__inst);
    return 0;
}

xdc_Void
ti_sysbios_gates_GateMutex_leave__E(ti_sysbios_gates_GateMutex_Handle __inst,
                                    xdc_IArg key)
{
    UNUSED(__inst);
    UNUSED(key);
    return;
}
