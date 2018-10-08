/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef GOSSIPER_H_
#define GOSSIPER_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/utils/util.h"

/*****************************************************************************
 *                              MACRO DEFINITIONS
 *****************************************************************************/
#define GOSSIPER_TASK_PRIORITY 6
#define GOSSIPER_TASK_STACK_SIZE 2048

#define SET_DEBEUG_MODE(debugMode) ((debugMode | 0x00))
#define UNSET_DEBUG_MODE(debugMode) ((debugMode & 0x0f))

/*****************************************************************************
 *                             HANDLE DEFINITIONS
 *****************************************************************************/
/* Semaphore and Queue Handles for Gossiper */
extern Semaphore_Handle semGossiperMsg;
extern Queue_Handle gossiperTxMsgQueue;
extern Queue_Handle gossiperRxMsgQueue;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
void gossiper_createtask(void);

#endif /* GOSSIPER_H_ */
