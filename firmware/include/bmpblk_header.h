/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure definitions for firmware screen block (BMPBLOCK).
 *
 * The BmpBlock structure looks like:
 *  +-----------------------------------------+
 *  |             BmpBlock Header             |
 *  +-----------------------------------------+
 *  |             ScreenLayout[0]             | \
 *  +-----------------------------------------+  |
 *  |             ScreenLayout[1]             |  |
 *  +-----------------------------------------+  Localization[0]
 *  |                  ...                    |  |
 *  +-----------------------------------------+  |
 *  | ScreenLayout[number_of_screenlayouts-1] | /
 *  +-----------------------------------------+
 *  |             ScreenLayout[0]             | \
 *  +-----------------------------------------+  Localization[1]
 *  |                  ...                    | /
 *  +-----------------------------------------+        ...
 *  |             ScreenLayout[0]             | \
 *  +-----------------------------------------+  Localization[
 *  |                  ...                    | /   number_of_localizations-1]
 *  +-----------------------------------------+
 *  |              ImageInfo[0]               |
 *  +-----------------------------------------+
 *  |              Image Content              |
 *  +-----------------------------------------+
 *  |              ImageInfo[2]               |  ImageInfo is 4-byte aligned.
 *  +-----------------------------------------+
 *  |              Image Content              |
 *  +-----------------------------------------+
 *  |                  ...                    |
 *  +-----------------------------------------+
 *  |      ImageInfo[number_fo_images-1]      |
 *  +-----------------------------------------+
 *  |              Image Content              |
 *  +-----------------------------------------+
 *  |        List of locale names             |
 *  +-----------------------------------------+
 */
#ifndef VBOOT_REFERENCE_BMPBLK_HEADER_H_
#define VBOOT_REFERENCE_BMPBLK_HEADER_H_
#include <stdint.h>

#define BMPBLOCK_SIGNATURE      "$BMP"
#define BMPBLOCK_SIGNATURE_SIZE (4)

#define BMPBLOCK_MAJOR_VERSION  (0x0002)
#define BMPBLOCK_MINOR_VERSION  (0x0000)

#define MAX_IMAGE_IN_LAYOUT     (16)

/* BMPBLOCK header, describing how many screen layouts and image infos */
typedef struct BmpBlockHeader {
	/* BMPBLOCK_SIGNATURE $BMP */
	uint8_t  signature[BMPBLOCK_SIGNATURE_SIZE];
	uint16_t major_version;            /* see BMPBLOCK_MAJOR_VER */
	uint16_t minor_version;            /* see BMPBLOCK_MINOR_VER */
	uint32_t number_of_localizations;  /* Number of localizations */
	/* Number of screen layouts in each localization */
	uint32_t number_of_screenlayouts;
	uint32_t number_of_imageinfos;     /* Number of image infos */
	/* Offset of locale-translation string */
	uint32_t locale_string_offset;
	uint32_t reserved[2];
} __attribute__((packed)) BmpBlockHeader;

/* Screen layout, describing how to stack multiple images on screen */
typedef struct ScreenLayout {
	/*
	 * Images contained in the screen. Will be rendered from 0 to
	 * (number_of_images-1).
	 */
	struct {
		/* (X,Y) offset of image to be rendered */
		uint32_t x;
		uint32_t y;
		/* Offset of image info from start of BMPBLOCK; 0=end it. */
		uint32_t image_info_offset;
	} images[MAX_IMAGE_IN_LAYOUT];
} __attribute__((packed)) ScreenLayout;

/* Constants for screen index */
typedef enum ScreenIndex {
	SCREEN_DEVELOPER_WARNING = 0,
	SCREEN_RECOVERY_REMOVE,
	SCREEN_RECOVERY_NO_GOOD,
	SCREEN_RECOVERY_INSERT,
	SCREEN_RECOVERY_TO_DEV,
	SCREEN_DEVELOPER_TO_NORM,
	SCREEN_WAIT,
	SCREEN_TO_NORM_CONFIRMED,
	MAX_VALID_SCREEN_INDEX,
	SCREEN_BLANK = ~0UL,
} ScreenIndex;

/* Image info, describing the information of the image block */
typedef struct ImageInfo {
	uint32_t tag;              /* Tag it as a special image, like HWID */
	uint32_t width;            /* Width of the image */
	uint32_t height;           /* Height of the image */
	uint32_t format;           /* File format of the image */
	uint32_t compression;      /* Compression method for the image file */
	uint32_t original_size;    /* Size of the original uncompressed image */
	/*
	 * Size of the compressed image; if image is not compressed, this will
	 * be the same as the original size.
	 */
	uint32_t compressed_size;
	uint32_t reserved;
  /* NOTE: The actual image content (if any) follows immediately. */
} __attribute__((packed)) ImageInfo;

/* Constants for ImageInfo.tag */
typedef enum ImageTag {
	TAG_NONE = 0,
	TAG_HWID,
	TAG_HWID_RTOL,  /* "right-to-left", ie, right-justified HWID */
} ImageTag;

/* Constants for ImageInfo.format */
typedef enum ImageFormat {
	FORMAT_INVALID = 0,
	FORMAT_BMP,
	FORMAT_FONT,
} ImageFormat;

/*
 * These magic image names can be used in the .yaml file to indicate that the
 * ASCII HWID should be displayed. For RENDER_HWID, the image coordinates
 * specify upper-left corner of the HWID string. For RENDER_HWID_RTOL, they
 * indicate the upper-right corner (handy for right-to-left languages).
 */
#define RENDER_HWID       "$HWID"
#define RENDER_HWID_RTOL  "$HWID.rtol"

#endif  /* VBOOT_REFERENCE_BMPBLK_HEADER_H_ */
