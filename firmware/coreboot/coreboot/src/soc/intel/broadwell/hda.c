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
#include <device/azalia_device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <arch/io.h>
#include <delay.h>
#include <soc/intel/common/hda_verb.h>
#include <soc/pch.h>
#include <soc/ramstage.h>
#include <soc/rcba.h>

static void codecs_init(u8 *base, u32 codec_mask)
{
	int i;

	/* Can support up to 4 codecs */
	for (i = 3; i >= 0; i--) {
		if (codec_mask & (1 << i))
			hda_codec_init(base, i,
				       cim_verb_data_size,
				       cim_verb_data);
	}

	if (pc_beep_verbs_size)
		hda_codec_write(base, pc_beep_verbs_size, pc_beep_verbs);
}

static void hda_pch_init(struct device *dev, u8 *base)
{
	u8 reg8;
	u16 reg16;
	u32 reg32;

	if (RCBA32(0x2030) & (1 << 31)) {
		reg32 = pci_read_config32(dev, 0x120);
		reg32 &= 0xf8ffff01;
		reg32 |= (1 << 25);
		reg32 |= RCBA32(0x2030) & 0xfe;
		pci_write_config32(dev, 0x120, reg32);
	} else
		printk(BIOS_DEBUG, "HDA: V1CTL disabled.\n");

	reg32 = pci_read_config32(dev, 0x114);
	reg32 &= ~0xfe;
	pci_write_config32(dev, 0x114, reg32);

	// Set VCi enable bit
	if (pci_read_config32(dev, 0x120) & ((1 << 24) |
					     (1 << 25) | (1 << 26))) {
		reg32 = pci_read_config32(dev, 0x120);
		reg32 &= ~(1 << 31);
		pci_write_config32(dev, 0x120, reg32);
	}

	/* Additional programming steps */
	reg32 = pci_read_config32(dev, 0xc4);
	reg32 |= (1 << 24);
	pci_write_config32(dev, 0xc4, reg32);

	reg8 = pci_read_config8(dev, 0x40); // Audio Control
	reg8 |= 1; // Select HDA mode
	pci_write_config8(dev, 0x40, reg8);

	reg8 = pci_read_config8(dev, 0x4d); // Docking Status
	reg8 &= ~(1 << 7); // Docking not supported
	pci_write_config8(dev, 0x4d, reg8);

	reg16 = read32(base + 0x0012);
	reg16 |= (1 << 0);
	write32(base + 0x0012, reg16);

	/* disable Auto Voltage Detector */
	reg8 = pci_read_config8(dev, 0x42);
	reg8 |= (1 << 2);
	pci_write_config8(dev, 0x42, reg8);
}

static void hda_init(struct device *dev)
{
	u8 *base;
	struct resource *res;
	u32 codec_mask;
	u32 reg32;

	/* Find base address */
	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	if (!res)
		return;

	base = res2mmio(res, 0, 0);
	printk(BIOS_DEBUG, "HDA: base = %p\n", base);

	/* Set Bus Master */
	reg32 = pci_read_config32(dev, PCI_COMMAND);
	pci_write_config32(dev, PCI_COMMAND, reg32 | PCI_COMMAND_MASTER);

	hda_pch_init(dev, base);

	codec_mask = hda_codec_detect(base);

	if (codec_mask) {
		printk(BIOS_DEBUG, "HDA: codec_mask = %02x\n", codec_mask);
		codecs_init(base, codec_mask);
	}
}

static void hda_enable(struct device *dev)
{
	u32 reg32;
	u8 reg8;

	reg8 = pci_read_config8(dev, 0x43);
	reg8 |= 0x6f;
	pci_write_config8(dev, 0x43, reg8);

	if (!dev->enabled) {
		/* Route I/O buffers to ADSP function */
		reg8 = pci_read_config8(dev, 0x42);
		reg8 |= (1 << 7) | (1 << 6);
		pci_write_config8(dev, 0x42, reg8);

		printk(BIOS_INFO, "HDA disabled, I/O buffers routed to ADSP\n");

		/* Ensure memory, io, and bus master are all disabled */
		reg32 = pci_read_config32(dev, PCI_COMMAND);
		reg32 &= ~(PCI_COMMAND_MASTER |
			   PCI_COMMAND_MEMORY | PCI_COMMAND_IO);
		pci_write_config32(dev, PCI_COMMAND, reg32);

		/* Disable this device */
		pch_disable_devfn(dev);
	}
}

static struct device_operations hda_ops = {
	.read_resources		= &pci_dev_read_resources,
	.set_resources		= &pci_dev_set_resources,
	.enable_resources	= &pci_dev_enable_resources,
	.init			= &hda_init,
	.enable			= &hda_enable,
	.ops_pci		= &broadwell_pci_ops,
};

static const unsigned short pci_device_ids[] = {
	0x9c20, /* LynxPoint-LP */
	0x9ca0, /* WildcatPoint */
	0
};

static const struct pci_driver pch_hda __pci_driver = {
	.ops	 = &hda_ops,
	.vendor	 = PCI_VENDOR_ID_INTEL,
	.devices = pci_device_ids,
};
