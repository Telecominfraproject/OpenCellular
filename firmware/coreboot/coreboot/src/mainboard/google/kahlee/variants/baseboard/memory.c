/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Google Inc.
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

#include <baseboard/variants.h>
#include <compiler.h>
#include <console/console.h>
#include <gpio.h> /* src/include/gpio.h */
#include <spd_bin.h>
#include <variant/gpio.h>
#include <amdblocks/dimm_spd.h>

uint8_t __weak variant_memory_sku(void)
{
	gpio_t pads[] = {
		[3] = MEM_CONFIG3,
		[2] = MEM_CONFIG2,
		[1] = MEM_CONFIG1,
		[0] = MEM_CONFIG0,
	};

	return gpio_base2_value(pads, ARRAY_SIZE(pads));
}

int __weak variant_mainboard_read_spd(uint8_t spdAddress,
							char *buf, size_t len)
{
	struct region_device spd_rdev;
	u8 spd_index = variant_memory_sku();

	printk(BIOS_INFO, "%s SPD index %d\n", __func__, spd_index);

	if (get_spd_cbfs_rdev(&spd_rdev, spd_index) < 0) {
		printk(BIOS_ERR, "Error: spd.bin not found\n");
		return -1;
	}

	if (len != region_device_sz(&spd_rdev)) {
		printk(BIOS_ERR, "Error: spd.bin is not the correct size\n");
		return -1;
	}

	if (rdev_readat(&spd_rdev, buf, 0, len) != len) {
		printk(BIOS_ERR, "Error: couldn't read spd.bin\n");
		return -1;
	}

	return 0;
}
