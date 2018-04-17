/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#ifndef _GPIONATIVE_H_
#define _GPIONATIVE_H_

#include "OcGpio.h"

extern const OcGpio_FnTable GpioNative_fnTable;

/*
 * Must be called before using the GPIO Native driver
 */
void GpioNative_init(void);

#endif /* _GPIONATIVE_H_ */
