/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef _SSREGISTRY_H_
#define _SSREGISTRY_H_

#include "common/inc/global/ocmp_frame.h"
#include "inc/common/system_states.h"

#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

#include <stdbool.h>

typedef void (*SS_ProcessMsg_Cb)(OCMPMessageFrame *pBmsMsg);
/**
 * Common subsystem attributes (message queue, task entry point, etc.)
 */
typedef struct OCSubsystem {
    /* Message queue handles */
    Queue_Handle msgQueue;
    Semaphore_Handle sem;

    /* Private variables (reduce dynamic allocation needs) */
    Queue_Struct queueStruct;
    Semaphore_Struct semStruct;
    Task_Struct taskStruct;
    eSubSystemStates state;
} OCSubsystem;

/**
 * Initializes the subsystem registry by creating message queues
 * and spawning the tasks for each subsystem
 */
void SSRegistry_init(void);

/**
 * Retrieves the pointer to the #OCSubsystem struct associated with a given ID
 * @param ss_id The ID of the requested subsystem
 * @return #OCSubsystem pointer if valid ID, else NULL
 */
OCSubsystem *SSRegistry_Get(OCMPSubsystem ss_id);

/**
 * Enters a message into the desired subsystem's message queue
 * @note This is mostly for legacy support for the alerts manager
 * @param ss_id ID of the subsystem to send the message to
 * @param pMsg Message pointer to be inserted into the subsystem's queue
 * @return true if successful, false if subsystem not found or queue full
 */
bool SSRegistry_sendMessage(OCMPSubsystem ss_id, void *pMsg);

#endif /* _SSREGISTRY_H_ */
