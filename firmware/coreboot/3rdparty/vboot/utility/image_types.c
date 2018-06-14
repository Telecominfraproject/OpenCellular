// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <string.h>

#include "bmpblk_header.h"
#include "bmpblk_font.h"
#include "image_types.h"

/* BMP header, used to validate image requirements
 * See http://en.wikipedia.org/wiki/BMP_file_format
 */
typedef struct {
  uint8_t         CharB;                // must be 'B'
  uint8_t         CharM;                // must be 'M'
  uint32_t        Size;
  uint16_t        Reserved[2];
  uint32_t        ImageOffset;
  uint32_t        HeaderSize;
  uint32_t        PixelWidth;
  uint32_t        PixelHeight;
  uint16_t        Planes;               // Must be 1 for x86
  uint16_t        BitPerPixel;          // 1, 4, 8, or 24 for x86
  uint32_t        CompressionType;      // 0 (none) for x86, 1 (RLE) for arm
  uint32_t        ImageSize;
  uint32_t        XPixelsPerMeter;
  uint32_t        YPixelsPerMeter;
  uint32_t        NumberOfColors;
  uint32_t        ImportantColors;
} __attribute__((packed)) BMP_IMAGE_HEADER;


ImageFormat identify_image_type(const void *buf, uint32_t bufsize,
                                ImageInfo *info) {

  if (info)
    info->format = FORMAT_INVALID;

  if (bufsize < sizeof(BMP_IMAGE_HEADER) &&
      bufsize < sizeof(FontArrayHeader)) {
    return FORMAT_INVALID;
  }

  const BMP_IMAGE_HEADER *bhdr = buf;
  if (bhdr->CharB == 'B' && bhdr->CharM == 'M' &&
      bhdr->Planes == 1 &&
      (bhdr->CompressionType == 0 || bhdr->CompressionType == 1) &&
      (bhdr->BitPerPixel == 1 || bhdr->BitPerPixel == 4 ||
       bhdr->BitPerPixel == 8 || bhdr->BitPerPixel == 24)) {
    if (info) {
      info->format = FORMAT_BMP;
      info->width = bhdr->PixelWidth;
      info->height = bhdr->PixelHeight;
    }
    return FORMAT_BMP;
  }

  const FontArrayHeader *fhdr = buf;
  if (0 == memcmp(&fhdr->signature, FONT_SIGNATURE, FONT_SIGNATURE_SIZE) &&
      fhdr->num_entries > 0) {
    if (info)
      info->format = FORMAT_FONT;
    return FORMAT_FONT;
  }

  return FORMAT_INVALID;
}


