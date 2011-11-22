/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * X86 firmware platform-specific definitions
 */

#ifndef __ARCH_X86_BIOSINCLUDES_H__
#define __ARCH_X86_BIOSINCLUDES_H__

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef signed long long int64_t;
typedef unsigned long size_t;

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define UINT32_C(x) ((uint32_t) x)
#define UINT64_C(x) ((uint64_t) x)
#define PRIu64 "llu"
extern void debug(const char *format, ...);

#define POSSIBLY_UNUSED __attribute__((unused))

#ifdef __STRICT_ANSI__
#define INLINE
#else
#define INLINE inline
#endif

#define UINT64_RSHIFT(v, shiftby) (((uint64_t)(v)) >> (shiftby))
#define UINT64_MULT32(v, multby)  (((uint64_t)(v)) * ((uint32_t)(multby)))

#ifndef UINT32_MAX
#define UINT32_MAX (UINT32_C(0xffffffffU))
#endif

#ifndef UINT64_MAX
#define UINT64_MAX (UINT64_C(0xffffffffffffffffULL))
#endif

#endif /*__ARCH_X86_BIOSINCLUDES_H__ */
