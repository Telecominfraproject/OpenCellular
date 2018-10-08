/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef POST_H_
#define POST_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "common/inc/global/ocmp_frame.h"
#include "common/inc/global/post_frame.h"
#include "inc/common/global_header.h"

#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define POST_RECORDS 55

#define OC_POST_TASKPRIORITY 3
#define POST_TASK_STACK_SIZE 4096

/*****************************************************************************
 *                            HANDLE DECLARATIONS
 *****************************************************************************/
extern Semaphore_Handle semPOSTMsg;
extern Queue_Handle postRxMsgQueue;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
void post_createtask(void);
#endif /* POST_H_ */
