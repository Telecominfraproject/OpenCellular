/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef FAKE_GPIO_H_
#define FAKE_GPIO_H_

#include "src/drivers/OcGpio.h"

#define FAKE_GPIO_PIN_COUNT 32

typedef struct FakeGpio_Obj {
    struct {
        OcGpio_CallbackFn fn;
        void *context;
    } callback[FAKE_GPIO_PIN_COUNT];
} FakeGpio_Obj;

extern const OcGpio_FnTable FakeGpio_fnTable;

void FakeGpio_triggerInterrupt(const OcGpio_Pin *pin);
void FakeGpio_registerDevSimple(void *GpioPins, void *GpioConfig);

#endif /* FAKE_GPIO_H_ */
