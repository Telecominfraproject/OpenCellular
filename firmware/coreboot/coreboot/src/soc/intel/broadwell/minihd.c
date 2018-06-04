/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008 Advanced Micro Devices, Inc.
 * Copyright (C) 2008-2009 coresystems GmbH
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

#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <arch/io.h>
#include <delay.h>
#include <stdlib.h>
#include <soc/intel/common/hda_verb.h>
#include <soc/ramstage.h>
#include <soc/igd.h>

static const u32 minihd_verb_table[] = {
	/* coreboot specific header */
	0x80862807,	// Codec Vendor / Device ID: Intel Mini-HD
	0x00000000,	// Subsystem ID
	0x00000004,	// Number of jacks

	/* Enable 3rd Pin and Converter Widget */
	0x00878101,

	/* Pin Widget 5 - PORT B */
	0x00571C10,
	0x00571D00,
	0x00571E56,
	0x00571F18,

	/* Pin Widget 6 - PORT C */
	0x00671C20,
	0x00671D00,
	0x00671E56,
	0x00671F18,

	/* Pin Widget 7 - PORT D */
	0x00771C30,
	0x00771D00,
	0x00771E56,
	0x00771F18,

	/* Disable 3rd Pin and Converter Widget */
	0x00878100,

	/* Dummy entries to fill out the table */
	0x00878100,
	0x00878100,
};

static void minihd_init(struct device *dev)
{
	struct resource *res;
	u8 *base, reg32;
	int codec_mask, i;

	/* Find base address */
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	if (!res)
		return;

	base = res2mmio(res, 0, 0);
	printk(BIOS_DEBUG, "Mini-HD: base = %p\n", base);

	/* Set Bus Master */
	reg32 = pci_read_config32(dev, PCI_COMMAND);
	pci_write_config32(dev, PCI_COMMAND, reg32 | PCI_COMMAND_MASTER);

	/* Mini-HD configuration */
	reg32 = read32(base + 0x100c);
	reg32 &= 0xfffc0000;
	reg32 |= 0x4;
	write32(base + 0x100c, reg32);

	reg32 = read32(base + 0x1010);
	reg32 &= 0xfffc0000;
	reg32 |= 0x4b;
	write32(base + 0x1010, reg32);

	/* Init the codec and write the verb table */
	codec_mask = hda_codec_detect(base);

	if (codec_mask) {
		for (i = 3; i >= 0; i--) {
			if (codec_mask & (1 << i))
				hda_codec_init(base, i,
					       sizeof(minihd_verb_table),
					       minihd_verb_table);
		}
	}

	/* Set EM4/EM5 registers */
	write32(base + 0x0100c, igd_get_reg_em4());
	write32(base + 0x01010, igd_get_reg_em5());
}

static struct device_operations minihd_ops = {
	.read_resources		= &pci_dev_read_resources,
	.set_resources		= &pci_dev_set_resources,
	.enable_resources	= &pci_dev_enable_resources,
	.init			= &minihd_init,
	.ops_pci		= &broadwell_pci_ops,
};

static const unsigned short pci_device_ids[] = {
	0x0a0c, /* Haswell */
	0x160c, /* Broadwell */
	0
};

static const struct pci_driver minihd_driver __pci_driver = {
	.ops	 = &minihd_ops,
	.vendor	 = PCI_VENDOR_ID_INTEL,
	.devices = pci_device_ids,
};
