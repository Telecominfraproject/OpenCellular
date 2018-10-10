/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree. An additional grant
* of patent rights can be found in the PATENTS file in the same directory.
*/

static OcGpio_Port ec_io = {
    .fn_table = &FakeGpio_fnTable,
    .object_data = &(FakeGpio_Obj){},
};

static bool gpp_GpioPins[] = {
    [1] = 0x1, /* Pin = 1 */
    [115] = 0x1,
};

static uint32_t gpp_GpioConfig[] = {
    [1] = OCGPIO_CFG_INPUT,
    [115] = OCGPIO_CFG_INPUT,
};