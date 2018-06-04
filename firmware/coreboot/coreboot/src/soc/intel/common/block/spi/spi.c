/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Intel Corporation
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

#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/spi.h>
#include <intelblocks/fast_spi.h>
#include <intelblocks/gspi.h>
#include <intelblocks/spi.h>
#include <soc/pci_devs.h>
#include <spi-generic.h>

const struct spi_ctrlr_buses spi_ctrlr_bus_map[] = {
	{ .ctrlr = &fast_spi_flash_ctrlr, .bus_start = 0, .bus_end = 0 },
#if !ENV_SMM && IS_ENABLED(CONFIG_SOC_INTEL_COMMON_BLOCK_GSPI)
	{ .ctrlr = &gspi_ctrlr, .bus_start = 1,
	  .bus_end =  1 + (CONFIG_SOC_INTEL_COMMON_BLOCK_GSPI_MAX - 1)},
#endif
};

const size_t spi_ctrlr_bus_map_count = ARRAY_SIZE(spi_ctrlr_bus_map);

static int spi_dev_to_bus(struct device *dev)
{
	return spi_soc_devfn_to_bus(dev->path.pci.devfn);
}

static struct spi_bus_operations spi_bus_ops = {
	.dev_to_bus			= &spi_dev_to_bus,
};

static struct device_operations spi_dev_ops = {
	.read_resources			= &pci_dev_read_resources,
	.set_resources			= &pci_dev_set_resources,
	.enable_resources		= &pci_dev_enable_resources,
	.scan_bus			= &scan_generic_bus,
	.ops_spi_bus			= &spi_bus_ops,
	.ops_pci			= &pci_dev_ops_pci,
};

static const unsigned short pci_device_ids[] = {
	PCI_DEVICE_ID_INTEL_SPT_SPI1,
	PCI_DEVICE_ID_INTEL_SPT_SPI2,
	PCI_DEVICE_ID_INTEL_SPT_SPI3,
	PCI_DEVICE_ID_INTEL_APL_SPI0,
	PCI_DEVICE_ID_INTEL_APL_SPI1,
	PCI_DEVICE_ID_INTEL_APL_SPI2,
	PCI_DEVICE_ID_INTEL_APL_HWSEQ_SPI,
	PCI_DEVICE_ID_INTEL_GLK_SPI0,
	PCI_DEVICE_ID_INTEL_GLK_SPI1,
	PCI_DEVICE_ID_INTEL_GLK_SPI2,
	PCI_DEVICE_ID_INTEL_CNL_SPI0,
	PCI_DEVICE_ID_INTEL_CNL_SPI1,
	PCI_DEVICE_ID_INTEL_CNL_SPI2,
	PCI_DEVICE_ID_INTEL_CNL_HWSEQ_SPI,
	0
};

static const struct pci_driver pch_spi __pci_driver = {
	.ops				= &spi_dev_ops,
	.vendor				= PCI_VENDOR_ID_INTEL,
	.devices			= pci_device_ids,
};
