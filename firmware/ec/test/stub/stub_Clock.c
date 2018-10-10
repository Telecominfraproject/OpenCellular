/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include <stdbool.h>
#include <ti/sysbios/knl/clock.h>
#include "unity.h"
const CT__ti_sysbios_knl_Clock_tickPeriod ti_sysbios_knl_Clock_tickPeriod__C;

xdc_Void ti_sysbios_knl_Clock_stop__E( ti_sysbios_knl_Clock_Handle __inst )
{
    TEST_ASSERT(__inst);
    return;
}

void ti_sysbios_knl_Clock_construct( ti_sysbios_knl_Clock_Struct *__obj, ti_sysbios_knl_Clock_FuncPtr clockFxn,
                    xdc_UInt timeout, const ti_sysbios_knl_Clock_Params *__prms )
{
    TEST_ASSERT(__obj);
    return;
}

xdc_Void ti_sysbios_knl_Clock_start__E( ti_sysbios_knl_Clock_Handle __inst )
{
    TEST_ASSERT(__inst);
    return;
}

xdc_Bool ti_sysbios_knl_Clock_isActive__E( ti_sysbios_knl_Clock_Handle __inst)
{
    TEST_ASSERT(__inst);
    return true;
}

xdc_Void ti_sysbios_knl_Clock_setTimeout__E( ti_sysbios_knl_Clock_Handle __inst, xdc_UInt32 timeout )
{
    return;
}

xdc_Void ti_sysbios_knl_Clock_setPeriod__E( ti_sysbios_knl_Clock_Handle
                                                 __inst, xdc_UInt32 period )
{
    TEST_ASSERT(__inst);
    return;
}

xdc_Void ti_sysbios_knl_Clock_Params__init__S( xdc_Ptr dst, const xdc_Void
                                               *src, xdc_SizeT psz, xdc_SizeT isz )
{
    return;
}