/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012-2017 Advanced Micro Devices, Inc.
 * Copyright (C) 2014 Google Inc.
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

#include <cbmem.h>
#include <stage_cache.h>
#include <mrc_cache.h>
#include <console/console.h>
#include <amdblocks/s3_resume.h>

/* Training data versioning is not supported or tracked. */
#define DEFAULT_MRC_VERSION 0

void get_s3nv_info(void **base, size_t *size)
{
	struct region_device rdev;

	if (mrc_cache_get_current(MRC_TRAINING_DATA, DEFAULT_MRC_VERSION,
					&rdev)) {
		printk(BIOS_ERR, "mrc_cache_get_current returned error\n");
		return;
	}
	*base = rdev_mmap_full(&rdev);
	*size = region_device_sz(&rdev);
	if (!*base || !*size)
		printk(BIOS_ERR, "Error: S3 NV data not found\n");
	else
		printk(BIOS_SPEW, "S3 NV data @0x%p 0x%0zx total bytes\n",
							*base, *size);
}

void get_s3vol_info(void **base, size_t *size)
{
	stage_cache_get_raw(STAGE_S3_DATA, base, size);
	if (!*base || !*size)
		printk(BIOS_ERR, "Error: S3 volatile data not found\n");
	else
		printk(BIOS_SPEW, "S3 volatile data @0x%p 0x%0zx total bytes\n",
							*base, *size);
}

int save_s3_info(void *nv_base, size_t nv_size, void *vol_base, size_t vol_size)
{
	if (mrc_cache_stash_data(MRC_TRAINING_DATA, DEFAULT_MRC_VERSION,
				nv_base, nv_size) < 0) {
		printk(BIOS_ERR, "Failed to stash MRC data\n");
		return -1;
	}

	stage_cache_add_raw(STAGE_S3_DATA, vol_base, vol_size);
	return 0;
}
