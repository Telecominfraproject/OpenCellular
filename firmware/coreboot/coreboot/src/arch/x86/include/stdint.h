/*
 * This file is part of the coreboot project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef X86_STDINT_H
#define X86_STDINT_H

#if defined(__GNUC__)
#define __HAVE_LONG_LONG__ 1
#else
#define __HAVE_LONG_LONG__ 0
#endif

/* Exact integral types */
typedef unsigned char      uint8_t;
typedef signed char        int8_t;

typedef unsigned short     uint16_t;
typedef signed short       int16_t;

typedef unsigned int       uint32_t;
typedef signed int         int32_t;

#if __HAVE_LONG_LONG__
typedef unsigned long long uint64_t;
typedef signed long long   int64_t;
#endif

/* Small types */
typedef unsigned char      uint_least8_t;
typedef signed char        int_least8_t;

typedef unsigned short     uint_least16_t;
typedef signed short       int_least16_t;

typedef unsigned int       uint_least32_t;
typedef signed int         int_least32_t;

#if __HAVE_LONG_LONG__
typedef unsigned long long uint_least64_t;
typedef signed long long   int_least64_t;
#endif

/* Fast Types */
typedef unsigned char      uint_fast8_t;
typedef signed char        int_fast8_t;

typedef unsigned int       uint_fast16_t;
typedef signed int         int_fast16_t;

typedef unsigned int       uint_fast32_t;
typedef signed int         int_fast32_t;

#if __HAVE_LONG_LONG__
typedef unsigned long long uint_fast64_t;
typedef signed long long   int_fast64_t;
#endif

/* Types for `void *' pointers.  */
typedef long               intptr_t;
typedef unsigned long      uintptr_t;

/* Largest integral types */
#if __HAVE_LONG_LONG__
typedef long long int      intmax_t;
typedef unsigned long long uintmax_t;
#else
typedef long int           intmax_t;
typedef unsigned long int  uintmax_t;
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#if __HAVE_LONG_LONG__
typedef uint64_t u64;
#endif
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

typedef uint8_t bool;
#define true	1
#define false	0

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif
#ifndef UINT64_MAX
# define UINT64_MAX (18446744073709551615ULL)
#endif

#ifdef __x86_64__

#ifndef UINT64_C
#define UINT64_C(c) c ## UL
#endif
#ifndef PRIu64
#define PRIu64 "lu"
#endif

#else

#ifndef UINT64_C
#define UINT64_C(c) c ## ULL
#endif
#ifndef PRIu64
#define PRIu64 "llu"
#endif

#endif


#undef __HAVE_LONG_LONG__

#endif /* X86_STDINT_H */
