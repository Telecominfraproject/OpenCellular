/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Helper functions/wrappers for memory allocations, manipulation and
 * comparison.
 */

#ifndef VBOOT_REFERENCE_UTILITY_H_
#define VBOOT_REFERENCE_UTILITY_H_

#include <string.h>

/* Allocate [size] bytes and return a pointer to the allocated memory. Abort
 * on error.
 */
void* Malloc(size_t size);

/* Free memory pointed by [ptr] previously allocated by Malloc(). */
void Free(void* ptr);

/* Copy [n] bytes from [src] to [dest]. */
void* Memcpy(void* dest, const void* src, size_t n);

/* Compare [n] bytes starting at [s1] with [s2] and return 1 if they match,
 * 0 if they don't. Time taken to perform the comparison is only dependent on
 * [n] and not on the relationship of the match between [s1] and [s2].
 */
int SafeMemcmp(const void* s1, const void* s2, size_t n);

#endif  /* VBOOT_REFERENCE_UTILITY_H_ */
