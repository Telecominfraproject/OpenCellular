/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host-side misc functions for verified boot.
 */

#ifndef VBOOT_REFERENCE_HOST_MISC_H_
#define VBOOT_REFERENCE_HOST_MISC_H_

#include "utility.h"
#include "vboot_struct.h"


/* Read data from [filename].  Store the size of returned data in [size].
 *
 * Returns the data buffer, which the caller must Free(), or NULL if
 * error. */
uint8_t* ReadFile(const char* filename, uint64_t* size);


/* Writes [size] bytes of [data] to [filename].
 *
 * Returns 0 if success, 1 if error. */
int WriteFile(const char* filename, const void *data, uint64_t size);


#endif  /* VBOOT_REFERENCE_HOST_MISC_H_ */
