/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Sage Electronic Engineering, LLC
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

#include <stdint.h>
#include <arch/io.h>
#include <southbridge/amd/cimx/cimx_util.h>
#include "FchPlatform.h"
#include "gpio_ftns.h"

void configure_gpio(uintptr_t base_addr, u32 iomux_gpio, u8 iomux_ftn, u32 gpio, u32 setting)
{
	u8 bdata;
	u8 *memptr;

	memptr = (u8 *)(base_addr + IOMUX_OFFSET + iomux_gpio);
	*memptr = iomux_ftn;

	memptr = (u8 *)(base_addr + GPIO_OFFSET + gpio);
	bdata = *memptr;
	bdata &= 0x07;
	bdata |= setting; /* set direction and data value */
	*memptr = bdata;
}

int get_spd_offset(void)
{
	u8 index = 0;
	/* One SPD file contains all 4 options, determine which index to
	 * read here, then call into the standard routines.
	 */
	u8 *gpio_bank0_ptr = (u8 *)(ACPI_MMIO_BASE + GPIO_BANK0_BASE);
	if (*(gpio_bank0_ptr + (0x40 << 2) + 2) & BIT0) index |= BIT0;
	if (*(gpio_bank0_ptr + (0x41 << 2) + 2) & BIT0) index |= BIT1;

	return index;
}
