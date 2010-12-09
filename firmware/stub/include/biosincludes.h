/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef CHROMEOS_SRC_PLATFORM_VBOOT_REFERENCE_FIRMWARE_STUB_BIOSINCLUDES_H_
#define CHROMEOS_SRC_PLATFORM_VBOOT_REFERENCE_FIRMWARE_STUB_BIOSINCLUDES_H_

/*
 * This file is a placeholder for the includes supplied by the BIOS
 * compilation environment. This file is included if and only if
 * CHROMEOS_ENVIRONMENT is not defined at compilation time.
 */

#ifdef TARGET_TEST_MODE

typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned size_t;

#ifndef NULL
#define NULL ((void*) 0)
#endif

#define UINT64_C(x) ((uint64_t)x)
#define __attribute__(x)
#define PRIu64 "llu"
extern void debug(const char *format, ...);

#define POSSIBLY_UNUSED
#define INLINE

#endif

#endif /*CHROMEOS_SRC_PLATFORM_VBOOT_REFERENCE_FIRMWARE_STUB_BIOSINCLUDES_H_*/
