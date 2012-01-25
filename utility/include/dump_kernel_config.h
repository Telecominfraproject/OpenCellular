/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports the kernel commandline from a given partition/image.
 */

#ifndef DUMP_KERNEL_CONFIG_UTILITY_H_
#define DUMP_KERNEL_CONFIG_UTILITY_H_

#include <inttypes.h>
#include <stdlib.h>

uint8_t* find_kernel_config(uint8_t* blob, uint64_t blob_size,
                            uint64_t kernel_body_load_address);

void* MapFile(const char* filename, size_t *size);

#endif  // DUMP_KERNEL_CONFIG_UTILITY_H_
