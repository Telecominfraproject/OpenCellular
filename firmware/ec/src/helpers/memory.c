/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#include "memory.h"

#include "inc/common/global_header.h"

void printMemory(const void *start, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        DEBUG("0x%02x ", ((uint8_t *)start)[i]);

        if ((i + 1) % 8 == 0) {
            DEBUG("\n");
        } else if ((i + 1) % 4 == 0) {
            DEBUG(" ");
        }
    }
    DEBUG("\n");
}

uint8_t set_bit8(uint8_t byte, uint8_t bit, bool value)
{
    const uint8_t mask = 1 << bit;
    if (value) {
        byte |= mask;
    } else {
        byte &= ~mask;
    }
    return byte;
}
