/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "inc/common/global_header.h"
#include "registry/SSRegistry.h"

#include <ti/sysbios/BIOS.h>

#define DEBUG_TASK_PRIORITY     2
#define DEBUG_TASK_STACK_SIZE  1024

/* Global Task Configuration Variables */
static Char debug_task_stack[DEBUG_TASK_STACK_SIZE];

extern void *sys_config[];
#define DBG ((Dbg_Cfg *)sys_config[OC_SS_DEBUG])

OCSubsystem ssDbg = {
    .taskStackSize = DEBUG_TASK_STACK_SIZE,
    .taskPriority = DEBUG_TASK_PRIORITY,
    .taskStack = debug_task_stack,
};
