/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <ti/sysbios/knl/Task.h>
#include <stdbool.h>
#include "unity.h"
uint8_t taskCreated = false;
uint8_t taskInit = false;

void ti_sysbios_knl_Task_construct( ti_sysbios_knl_Task_Struct *__obj, ti_sysbios_knl_Task_FuncPtr fxn,
                        const ti_sysbios_knl_Task_Params *__prms, xdc_runtime_Error_Block *__eb )
{
    TEST_ASSERT(__obj);
    TEST_ASSERT(fxn);
    TEST_ASSERT(__prms);
    TEST_ASSERT_TRUE((__prms->stackSize > 0));
    TEST_ASSERT_TRUE((__prms->priority > 0));
    TEST_ASSERT_TRUE((__prms->stack));
    TEST_ASSERT_TRUE(taskInit);

    /* check for this in the test suite to indicate task is created */
    taskCreated = true;

    /* Reset */
    taskInit = false;
    return;
}

xdc_Void ti_sysbios_knl_Task_Params__init__S( xdc_Ptr dst, const xdc_Void *src, xdc_SizeT psz, xdc_SizeT isz )
{
    TEST_ASSERT(dst);
    /* Init is done now task can be created */
    taskInit = true;
    return;
}
