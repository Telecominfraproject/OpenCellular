/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

/* Define the memory block start/length for the OC_CONNECT1 M4 */

MEMORY
{
    FLASH (RX) : origin = 0x00000000, length = 0x00100000
    SRAM (RWX) : origin = 0x20000000, length = 0x00040000
}

/* Section allocation in memory */

SECTIONS
{
    .text   :   > FLASH
    .const  :   > FLASH
    .cinit  :   > FLASH
    .pinit  :   > FLASH
    .init_array : > FLASH

    .data   :   > SRAM
    .bss    :   > SRAM
    .sysmem :   > SRAM
    .stack  :   > SRAM
}
