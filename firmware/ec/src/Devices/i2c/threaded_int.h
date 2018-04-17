/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once

#ifndef DEVICES_I2C_THREADED_INT_H_
#define DEVICES_I2C_THREADED_INT_H_

#include "drivers/OcGpio.h"

typedef void (*ThreadedInt_Callback)(void *context);

void ThreadedInt_Init(OcGpio_Pin *irqPin, ThreadedInt_Callback cb,
                      void *context);

#endif /* DEVICES_I2C_THREADED_INT_H_ */
