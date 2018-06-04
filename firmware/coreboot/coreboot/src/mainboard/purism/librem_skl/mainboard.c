/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Purism SPC
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

#include <smbios.h>
#include <string.h>
#include <cbfs.h>

#define MAX_SERIAL_LENGTH 0x100

const char *smbios_mainboard_serial_number(void)
{
	static char serial_number[MAX_SERIAL_LENGTH + 1] = {0};
	struct cbfsf file;

	if (serial_number[0] != 0)
		return serial_number;

	if (cbfs_boot_locate(&file, "serial_number", NULL) == 0) {
		struct region_device cbfs_region;
		size_t serial_len;

		cbfs_file_data(&cbfs_region, &file);

		serial_len = region_device_sz(&cbfs_region);
		if (serial_len <= MAX_SERIAL_LENGTH) {
			if (rdev_readat(&cbfs_region, serial_number, 0,
					serial_len) == serial_len) {
				serial_number[serial_len] = 0;
				return serial_number;
			}
		}
	}

	strncpy(serial_number, CONFIG_MAINBOARD_SERIAL_NUMBER,
			MAX_SERIAL_LENGTH);

	return serial_number;
}
