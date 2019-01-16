/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "include/test_ltc4275.h"

uint8_t LTC4275_GpioPins[] = {
    [LTC4275_PWR_PD_NT2P] = 0x00,    /* OC_EC_PWR_PD_NT2P = 64*/
    [LTC4275_PD_PWRGD_ALERT] = 0x00, /* OC_EC_PD_PWRGD_ALERT = 96 */
};

uint32_t LTC4275_GpioConfig[] = {
    [LTC4275_PWR_PD_NT2P] = OCGPIO_CFG_INPUT,
    [LTC4275_PD_PWRGD_ALERT] = OCGPIO_CFG_INPUT,
};

OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

LTC4275_Dev *ltc4275_invalid_cfg = NULL;