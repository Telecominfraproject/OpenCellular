/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef BIGBROTHER_H_
#define BIGBROTHER_H_

/*****************************************************************************
 *                               HEADER FILES
 *****************************************************************************/
#include "inc/common/global_header.h"
#include "inc/utils/util.h"

/*****************************************************************************
 *                             MACRO DEFINITIONS
 *****************************************************************************/
#define BIGBROTHER_TASK_PRIORITY 5
#define BIGBROTHER_TASK_STACK_SIZE 8096

typedef enum {
    OC_SYS_S_ID_EEPROM = 1,
    OC_SYS_INVEN_EEPROM = 2,
    OC_SYS_FLASH = 3,
} eSysDeviceSno;

/* Semaphore and Queue Handles for Big Brother */
extern Semaphore_Handle semBigBrotherMsg;
extern Queue_Handle bigBrotherRxMsgQueue;
extern Queue_Handle bigBrotherTxMsgQueue;

/*****************************************************************************
 *                           FUNCTION DECLARATIONS
 *****************************************************************************/
void bigbrother_createtask(void);

#endif /* BIGBROTHER_H_ */
