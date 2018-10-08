/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "threaded_int.h"

#include "inc/common/global_header.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

// Threaded interrupt info
#define TI_TASKSTACKSIZE 1024
#define TI_TASKPRIORITY 6

// This number is fairly superficial - just used to keep track of the
// various tasks, it can be increased without much overhead
#define MAX_DEVICES 30

// Config simply to map context to our GPIO interrupts
typedef struct InterruptConfig {
    Semaphore_Handle sem; //!< Semaphore to wake up INT thread
    ThreadedInt_Callback cb; //!< Callback to run when interrupt occurs
    void *context; //!< Pointer to pass to cb function
} InterruptConfig;
static InterruptConfig s_intConfigs[MAX_DEVICES] = {};
static int s_numDevices = 0;

static void gpioIntFxn(const OcGpio_Pin *pin, void *context)
{
    Semaphore_Handle sem = context;

    // TODO: this should probably be an assert
    if (!sem) {
        return;
    }

    /* Just wake up the TI task */
    Semaphore_post(sem);
}

static void ThreadedInt_Task(UArg arg0, UArg arg1)
{
    InterruptConfig *cfg = (InterruptConfig *)arg0;
    if (!cfg) {
        DEBUG("Threaded Int started without configuration???\n");
        return;
    }

    DEBUG("Threaded INT thread ready\n");
    while (true) {
        Semaphore_pend(cfg->sem, BIOS_WAIT_FOREVER);
        cfg->cb(cfg->context);
    }
}

// TODO: this function isn't thread safe at the moment
void ThreadedInt_Init(OcGpio_Pin *irqPin, ThreadedInt_Callback cb,
                      void *context)
{
    // Build up table of all devices for interrupt handling. This is an ok
    // workaround for TI RTOS GPIO interrupts for now (only using one device)
    if (s_numDevices >= MAX_DEVICES) {
        DEBUG("ThrdInt::FATAL: too many configurations");
        return;
    }
    int devNum = s_numDevices++;

    Semaphore_Handle sem = Semaphore_create(0, NULL, NULL);
    if (!sem) {
        DEBUG("ThrdInt::Can't create ISR semaphore\n");
        return;
    }

    s_intConfigs[devNum] = (InterruptConfig){
        .sem = sem,
        .cb = cb,
        .context = context,
    };

    // Start interrupt handling task
    // One task per interrupt, not that efficient, but we don't have much
    // need to optimize into a thread pool
    // TODO: look into error block and see if I should use it
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TI_TASKSTACKSIZE;
    taskParams.priority = TI_TASKPRIORITY;
    taskParams.arg0 = (uintptr_t)&s_intConfigs[devNum];
    Task_Handle task = Task_create(ThreadedInt_Task, &taskParams, NULL);
    if (!task) {
        DEBUG("ThrdInt::FATAL: Unable to start interrupt task\n");
        Semaphore_delete(&sem);
        s_numDevices--;
        return;
    }
    // TODO: what do I do with task handle?

    /* Set up IRQ pin callback */
    OcGpio_setCallback(irqPin, gpioIntFxn, sem);
    OcGpio_enableInt(irqPin);
}
