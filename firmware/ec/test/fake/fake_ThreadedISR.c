/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "fake_ThreadedISR.h"

#include "devices/i2c/threaded_int.h"
#include "helpers/attribute.h"

/* We'll statically allocate a circular queue so we can be lazy and not worry
 * about cleanup (we shouldn't have more than one anyway) */
#define NUM_ISR 5

static unsigned int s_isr_count;

typedef struct ISR_Data {
    ThreadedInt_Callback cb;
    void *ctx;
} ISR_Data;
static ISR_Data s_isr_data[NUM_ISR];

static void gpioIntFxn(const OcGpio_Pin *pin, void *context)
{
    UNUSED(pin);
    ISR_Data *isr_data = context;

    if (!isr_data) {
        return;
    }

    if (isr_data->cb) {
        isr_data->cb(isr_data->ctx);
    }
}

void ThreadedInt_Init(OcGpio_Pin *irqPin, ThreadedInt_Callback cb,
                      void *context)
{
    UNUSED(irqPin);

    s_isr_data[s_isr_count].cb = cb;
    s_isr_data[s_isr_count].ctx = context;

    OcGpio_setCallback(irqPin, gpioIntFxn, &s_isr_data[s_isr_count]);
    OcGpio_enableInt(irqPin);

    s_isr_count = (s_isr_count + 1) % NUM_ISR;
}
