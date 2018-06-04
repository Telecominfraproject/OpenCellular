/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Marvell, Inc.
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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <boot_device.h>
#include <soc/addressmap.h>
#include <symbols.h>

static struct mem_region_device mdev =
	MEM_REGION_DEV_RO_INIT((void *)MVMAP2315_CBFS_BASE, CONFIG_ROM_SIZE);

const struct region_device *boot_device_ro(void)
{
	return &mdev.rdev;
}
