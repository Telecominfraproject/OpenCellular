/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2004 Tyan Computer
 * Written by Yinghai Lu <yhlu@tyan.com> for Tyan Computer.
 * Copyright (C) 2006,2007 AMD
 * Written by Yinghai Lu <yinghai.lu@amd.com> for AMD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <arch/io.h>
#include "mcp55.h"

static void mcp55_enable_rom(void)
{
	u8 byte;
	u16 word;
	pci_devfn_t addr;

	/* Enable 4MB ROM access at 0xFFC00000 - 0xFFFFFFFF. */

	addr = PCI_DEV(0, (MCP55_DEVN_BASE + 1), 0);

	/* Set the 15MB enable bits. */
	byte = pci_read_config8(addr, 0x88);
	byte |= 0xff; /* 256K */
	pci_write_config8(addr, 0x88, byte);
	byte = pci_read_config8(addr, 0x8c);
	byte |= 0xff; /* 1M */
	pci_write_config8(addr, 0x8c, byte);
	word = pci_read_config16(addr, 0x90);
	word |= 0x7fff; /* 15M */
	pci_write_config16(addr, 0x90, word);
}

static void bootblock_southbridge_init(void)
{
	mcp55_enable_rom();
}
