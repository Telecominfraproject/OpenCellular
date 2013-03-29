/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of firmware-provided API functions.
 */

#include <stdint.h>

#define _STUB_IMPLEMENTATION_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "vboot_api.h"

/* U-Boot's printf uses '%L' for uint64_t. gcc uses '%l'. */
#define MAX_FMT 255
static char fmtbuf[MAX_FMT+1];

static const char *fixfmt(const char *format)
{
	int i;
	for(i=0; i<MAX_FMT && format[i]; i++) {
		fmtbuf[i] = format[i];
		if(format[i] == '%' && format[i+1] == 'L') {
			fmtbuf[i+1] = 'l';
			i++;
		}
	}
	fmtbuf[i] = '\0';
	return fmtbuf;
}

void VbExError(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, fixfmt(format), ap);
	va_end(ap);
	exit(1);
}

void VbExDebug(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "DEBUG: ");
	vfprintf(stderr, fixfmt(format), ap);
	va_end(ap);
}

uint64_t VbExGetTimer(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}

VbError_t VbExNvStorageRead(uint8_t *buf)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExNvStorageWrite(const uint8_t *buf)
{
	return VBERROR_SUCCESS;
}
