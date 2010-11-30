/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * ARM firmware platform-specific definitions
 */

#ifndef __ARCH_ARM_BIOSINCLUDES_H__
#define __ARCH_ARM_BIOSINCLUDES_H__

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef unsigned int size_t;

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define UINT64_C(x) ((uint64_t) x)
#define PRIu64 "%ll"
extern void debug(const char *format, ...);

#define POSSIBLY_UNUSED __attribute__((unused))

#ifdef __STRICT_ANSI__
#define INLINE
#else
#define INLINE inline
#endif

#define UINT64_RSHIFT(v, shiftby) (((uint64_t)(v)) >> (shiftby))
#define UINT64_MULT32(v, multby)  (((uint64_t)(v)) * ((uint32_t)(multby)))

#ifndef UINT64_MAX
#define UINT64_MAX (UINT64_C(0xffffffffffffffffULL))
#endif

#endif /*__ARCH_ARM_BIOSINCLUDES_H__ */
