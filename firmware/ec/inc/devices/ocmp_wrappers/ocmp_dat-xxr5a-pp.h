/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/
#ifndef _OCMP_DATXXR5APP_H
#define _OCMP_DATXXR5APP_H

#include "drivers/OcGpio.h"

#include <stdint.h>

#define DATR5APP_PIN_COUNT 6

typedef union DATR5APP_Config {
    int16_t attenuation; /* Attenuation in db (x2): 1.5 -> 3 */
} DATR5APP_Config;

typedef struct DATR5APP_Cfg {
    union {
        struct {
            OcGpio_Pin pin_p5db;
            OcGpio_Pin pin_1db;
            OcGpio_Pin pin_2db;
            OcGpio_Pin pin_4db;
            OcGpio_Pin pin_8db;
            OcGpio_Pin pin_16db; /* Optional */
        };
        OcGpio_Pin pin_group[DATR5APP_PIN_COUNT];
    };
    OcGpio_Pin pin_le;
} DATR5APP_Cfg;

extern const Driver DATXXR5APP;

#endif /* _OCMP_DATXXR5APP_H */
