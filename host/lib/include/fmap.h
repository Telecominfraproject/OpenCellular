/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __FMAP_H__
#define __FMAP_H__

#include <inttypes.h>
#include <stddef.h>

/* FMAP structs. See http://code.google.com/p/flashmap/wiki/FmapSpec */
#define FMAP_NAMELEN 32
#define FMAP_SIGNATURE "__FMAP__"
#define FMAP_SIGNATURE_SIZE 8
#define FMAP_SEARCH_STRIDE 4
#define FMAP_VER_MAJOR 1
typedef struct _FmapHeader {
  char        fmap_signature[FMAP_SIGNATURE_SIZE]; /* avoiding endian issues */
  uint8_t     fmap_ver_major;
  uint8_t     fmap_ver_minor;
  uint64_t    fmap_base;
  uint32_t    fmap_size;
  char        fmap_name[FMAP_NAMELEN];
  uint16_t    fmap_nareas;
} __attribute__((packed)) FmapHeader;

typedef struct _FmapAreaHeader {
  uint32_t area_offset;
  uint32_t area_size;
  char     area_name[FMAP_NAMELEN];
  uint16_t area_flags;
} __attribute__((packed)) FmapAreaHeader;


/* Find and point to the FMAP header within the buffer */
FmapHeader *fmap_find(uint8_t *ptr, size_t size);

/* Search for an area by name, return pointer to its beginning */
uint8_t *fmap_find_by_name(uint8_t *ptr, size_t size,
			   /* optional, will call fmap_find() if NULL */
			   FmapHeader *fmap,
			   /* The area name to search for */
			   const char *name,
			   /* optional, return pointer to entry if not NULL */
			   FmapAreaHeader **ah);

#endif  /* __FMAP_H__ */
