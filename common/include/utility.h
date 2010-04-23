/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Helper functions/wrappers for memory allocations, manipulation and
 * comparison.
 */

#ifndef VBOOT_REFERENCE_UTILITY_H_
#define VBOOT_REFERENCE_UTILITY_H_

#include <stdint.h>
#include <string.h>

/* Outputs an error message and quits. */
void error(const char *format, ...);

/* Outputs debug/warning messages. */
void debug(const char *format, ...);


#define assert(expr) do { if (!(expr)) { \
      error("assert fail: %s at %s:%d\n", \
            #expr, __FILE__, __LINE__); }} while(0)

/* Combine [msw] and [lsw] uint16s to a uint32_t with its [msw] and
 * [lsw] forming the most and least signficant 16-bit words.
 */
#define CombineUint16Pair(msw,lsw) (((msw) << 16) |     \
                                    (((lsw)) & 0xFFFF))
/* Return the minimum of (a) or (b). */
#define Min(a, b) (((a) < (b)) ? (a) : (b))

/* Allocate [size] bytes and return a pointer to the allocated memory. Abort
 * on error.
 */
void* Malloc(size_t size);

/* Free memory pointed by [ptr] previously allocated by Malloc(). */
void Free(void* ptr);

/* Compare [n] bytes in [src1] and [src2]
 * Returns an integer less than, equal to, or greater than zero if the first [n]
 * bytes of [src1] is found, respectively, to be less than, to match, or be
 * greater than the first n bytes of [src2]. */
int Memcmp(const void* src1, const void* src2, size_t n);

/* Copy [n] bytes from [src] to [dest]. */
void* Memcpy(void* dest, const void* src, size_t n);

/* Set [n] bytes starting at [s] to [c]. */
void* Memset(void *dest, const uint8_t c, size_t n);

/* Compare [n] bytes starting at [s1] with [s2] and return 0 if they match,
 * 1 if they don't. Time taken to perform the comparison is only dependent on
 * [n] and not on the relationship of the match between [s1] and [s2].
 */
int SafeMemcmp(const void* s1, const void* s2, size_t n);

/* Track remaining data to be read in a buffer. */
typedef struct MemcpyState {
  void* remaining_buf;
  uint64_t remaining_len;  /* Remaining length of the buffer. */
  uint8_t overrun;  /* Flag set to 1 when an overrun occurs. */
} MemcpyState;

/* Copy [len] bytes into [dst] only if there's enough data to read according
 * to [state].
 * On success, return [dst] and update [state].
 * On failure, return NULL, set remaining len in state to -1.
 *
 * Useful for iterating through a binary blob to populate a struct. After the
 * first failure (buffer overrun), successive calls will always fail.
 */
void* StatefulMemcpy(MemcpyState* state, void* dst, uint64_t len);

/* Like StatefulMemcpy() but copies in the opposite direction, populating
 * data from [src] into the buffer encapsulated in state [state].
 * On success, return [src] and update [state].
 * On failure, return NULL, set remaining_len in state to -1.
 *
 * Useful for iterating through a structure to populate a binary blob. After the
 * first failure (buffer overrun), successive calls will always fail.
 */
const void* StatefulMemcpy_r(MemcpyState* state, const void* src, uint64_t len);

#endif  /* VBOOT_REFERENCE_UTILITY_H_ */
