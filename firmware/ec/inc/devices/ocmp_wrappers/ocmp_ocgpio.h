/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#ifndef OCMP_OCGPIO_H_
#define OCMP_OCGPIO_H_
#include "src/drivers/OcGpio.h"

typedef struct __attribute__ ((packed, aligned(1))) {
    uint8_t    pin;
    uint8_t    value;
}S_OCGPIO;

typedef struct S_OCGPIO_Cfg {
    OcGpio_Port* port;
    unsigned int group;
}S_OCGPIO_Cfg;
extern const Driver OC_GPIO;

#endif /* OCMP_OCGPIO_H_ */
