/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "fmap.h"

/* Find and point to the FMAP header within the buffer */
FmapHeader *fmap_find(uint8_t *ptr, size_t size)
{
	size_t i;
	FmapHeader *fmap_header;
	for (i=0; i<size; i += FMAP_SEARCH_STRIDE, ptr += FMAP_SEARCH_STRIDE) {
		if (0 != memcmp(ptr, FMAP_SIGNATURE, FMAP_SIGNATURE_SIZE))
			continue;
		fmap_header = (FmapHeader *)ptr;
		if (fmap_header->fmap_ver_major == FMAP_VER_MAJOR)
			return fmap_header;
	}
	return NULL;
}

/* Search for an area by name, return pointer to its beginning */
uint8_t *fmap_find_by_name(uint8_t *ptr, size_t size, FmapHeader *fmap,
			   const char *name, FmapAreaHeader **ah_ptr)
{
	int i;
	FmapAreaHeader *ah;

	if (!fmap)
		fmap = fmap_find(ptr, size);
	if (!fmap)
		return NULL;

	ah = (FmapAreaHeader*)((void *)fmap + sizeof(FmapHeader));
	for (i = 0; i < fmap->fmap_nareas; i++)
		if (!strncmp(ah[i].area_name, name, FMAP_NAMELEN)) {
			if (ah_ptr)
				*ah_ptr = ah + i;
			return ptr + ah[i].area_offset;
		}

	return NULL;
}

