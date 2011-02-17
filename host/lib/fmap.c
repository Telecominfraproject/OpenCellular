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
  for (i=0; i<size; i += FMAP_SEARCH_STRIDE) {
    if (0 == strncmp(ptr, FMAP_SIGNATURE, FMAP_SIGNATURE_SIZE))
      return ptr;
    ptr += FMAP_SEARCH_STRIDE;
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
