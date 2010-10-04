/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Helper functions/wrappers for memory allocations, manipulation and
 * comparison.
 */

#ifndef VBOOT_REFERENCE_UTILITY_H_
#define VBOOT_REFERENCE_UTILITY_H_

#include "sysincludes.h"

/* Debug and error output */
#ifdef VBOOT_DEBUG
#define VBDEBUG(params) debug params
#else
#define VBDEBUG(params)
#endif

#ifndef VBOOT_PERFORMANCE
/* Define performance macros as nothing.  If you enable VBOOT_PERFORMANCE,
 * you must define these macros in your platform's biosincludes.h.
 *
 * Intended usage for using a performance counter called 'foo':
 *
 * VBPERFSTART("foo")
 * ...code to be tested...
 * VBPERFEND("foo")
 *
 * Names should be <= 8 characters to be compatible with all platforms.
 */
#define VBPERFSTART(name)
#define VBPERFEND(name)
#endif

/* Outputs an error message and quits. */
void error(const char* format, ...);

/* Outputs debug/warning messages. */
void debug(const char* format, ...);

#ifdef VBOOT_DEBUG
#define assert(expr) do { if (!(expr)) { \
      error("assert fail: %s at %s:%d\n", \
            #expr, __FILE__, __LINE__); }} while(0)
#else
#define assert(expr)
#endif

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
void* Memcpy(void* dest, const void* src, uint64_t n);


/* Implementations of the functions below must be built as part of the firmware
 * and defined in lib/utility.c */

/* Set [n] bytes starting at [s] to [c]. */
void* Memset(void *dest, const uint8_t c, uint64_t n);

/* Compare [n] bytes starting at [s1] with [s2] and return 0 if they match,
 * 1 if they don't. Time taken to perform the comparison is only dependent on
 * [n] and not on the relationship of the match between [s1] and [s2].
 */
int SafeMemcmp(const void* s1, const void* s2, size_t n);

/* Ensure that only our stub implementations are used, not standard C */
#ifndef _STUB_IMPLEMENTATION_
#define malloc _do_not_use_standard_malloc
#define free _do_not_use_standard_free
#define memcmp _do_not_use_standard_memcmp
#define memcpy _do_not_use_standard_memcpy
#define memset _do_not_use_standard_memset
#endif

#endif  /* VBOOT_REFERENCE_UTILITY_H_ */
