/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * String utility functions that need to be built as part of the firmware.
 */

#include "sysincludes.h"

#include "utility.h"


uint32_t Uint64ToString(char *buf, uint32_t bufsize, uint64_t value,
			uint32_t radix, uint32_t zero_pad_width)
{
	char ibuf[UINT64_TO_STRING_MAX];
	char *s;
	uint32_t usedsize = 1;

	if (!buf)
		return 0;

	/* Clear output buffer in case of error */
	*buf = '\0';

	/* Sanity-check input args */
	if (radix < 2 || radix > 36 || zero_pad_width >= UINT64_TO_STRING_MAX)
		return 0;

	/* Start at end of string and work backwards */
	s = ibuf + UINT64_TO_STRING_MAX - 1;
	*(s) = '\0';
	do {
		int v = value % radix;
		value /= radix;

		*(--s) = (char)(v < 10 ? v + '0' : v + 'a' - 10);
		if (++usedsize > bufsize)
			return 0;  /* Result won't fit in buffer */
	} while (value);

	/* Zero-pad if necessary */
	while (usedsize <= zero_pad_width) {
		*(--s) = '0';
		if (++usedsize > bufsize)
			return 0;  /* Result won't fit in buffer */
	}

	/* Now copy the string back to the input buffer. */
	Memcpy(buf, s, usedsize);

	/* Don't count the terminating null in the bytes used */
	return usedsize - 1;
}

uint32_t StrnAppend(char *dest, const char *src, uint32_t destlen)
{
	uint32_t used = 0;

	if (!dest || !src || !destlen)
		return 0;

	/* Skip past existing string in destination.*/
	while (dest[used] && used < destlen - 1)
		used++;

	/* Now copy source */
	while (*src && used < destlen - 1)
		dest[used++] = *src++;

	/* Terminate destination and return count of non-null characters */
	dest[used] = 0;
	return used;
}
