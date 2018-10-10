/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <ti/sysbios/knl/Queue.h>
#include <stdbool.h>
#include "unity.h"

Queue_Handle postRxMsgQueue;
Queue_Handle bigBrotherTxMsgQueue;
xdc_Bool ti_sysbios_knl_Queue_empty__E( ti_sysbios_knl_Queue_Handle __inst )
{
    TEST_ASSERT(__inst);
    return true;
}

void ti_sysbios_knl_Queue_construct( ti_sysbios_knl_Queue_Struct *__obj, const
                              ti_sysbios_knl_Queue_Params *__prms )
{
    TEST_ASSERT(__obj);
    return;
}

xdc_Void ti_sysbios_knl_Queue_enqueue__E( ti_sysbios_knl_Queue_Handle __inst,
                                               ti_sysbios_knl_Queue_Elem *elem )
{
    TEST_ASSERT(__inst);
    return;
}

xdc_Ptr ti_sysbios_knl_Queue_dequeue__E( ti_sysbios_knl_Queue_Handle __inst )
{
    return ((xdc_Ptr)1);
}