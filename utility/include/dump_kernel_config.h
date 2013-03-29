/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports the kernel commandline from a given partition/image file.
 */

#ifndef DUMP_KERNEL_CONFIG_UTILITY_H_
#define DUMP_KERNEL_CONFIG_UTILITY_H_

#include <inttypes.h>
#include <stdlib.h>

/* TODO(wfrichar): This needs a better location */
#define MAX_KERNEL_CONFIG_SIZE     4096

/* Use this to obtain the body load address from the kernel preamble */
#define USE_PREAMBLE_LOAD_ADDR     (~0)

/* Returns a new copy of the kernel cmdline. The caller must free it. */
char *FindKernelConfig(const char *filename,
                       uint64_t kernel_body_load_address);

#endif  // DUMP_KERNEL_CONFIG_UTILITY_H_
