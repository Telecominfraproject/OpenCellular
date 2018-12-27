/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "include/test_se98a.h"
#include <stdint.h>

OcGpio_Port gbc_io_0 = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};
SE98A_Dev s_invalid_device = {
    .cfg =
        {
            .dev =
                {
                    .bus = 3,
                    .slave_addr = 0xFF,
                },
        },
};

SE98A_Dev s_invalid_bus = {
    .cfg =
        {
            .dev =
                {
                    .bus = 0xFF,
                    .slave_addr = 0x1A,
                },
        },
};
bool SE98A_GpioPins[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};

uint32_t SE98A_GpioConfig[] = {
    [0x05] = OCGPIO_CFG_INPUT,
};

uint16_t SE98A_regs[] = {
    [SE98A_REG_CAPABILITY] = 0x00,     /* Capabilities */
    [SE98A_REG_CONFIG] = 0x00,         /* Config */
    [SE98A_REG_HIGH_LIMIT] = 0x00,     /* High limit */
    [SE98A_REG_LOW_LIMIT] = 0x00,      /* Low limit */
    [SE98A_REG_CRITICAL_LIMIT] = 0x00, /* Critical limit */
    [SE98A_REG_MEASURED_TEMP] = 0x00,  /* Measured Temperature */
    [SE98A_REG_MFG_ID] = 0x00,         /* MFG ID */
    [SE98A_REG_DEVICE_ID] = 0x00,      /* Device ID */
};
