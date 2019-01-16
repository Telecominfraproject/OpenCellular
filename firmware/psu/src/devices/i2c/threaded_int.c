/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "threaded_int.h"
#include "common/inc/global/ocmp_frame.h"
#include "inc/common/global_header.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

#include <xdc/runtime/Error.h>

// Threaded interrupt info
//#define TI_TASKSTACKSIZE        1024
#define TI_TASKSTACKSIZE        512
#define TI_TASKPRIORITY         6
#define EVENT_ALL               0xFFFFFFFF
#define EVENT(x)                ((uint32_t)(1 << x))

// This number is fairly superficial - just used to keep track of the
// various tasks, it can be increased without much overhead
#define MAX_DEVICES 30

// Config simply to map context to our GPIO interrupts
typedef struct InterruptConfig {
    Semaphore_Handle sem;       //!< Semaphore to wake up INT thread
    ThreadedInt_Callback cb;    //!< Callback to run when interrupt occurs
    void *context;              //!< Pointer to pass to cb function
} InterruptConfig;

static AlertConfig s_intConfigs[MAX_DEVICES] = {};
static int s_numDevices = 0;
Event_Handle alertEvent;

void ThreadedInt_set_event(const OcGpio_Pin *pin)
{
    int8_t count = 0;

    for(count = 0; count <= s_numDevices; count++) {
        /* Set up IRQ pin callback */
        if(s_intConfigs[count].irqPin == pin) {
            Event_post(alertEvent, EVENT(count));
        }
    }
}

void ThreadedInt_send_alert_message(int32_t result)
{
    uint8_t count = 0;
    uint8_t size = sizeof(AlertConfig);
    uint8_t *alertPtr = (uint8_t *)(&s_intConfigs[result]);
    OCMPMessageFrame *pMsg = create_ocmp_msg_frame(s_intConfigs[count].subSystem,
                                                      OCMP_MSG_TYPE_ALERT,
                                                       0x00, 0x00, 0x00, size);
    memcpy(pMsg->message.ocmp_data, (uint8_t *)(&s_intConfigs[result]), size);

    for(count = 0; count < size; count++) {
        pMsg->message.ocmp_data[count] = alertPtr[count];
    }
    SSRegistry_sendMessage(s_intConfigs[count].subSystem, pMsg);
}

static void gpioIntFxn(const OcGpio_Pin *pin, void *context) {
    ThreadedInt_set_event(pin);
}

static void ThreadedInt_Task(UArg arg0, UArg arg1)
{
    uint8_t count = 0;
    uint32_t result = 0;
    InterruptConfig *cfg = (InterruptConfig *)arg0;
    if (!cfg) {
        DEBUG("Threaded Int started without configuration???\n");
        return;
    }

    Error_Block errorBlock;
    Error_init(&errorBlock);

    alertEvent = Event_create(NULL, &errorBlock);
    if (alertEvent == NULL) {
        DEBUG("Event create failed");
    }

    for(int8_t count = 0; count <= s_numDevices; count++) {
        /* Set up IRQ pin callback */
        OcGpio_setCallback(s_intConfigs[count].irqPin, gpioIntFxn, NULL);
        OcGpio_enableInt(s_intConfigs[count].irqPin);
    }
    DEBUG("Threaded INT thread ready %d\n", count);

    while (true) {
        result = Event_pend(alertEvent, Event_Id_NONE, EVENT_ALL, BIOS_WAIT_FOREVER);
        for(count = 0; count <= s_numDevices; count++) {
            if(result & EVENT(count)) {
                ThreadedInt_send_alert_message(count);
            }
        }
    }
}

void ThreadedInt_createtask()
{
    Task_Params taskParams;
    Task_Params_init(&taskParams);
    taskParams.stackSize = TI_TASKSTACKSIZE;
    taskParams.priority = TI_TASKPRIORITY;
    taskParams.arg0 = (uintptr_t)&s_intConfigs[s_numDevices];
    Task_Handle task = Task_create(ThreadedInt_Task, &taskParams, NULL);

    return;
}
// TODO: this function isn't thread safe at the moment
void ThreadedInt_Init(pinConfig *pinCfg, ThreadedInt_Callback cb,
                      void *context) {

    // Build up table of all devices for interrupt handling. This is an ok
    // workaround for TI RTOS GPIO interrupts for now (only using one device)
    if (s_numDevices >= MAX_DEVICES) {
        DEBUG("ThrdInt::FATAL: too many configurations");
        return;
    }
    int devNum = s_numDevices++;

    s_intConfigs[devNum] = (AlertConfig) {
        .subSystem = pinCfg->subSystem,
        .irqPin = pinCfg->alertPin,
        .cb = cb,
        .context = context,
    };
}
