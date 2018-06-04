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

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <arch/acpi.h>
#include "mcp55.h"

#if IS_ENABLED(CONFIG_HAVE_ACPI_TABLES)
unsigned long acpi_fill_mcfg(unsigned long current)
{
	/* Not implemented */
	return current;
}
#endif

static struct device_operations ht_ops = {
	.read_resources   = pci_dev_read_resources,
	.set_resources    = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init             = 0,
	.scan_bus         = 0,
	.ops_pci          = &mcp55_pci_ops,
};

static const struct pci_driver ht_driver __pci_driver = {
	.ops	= &ht_ops,
	.vendor	= PCI_VENDOR_ID_NVIDIA,
	.device	= PCI_DEVICE_ID_NVIDIA_MCP55_HT,
};
