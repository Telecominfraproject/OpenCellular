/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Helper functions/wrappers for memory allocations, manipulation and
 * comparison.
 */

#ifndef VBOOT_REFERENCE_UTILITY_H_
#define VBOOT_REFERENCE_UTILITY_H_

#include <inttypes.h>
#include <string.h>

/* Allocate [size] bytes and return a pointer to the allocated memory. Abort
 * on error.
 */
void* Malloc(size_t size);

/* Free memory pointed by [ptr] previously allocated by Malloc(). */
void Free(void* ptr);

/* Copy [n] bytes from [src] to [dest]. */
void* Memcpy(void* dest, const void* src, size_t n);

/* Set [n] bytes starting at [s] to [c]. */
void* Memset(void *dest, const uint8_t c, size_t n);

/* Compare [n] bytes starting at [s1] with [s2] and return 1 if they match,
 * 0 if they don't. Time taken to perform the comparison is only dependent on
 * [n] and not on the relationship of the match between [s1] and [s2].
 */
int SafeMemcmp(const void* s1, const void* s2, size_t n);

/* Track remaining data to be read in a buffer. */
typedef struct MemcpyState {
  void* remaining_buf;
  int remaining_len;
} MemcpyState;

/* Copy [len] bytes into [dst] only if there's enough data to read according
 * to [state].
 * On success, return [dst] and update [state]..
 * On failure, return NULL, set remaining len in state to -1.
 *
 * Useful for iterating through a binary blob to populate a struct. After the
 * first failure (buffer overrun), successive calls will always fail.
 */
void* StatefulMemcpy(MemcpyState* state, void* dst, int len);


#endif  /* VBOOT_REFERENCE_UTILITY_H_ */
