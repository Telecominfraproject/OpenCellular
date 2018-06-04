/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_FILE_TYPE_BIOS_H_
#define VBOOT_REFERENCE_FUTILITY_FILE_TYPE_BIOS_H_
#include <stdint.h>

/*
 * The Chrome OS BIOS must contain specific FMAP areas, which we want to look
 * at in a certain order.
 */
enum bios_component {
	BIOS_FMAP_GBB,
	BIOS_FMAP_FW_MAIN_A,
	BIOS_FMAP_FW_MAIN_B,
	BIOS_FMAP_VBLOCK_A,
	BIOS_FMAP_VBLOCK_B,

	NUM_BIOS_COMPONENTS
};

/* Location information for each component */
struct bios_area_s {
	uint32_t offset;			/* to avoid pointer math */
	uint8_t *buf;
	uint32_t len;
	uint32_t is_valid;
};

/* State to track as we visit all components */
struct bios_state_s {
	/* Current component */
	enum bios_component c;
	/* Other activites, possibly before or after the current one */
	struct bios_area_s area[NUM_BIOS_COMPONENTS];
	struct bios_area_s recovery_key;
	struct bios_area_s rootkey;
};

#endif	/* VBOOT_REFERENCE_FUTILITY_FILE_TYPE_BIOS_H_ */
