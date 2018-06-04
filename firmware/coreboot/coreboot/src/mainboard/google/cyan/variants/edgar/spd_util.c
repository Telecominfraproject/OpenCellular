/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
 * Copyright (C) 2017 Matt DeVillier
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <console/console.h>
#include <mainboard/google/cyan/spd/spd_util.h>

/*
 *   0b0000 - 4GiB total - 2 x 2GiB Samsung K4E8E304EE-EGCE
 *   0b0001 - 4GiB total - 2 x 2GiB Hynix H9CCNNN8JTBLAR-NUD
 *   0b0010 - 2GiB total - 1 x 2GiB Samsung K4E8E304EE-EGCE
 *   0b0011 - 2GiB total - 1 x 2GiB Hynix H9CCNNN8JTBLAR-NUD
 *   0b0100 - 4GiB total - 2 x 2GiB Samsung K4E8E324EB-EGCF
 *   0b0101 - TBD
 *   0b0110 - 2GiB total - 1 x 2GiB Samsung K4E8E324EB-EGCF
 */

int get_variant_spd_index(int ram_id, int *dual)
{
	/* Determine if single or dual channel memory system */
	/* RAMID1 is deterministic for edgar */
	*dual = ((ram_id >> 1) & 0x1) ? 0 : 1;

	/* Display the RAM type */
	switch (ram_id) {
	case 0:
		printk(BIOS_DEBUG, "4GiB Samsung K4E8E304EE-EGCE\n");
		break;
	case 1:
		printk(BIOS_DEBUG, "4GiB Hynix H9CCNNN8JTBLAR-NUD\n");
		break;
	case 2:
		printk(BIOS_DEBUG, "2GiB Samsung K4E8E304EE-EGCE\n");
		break;
	case 3:
		printk(BIOS_DEBUG, "2GiB Hynix H9CCNNN8JTBLAR-NUD\n");
		break;
	case 4:
		printk(BIOS_DEBUG, "4GiB Samsung K4E8E324EB-EGCF\n");
		break;
	case 5:
		printk(BIOS_DEBUG, "empty\n");
		break;
	case 6:
		printk(BIOS_DEBUG, "2GiB Samsung K4E8E324EB-EGCF\n");
		break;
	}

	/* 1:1 mapping between ram_id and spd_index for edgar */
	return ram_id;
}
