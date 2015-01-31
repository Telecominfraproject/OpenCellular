/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "fmap.h"

static int is_fmap(uint8_t *ptr)
{
	FmapHeader *fmap_header = (FmapHeader *)ptr;

	if (0 != memcmp(ptr, FMAP_SIGNATURE, FMAP_SIGNATURE_SIZE))
		return 0;

	if (fmap_header->fmap_ver_major == FMAP_VER_MAJOR)
		return 1;

	fprintf(stderr, "Found FMAP, but major version is %u instead of %u\n",
		fmap_header->fmap_ver_major, FMAP_VER_MAJOR);
	return 0;
}

/* Find and point to the FMAP header within the buffer */
FmapHeader *fmap_find(uint8_t *ptr, size_t size)
{
	ssize_t offset, align;
	ssize_t lim = size - sizeof(FmapHeader);

	if (lim >= 0 && is_fmap(ptr))
		return (FmapHeader *)ptr;

	/* Search large alignments before small ones to find "right" FMAP. */
	for (align = FMAP_SEARCH_STRIDE; align <= lim; align *= 2);
	for (; align >= FMAP_SEARCH_STRIDE; align /= 2)
		for (offset = align; offset <= lim; offset += align * 2)
			if (is_fmap(ptr + offset))
				return (FmapHeader *)(ptr + offset);

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
