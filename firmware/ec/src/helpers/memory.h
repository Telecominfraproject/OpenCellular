/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once

#ifndef HELPERS_MEMORY_H_
#define HELPERS_MEMORY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef HIWORD
#define HIWORD(x) ((uint16_t)((x) >> 16))
#endif

#ifndef LOWORD
#define LOWORD(x) ((uint16_t)(x))
#endif

#ifndef HIBYTE
#define HIBYTE(x) ((uint8_t)((x) >> 8))
#endif

#ifndef LOBYTE
#define LOBYTE(x) ((uint8_t)(x))
#endif

#define zalloc(size) calloc((size), 1)

void printMemory(const void *start, size_t size);

/* Sets a bit in a uint8 to the specified value (1/0)
 * @param byte The original value we're modifying
 * @param bit The index of the bit we want to set/clear
 * @param value The value the bit should be set to (1/0)
 * @return The modified byte with the bit's new value
 */
uint8_t set_bit8(uint8_t byte, uint8_t bit, bool value);

#endif /* HELPERS_MEMORY_H_ */
