/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This describes the internal format used to pack a set of character glpyhs so
 * we can render strings by drawing one character at a time.
 *
 * The format is this:
 *
 *   +-------------------------+
 *   | FontArrayHeader         |
 *   +-------------------------+
 *   | FontArrayEntryHeader[0] |
 *   +-------------------------+
 *   | raw image data[0]       |
 *   +-------------------------+
 *   | FontArrayEntryHeader[1] |
 *   +-------------------------+
 *   | raw image data[1]       |
 *   +-------------------------+
 *   | FontArrayEntryHeader[2] |
 *   +-------------------------+
 *   | raw image data[2]       |
 *   +-------------------------+
 *      ...
 *   +-------------------------+
 *   | FontArrayEntryHeader[n] |
 *   +-------------------------+
 *   | raw image data[n]       |
 *   +-------------------------+
 *
 * The FontArrayHeader describes how many characters will be encoded.
 * Each character encoding consists of a FontArrayEntryHeader followed
 * immediately by the raw image data for that character.
 */

#ifndef VBOOT_REFERENCE_BMPBLK_FONT_H_
#define VBOOT_REFERENCE_BMPBLK_FONT_H_

#include "bmpblk_header.h"

#define FONT_SIGNATURE      "FONT"
#define FONT_SIGNATURE_SIZE 4

typedef struct FontArrayHeader {
	uint8_t  signature[FONT_SIGNATURE_SIZE];
	uint32_t num_entries;  /* Number of chars encoded here. */
} __attribute__((packed)) FontArrayHeader;

typedef struct FontArrayEntryHeader {
	uint32_t ascii;  /* What to show. Could even be UTF? */
	ImageInfo info;  /* Describes the bitmap. */

	/*
	 * The image to use follows immediately, NOT compressed. It's
	 * uncompressed because each glyph is only a few hundred bytes, but
	 * they have much in common (colormaps, for example). When we add the
	 * whole font blob to the bmpblk, it will be compressed as a single
	 * item there.
	 */
} __attribute__((packed)) FontArrayEntryHeader;

#endif  /* VBOOT_REFERENCE_BMPBLK_FONT_H_ */
