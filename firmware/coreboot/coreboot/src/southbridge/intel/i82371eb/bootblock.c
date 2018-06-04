/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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
#include <device/pci_ids.h>
#include "i82371eb.h"

static void bootblock_southbridge_init(void)
{
	u16 reg16;
	pci_devfn_t dev;

	/*
	 * Note: The Intel 82371AB/EB/MB ISA device can be on different
	 * PCI bus:device.function locations on different boards.
	 * Examples we encountered: 00:07.0, 00:04.0, or 00:14.0.
	 * But scanning for the PCI IDs (instead of hardcoding
	 * bus/device/function numbers) works on all boards.
	 */
	dev = pci_locate_device(PCI_ID(PCI_VENDOR_ID_INTEL,
				       PCI_DEVICE_ID_INTEL_82371AB_ISA), 0);

	/* Enable access to the whole ROM, disable ROM write access. */
	reg16 = pci_read_config16(dev, XBCS);
	reg16 |= LOWER_BIOS_ENABLE | EXT_BIOS_ENABLE | EXT_BIOS_ENABLE_1MB;
	reg16 &= ~(WRITE_PROTECT_ENABLE);	/* Disable ROM write access. */
	pci_write_config16(dev, XBCS, reg16);
}
