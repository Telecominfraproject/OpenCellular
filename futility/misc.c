/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include <string.h>

#include "gbb_header.h"

static int is_null_terminated(const char *s, int len)
{
	len--;
	s += len;
	while (len-- >= 0)
		if (!*s--)
			return 1;
	return 0;
}

static inline uint32_t max(uint32_t a, uint32_t b)
{
	return a > b ? a : b;
}

int futil_looks_like_gbb(GoogleBinaryBlockHeader *gbb, uint32_t len)
{
	if (memcmp(gbb->signature, GBB_SIGNATURE, GBB_SIGNATURE_SIZE))
		return 0;
	if (gbb->major_version > GBB_MAJOR_VER)
		return 0;
	if (sizeof(GoogleBinaryBlockHeader) > len)
		return 0;

	/* close enough */
	return 1;
}

int futil_valid_gbb_header(GoogleBinaryBlockHeader *gbb, uint32_t len,
			   uint32_t *maxlen_ptr)
{
	if (memcmp(gbb->signature, GBB_SIGNATURE, GBB_SIGNATURE_SIZE))
		return 0;
	if (gbb->major_version != GBB_MAJOR_VER)
		return 0;

	/* Check limits first, to help identify problems */
	if (maxlen_ptr) {
		uint32_t maxlen = gbb->header_size;
		maxlen = max(maxlen,
			     gbb->hwid_offset + gbb->hwid_size);
		maxlen = max(maxlen,
			     gbb->rootkey_offset + gbb->rootkey_size);
		maxlen = max(maxlen,
			     gbb->bmpfv_offset + gbb->bmpfv_size);
		maxlen = max(maxlen,
			     gbb->recovery_key_offset + gbb->recovery_key_size);
		*maxlen_ptr = maxlen;
	}

	if (gbb->header_size != GBB_HEADER_SIZE || gbb->header_size > len)
		return 0;
	if (gbb->hwid_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->hwid_offset + gbb->hwid_size > len)
		return 0;
	if (gbb->hwid_size) {
		const char *s = (const char *)
			((uint8_t *)gbb + gbb->hwid_offset);
		if (!is_null_terminated(s, gbb->hwid_size))
			return 0;
	}
	if (gbb->rootkey_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->rootkey_offset + gbb->rootkey_size > len)
		return 0;

	if (gbb->bmpfv_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->bmpfv_offset + gbb->bmpfv_size > len)
		return 0;
	if (gbb->recovery_key_offset < GBB_HEADER_SIZE)
		return 0;
	if (gbb->recovery_key_offset + gbb->recovery_key_size > len)
		return 0;

	/* Seems legit... */
	return 1;
}
