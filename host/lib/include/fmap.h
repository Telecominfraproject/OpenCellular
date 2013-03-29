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


/* Scan firmware image, pointed by [ptr] with length [size], for fmap header.
 * Return pointer to fmap header, or NULL if not found.
 */
const char* FmapFind(const char *ptr, size_t size);

/* Look up fmap area by name, that is, strcmp(fh->fmap_name, name) == 0.
 * Return index of fmap area, that is, ah[returned_index],
 * or -1 if not found. */
int FmapAreaIndex(const FmapHeader* fh, const FmapAreaHeader* ah,
		const char* name);

#endif  /* __FMAP_H__ */
