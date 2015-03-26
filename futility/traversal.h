/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_FUTILITY_TRAVERSAL_H_
#define VBOOT_REFERENCE_FUTILITY_TRAVERSAL_H_
#include <stdint.h>
#include "fmap.h"

/*
 * The Chrome OS BIOS must contain specific FMAP areas, and we generally want
 * to look at each one in a certain order.
 */
enum bios_component {
	BIOS_FMAP_GBB,
	BIOS_FMAP_FW_MAIN_A,
	BIOS_FMAP_FW_MAIN_B,
	BIOS_FMAP_VBLOCK_A,
	BIOS_FMAP_VBLOCK_B,

	NUM_BIOS_COMPONENTS
};

/* These are the expected areas, in order of traversal */
extern struct bios_fmap_s {
	enum bios_component component;
	const char * const name;
	/* The Cr-48 BIOS images have different FMAP names but work the same,
	 * so we allow those too. */
	const char * const oldname;
} bios_area[];


void fmap_limit_area(FmapAreaHeader *ah, uint32_t len);


#endif	/* VBOOT_REFERENCE_FUTILITY_TRAVERSAL_H_ */
