/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Helper functions/wrappers for memory allocations, manipulation and
 * comparison.
 */

#ifndef VBOOT_REFERENCE_UTILITY_H_
#define VBOOT_REFERENCE_UTILITY_H_

#include "sysincludes.h"
#include "vboot_api.h"

/* Debug and error output */
#ifdef VBOOT_DEBUG
#define VBDEBUG(params) VbExDebug params
#else
#define VBDEBUG(params)
#endif

#ifdef VBOOT_DEBUG
#define VbAssert(expr) do { if (!(expr)) { \
    VbExError("assert fail: %s at %s:%d\n", \
              #expr, __FILE__, __LINE__); }} while(0)
#else
#define VbAssert(expr)
#endif

/* Optional, up to the BIOS */
#ifdef VBOOT_EASTER_EGG
#define VBEASTEREGG VbExEasterEgg()
#else
#define VBEASTEREGG 0
#endif

/*
 * Combine [msw] and [lsw] uint16s to a uint32_t with its [msw] and [lsw]
 * forming the most and least signficant 16-bit words.
 */
#define CombineUint16Pair(msw,lsw) (((uint32_t)(msw) << 16) |   \
                                    (((lsw)) & 0xFFFF))

/* Return the minimum of (a) or (b). */
#define Min(a, b) (((a) < (b)) ? (a) : (b))

/**
 * Compare [n] bytes in [src1] and [src2].
 *
 * Returns an integer less than, equal to, or greater than zero if the first
 * [n] bytes of [src1] is found, respectively, to be less than, to match, or be
 * greater than the first n bytes of [src2]. */
int Memcmp(const void *src1, const void *src2, size_t n);

/**
 * Copy [n] bytes from [src] to [dest].
 */
void *Memcpy(void *dest, const void *src, uint64_t n);

/*
 * Implementations of the functions below must be built as part of the firmware
 * and defined in lib/utility.c.
 */

/**
 * Set [n] bytes starting at [s] to [c].  Returns dest.
 */
void *Memset(void *dest, const uint8_t c, uint64_t n);

/**
 * Compare [n] bytes starting at [s1] with [s2] and return 0 if they
 * match, 1 if they don't.  Returns 0 if n=0, since no bytes mismatched.
 *
 * Time taken to perform the comparison is only dependent on [n] and
 * not on the relationship of the match between [s1] and [s2].
 *
 * Note that unlike Memcmp(), this only indicates inequality, not
 * whether s1 is less than or greater than s2.
 */
int SafeMemcmp(const void *s1, const void *s2, size_t n);

/*
 * Buffer size required to hold the longest possible output of Uint64ToString()
 * - that is, Uint64ToString(~0, 2).
 */
#define UINT64_TO_STRING_MAX 65

/**
 * Convert a value to a string in the specified radix (2=binary, 10=decimal,
 * 16=hex) and store it in <buf>, which is <bufsize> chars long.  If
 * <zero_pad_width>, left-pads the string to at least that width with '0'.
 * Returns the length of the stored string, not counting the terminating null.
 */
uint32_t Uint64ToString(char *buf, uint32_t bufsize, uint64_t value,
                        uint32_t radix, uint32_t zero_pad_width);

/**
 * Concatenate <src> onto <dest>, which has space for <destlen> characters
 * including the terminating null.  Note that <dest> will always be
 * null-terminated if <destlen> > 0.  Returns the number of characters used in
 * <dest>, not counting the terminating null.
 */
uint32_t StrnAppend(char *dest, const char *src, uint32_t destlen);

/* Ensure that only our stub implementations are used, not standard C */
#ifndef _STUB_IMPLEMENTATION_
#define malloc _do_not_use_standard_malloc
#define free _do_not_use_standard_free
#define memcmp _do_not_use_standard_memcmp
#define memcpy _do_not_use_standard_memcpy
#define memset _do_not_use_standard_memset
#endif

#endif  /* VBOOT_REFERENCE_UTILITY_H_ */
