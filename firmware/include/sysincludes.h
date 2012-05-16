/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* System includes for vboot reference library.  This is the ONLY
 * place in firmware/ where system headers may be included via
 * #include <...>, so that there's only one place that needs to be
 * fixed up for platforms which don't have all the system includes.
 *
 * Files in firmware/stub may still include system headers, because
 * they're local implementations and will be ported to each system
 * anyway. */

#ifndef VBOOT_REFERENCE_SYSINCLUDES_H_
#define VBOOT_REFERENCE_SYSINCLUDES_H_

#ifdef CHROMEOS_ENVIRONMENT

#include <inttypes.h>  /* For PRIu64 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(HAVE_ENDIAN_H) && defined(HAVE_LITTLE_ENDIAN)
#include <byteswap.h>
#include <memory.h>
#endif

#define POSSIBLY_UNUSED __attribute__((unused))

#ifdef __STRICT_ANSI__
#define INLINE
#else
#define INLINE inline
#endif

#else
#include "biosincludes.h"
#endif

#ifndef _MSC_VER
#define __pragma(...)
#endif

#if defined (CHROMEOS_ENVIRONMENT) || defined (TARGET_TEST_MODE)

/* 64-bit operations, for platforms where they need to be function calls */
#define UINT64_RSHIFT(v, shiftby) (((uint64_t)(v)) >> (shiftby))
#define UINT64_MULT32(v, multby)  (((uint64_t)(v)) * ((uint32_t)(multby)))

#endif

#endif  /* VBOOT_REFERENCE_SYSINCLUDES_H_ */
