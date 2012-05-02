/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdint.h>
#include "config.h"

/* FMAP structs. See http://code.google.com/p/flashmap/wiki/FmapSpec */
#define FMAP_NAMELEN 32
#define FMAP_SIGNATURE "__FMAP__"
#define FMAP_SIGNATURE_SIZE 8
#define FMAP_VER_MAJOR 1
#define FMAP_VER_MINOR 0
#define FMAP_SEARCH_STRIDE 64		/* Spec revision 1.01 */

typedef struct _FmapHeader {
	char        fmap_signature[FMAP_SIGNATURE_SIZE];
	uint8_t     fmap_ver_major;
	uint8_t     fmap_ver_minor;
	uint64_t    fmap_base;
	uint32_t    fmap_size;
	char        fmap_name[FMAP_NAMELEN];
	uint16_t    fmap_nareas;
} __attribute__((packed)) FmapHeader;

#define FMAP_AREA_STATIC      (1 << 0)	/* can be checksummed */
#define FMAP_AREA_COMPRESSED  (1 << 1)  /* may be compressed */
#define FMAP_AREA_RO          (1 << 2)  /* writes may fail */

typedef struct _FmapAreaHeader {
	uint32_t area_offset;
	uint32_t area_size;
	char     area_name[FMAP_NAMELEN];
	uint16_t area_flags;
} __attribute__((packed)) FmapAreaHeader;


#define NUM_EC_FMAP_AREAS 14

const struct _ec_fmap {
	FmapHeader header;
	FmapAreaHeader area[NUM_EC_FMAP_AREAS];
} ec_fmap __attribute__((section(".google"))) = {
	/* Header */
	{
		.fmap_signature = {'_', '_', 'F', 'M', 'A', 'P', '_', '_'},
		.fmap_ver_major = FMAP_VER_MAJOR,
		.fmap_ver_minor = FMAP_VER_MINOR,
		.fmap_base = CONFIG_FLASH_BASE,
		/* NOTE: EC implementation reserves one bank for itself */
		.fmap_size = CONFIG_FLASH_SIZE - CONFIG_FLASH_BANK_SIZE,
		.fmap_name = "EC_FMAP",
		.fmap_nareas = NUM_EC_FMAP_AREAS,
	},

	{
	/* RO Firmware */
		{
			.area_name = "RO_SECTION",
			.area_offset = CONFIG_FW_RO_OFF,
			.area_size = CONFIG_FW_IMAGE_SIZE,
			.area_flags = FMAP_AREA_STATIC | FMAP_AREA_RO,
		},
		{
			.area_name = "BOOT_STUB",
			.area_offset = CONFIG_FW_RO_OFF,
			.area_size = CONFIG_FW_RO_SIZE,
			.area_flags = FMAP_AREA_STATIC | FMAP_AREA_RO,
		},
		{
			.area_name = "RO_FRID",	/* FIXME: Where is it? */
			.area_offset = CONFIG_FW_RO_OFF,
			.area_size = 0,
			.area_flags = FMAP_AREA_STATIC | FMAP_AREA_RO,
		},

		/* Other RO stuff: FMAP, GBB, etc. */
		{
			/* FIXME(wfrichar): GBB != FMAP. Use the right terms */
			.area_name = "FMAP",
			.area_offset = CONFIG_FW_RO_GBB_OFF,
			.area_size = CONFIG_FW_RO_GBB_SIZE,
			.area_flags = FMAP_AREA_STATIC | FMAP_AREA_RO,
		},
		{
			.area_name = "GBB",
			.area_offset = CONFIG_FW_RO_GBB_OFF,
			.area_size = 0,
			.area_flags = FMAP_AREA_STATIC | FMAP_AREA_RO,
		},
		{
			/* A dummy region to identify it as EC firmware */
			.area_name = "EC_IMAGE",
			.area_offset = CONFIG_FW_RO_GBB_OFF,
			.area_size = 0, /* Always zero */
			.area_flags = FMAP_AREA_STATIC | FMAP_AREA_RO,
		},

		/* Firmware A */
		{
			.area_name = "RW_SECTION_A",
			.area_offset = CONFIG_FW_A_OFF,
			.area_size = CONFIG_FW_IMAGE_SIZE,
			.area_flags = FMAP_AREA_STATIC,
		},
		{
			.area_name = "FW_MAIN_A",
			.area_offset = CONFIG_FW_A_OFF,
			.area_size = CONFIG_FW_IMAGE_SIZE,
			.area_flags = FMAP_AREA_STATIC,
		},
		{
			.area_name = "RW_FWID_A", /* FIXME: Where is it? */
			.area_offset = CONFIG_FW_A_OFF,
			.area_size = 0,
			.area_flags = FMAP_AREA_STATIC,
		},
		{
			.area_name = "VBLOCK_A",
			.area_offset = CONFIG_FW_A_OFF,
			.area_size = 0,
			.area_flags = FMAP_AREA_STATIC,
		},

		/* Firmware B */
		{
			.area_name = "RW_SECTION_B",
			.area_offset = CONFIG_FW_B_OFF,
			.area_size = CONFIG_FW_IMAGE_SIZE,
			.area_flags = FMAP_AREA_STATIC,
		},
		{
			.area_name = "FW_MAIN_B",
			.area_offset = CONFIG_FW_B_OFF,
			.area_size = CONFIG_FW_IMAGE_SIZE,
			.area_flags = FMAP_AREA_STATIC,
		},
		{
			.area_name = "RW_FWID_B", /* FIXME: Where is it? */
			.area_offset = CONFIG_FW_B_OFF,
			.area_size = 0,
			.area_flags = FMAP_AREA_STATIC,
		},
		{
			.area_name = "VBLOCK_B",
			.area_offset = CONFIG_FW_A_OFF,
			.area_size = 0,
			.area_flags = FMAP_AREA_STATIC,
		},
	}
};
