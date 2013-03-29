/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of utility functions which call their linux-specific
 * equivalents.
 */

#include <stdint.h>

#define _STUB_IMPLEMENTATION_
#include "utility.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int Memcmp(const void *src1, const void *src2, size_t n)
{
	return memcmp(src1, src2, n);
}

void *Memcpy(void *dest, const void *src, uint64_t n)
{
	return memcpy(dest, src, (size_t)n);
}

void *Memset(void *d, const uint8_t c, uint64_t n)
{
	return memset(d, c, n);
}


