/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
//*****************************************************************************
//                                HEADER FILES
//*****************************************************************************
#include "inc/subsystem/bms/bms.h"

#include "registry/SSRegistry.h"

/* Global Task Configuration Variables */
static Char bmsTaskStack[BMS_TASK_STACK_SIZE];

OCSubsystem ssBms = {
    .taskStackSize = BMS_TASK_STACK_SIZE,
    .taskPriority = BMS_TASK_PRIORITY,
    .taskStack = bmsTaskStack,
};
