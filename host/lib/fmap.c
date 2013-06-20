/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "fmap.h"

const char* FmapFind(const char* ptr, size_t size)
{
  size_t i;
  FmapHeader *fmap_header;
  for (i=0; i<size; i += FMAP_SEARCH_STRIDE, ptr += FMAP_SEARCH_STRIDE) {
    if (0 != strncmp(ptr, FMAP_SIGNATURE, FMAP_SIGNATURE_SIZE))
      continue;
    // Image may have multiple signatures (ex, in code that handles FMAP itself)
    // so we do want to check at least major version.
    fmap_header = (FmapHeader *)ptr;
    if (fmap_header->fmap_ver_major == FMAP_VER_MAJOR)
      return ptr;
  }
  return NULL;
}

int FmapAreaIndex(const FmapHeader* fh, const FmapAreaHeader* ah,
		const char* name) {
  int i;
  for (i = 0; i < fh->fmap_nareas; i++)
    if (!strcmp((const char*) ah[i].area_name, name))
      return i;
  return -1;
}
