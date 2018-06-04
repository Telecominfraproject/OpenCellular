/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Google Inc.
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
#include <soc/intel/common/hda_verb.h>
#include <soc/ramstage.h>

static void codecs_init(uint8_t *base, u32 codec_mask)
{
	int i;

	/* Can support up to 4 codecs */
	for (i = 3; i >= 0; i--) {
		if (codec_mask & (1 << i))
			hda_codec_init(base, i,
				       cim_verb_data_size, cim_verb_data);
	}

	if (pc_beep_verbs_size)
		hda_codec_write(base, pc_beep_verbs_size, pc_beep_verbs);
}

static void hda_init(struct device *dev)
{
	struct resource *res;
	int codec_mask;
	uint8_t *base;

	res = find_resource(dev, PCI_BASE_ADDRESS_0);
	if (!res)
		return;

	base = res2mmio(res, 0, 0);
	if (!base)
		return;

	codec_mask = hda_codec_detect(base);
	if (codec_mask) {
		printk(BIOS_INFO, "HDA: codec_mask = %02x\n", codec_mask);
		codecs_init(base, codec_mask);
	}
}

static struct device_operations hda_ops = {
	.read_resources		= &pci_dev_read_resources,
	.set_resources		= &pci_dev_set_resources,
	.enable_resources	= &pci_dev_enable_resources,
	.init			= &hda_init,
	.ops_pci		= &pci_dev_ops_pci,
};

static const unsigned short pci_device_ids[] = {
	PCI_DEVICE_ID_INTEL_SKL_AUDIO,
	PCI_DEVICE_ID_INTEL_KBL_AUDIO,
	PCI_DEVICE_ID_INTEL_CNL_AUDIO,
	0
};

static const struct pci_driver pch_hda __pci_driver = {
	.ops		= &hda_ops,
	.vendor		= PCI_VENDOR_ID_INTEL,
	.devices	= pci_device_ids,
};
