/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
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

#include <arch/io.h>
#include <cbmem.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <reg_script.h>

#include <soc/iomap.h>
#include <soc/iosf.h>
#include <soc/lpc.h>
#include <soc/nvs.h>
#include <soc/pattrs.h>
#include <soc/pci_devs.h>
#include <soc/pmc.h>
#include <soc/ramstage.h>
#include "chip.h"


/* The LPE audio devices needs 1MiB of memory reserved aligned to a 512MiB
 * address. Just take 1MiB @ 512MiB. */
#define FIRMWARE_PHYS_BASE (512 << 20)
#define FIRMWARE_PHYS_LENGTH (1 << 20)
#define FIRMWARE_PCI_REG_BASE 0xa8
#define FIRMWARE_PCI_REG_LENGTH 0xac
#define FIRMWARE_REG_BASE_C0 0x144000
#define FIRMWARE_REG_LENGTH_C0 (FIRMWARE_REG_BASE_C0 + 4)

static void assign_device_nvs(struct device *dev, u32 *field, unsigned index)
{
	struct resource *res;

	res = find_resource(dev, index);
	if (res)
		*field = res->base;
}

static void lpe_enable_acpi_mode(struct device *dev)
{
	static const struct reg_script ops[] = {
		/* Disable PCI interrupt, enable Memory and Bus Master */
		REG_PCI_OR32(PCI_COMMAND,
			     PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER | (1<<10)),
		/* Enable ACPI mode */
		REG_IOSF_OR(IOSF_PORT_0x58, LPE_PCICFGCTR1,
			    LPE_PCICFGCTR1_PCI_CFG_DIS |
			    LPE_PCICFGCTR1_ACPI_INT_EN),
		REG_SCRIPT_END
	};
	global_nvs_t *gnvs;

	/* Find ACPI NVS to update BARs */
	gnvs = (global_nvs_t *)cbmem_find(CBMEM_ID_ACPI_GNVS);
	if (!gnvs) {
		printk(BIOS_ERR, "Unable to locate Global NVS\n");
		return;
	}

	/* Save BAR0, BAR1, and firmware base  to ACPI NVS */
	assign_device_nvs(dev, &gnvs->dev.lpe_bar0, PCI_BASE_ADDRESS_0);
	assign_device_nvs(dev, &gnvs->dev.lpe_bar1, PCI_BASE_ADDRESS_1);
	assign_device_nvs(dev, &gnvs->dev.lpe_fw, FIRMWARE_PCI_REG_BASE);

	/* Device is enabled in ACPI mode */
	gnvs->dev.lpe_en = 1;

	/* Put device in ACPI mode */
	reg_script_run_on_dev(dev, ops);
}

static void setup_codec_clock(struct device *dev)
{
	uint32_t reg;
	u32 *clk_reg;
	struct soc_intel_fsp_baytrail_config *config;
	const char *freq_str;

	config = dev->chip_info;
	switch (config->lpe_codec_clk_freq) {
	case 19:
		freq_str = "19.2";
		reg = CLK_FREQ_19P2MHZ;
		break;
	case 25:
		freq_str = "25";
		reg = CLK_FREQ_25MHZ;
		break;
	default:
		printk(BIOS_DEBUG, "LPE codec clock not required.\n");
		return;
	}

	/* Default to always running. */
	reg |= CLK_CTL_ON;

	if (config->lpe_codec_clk_num < 0 || config->lpe_codec_clk_num > 5) {
		printk(BIOS_DEBUG, "Invalid LPE codec clock number.\n");
		return;
	}

	printk(BIOS_DEBUG, "LPE Audio codec clock set to %sMHz.\n", freq_str);

	clk_reg = (u32 *)(PMC_BASE_ADDRESS + PLT_CLK_CTL_0);
	clk_reg += config->lpe_codec_clk_num;

	write32(clk_reg, (read32(clk_reg) & ~0x7) | reg);
}

static void lpe_stash_firmware_info(struct device *dev)
{
	struct resource *res;
	struct resource *mmio;
	const struct pattrs *pattrs = pattrs_get();

	res = find_resource(dev, FIRMWARE_PCI_REG_BASE);
	if (res == NULL) {
		printk(BIOS_DEBUG, "LPE Firmware memory not found.\n");
		return;
	}

	/* Continue using old way of informing firmware address / size. */
	pci_write_config32(dev, FIRMWARE_PCI_REG_BASE, res->base);
	pci_write_config32(dev, FIRMWARE_PCI_REG_LENGTH, res->size);

	/* C0 and later steppings use an offset in the MMIO space. */
	if (pattrs->stepping >= STEP_C0) {
		mmio = find_resource(dev, PCI_BASE_ADDRESS_0);
		write32((u32 *)(uintptr_t)(mmio->base + FIRMWARE_REG_BASE_C0),
			res->base);
		write32((u32 *)(uintptr_t)(mmio->base + FIRMWARE_REG_LENGTH_C0),
			res->size);
	}
}

static void lpe_init(struct device *dev)
{
	struct soc_intel_fsp_baytrail_config *config = dev->chip_info;

	lpe_stash_firmware_info(dev);

	setup_codec_clock(dev);

	if (config->LpeAcpiModeEnable == LPE_ACPI_MODE_ENABLED)
		lpe_enable_acpi_mode(dev);
}

static void lpe_read_resources(struct device *dev)
{
	pci_dev_read_resources(dev);

	reserved_ram_resource(dev, FIRMWARE_PCI_REG_BASE,
			      FIRMWARE_PHYS_BASE >> 10,
			      FIRMWARE_PHYS_LENGTH >> 10);
}

static const struct device_operations device_ops = {
	.read_resources		= lpe_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= lpe_init,
	.enable			= NULL,
	.scan_bus		= NULL,
	.ops_pci		= &soc_pci_ops,
};

static const struct pci_driver southcluster __pci_driver = {
	.ops		= &device_ops,
	.vendor		= PCI_VENDOR_ID_INTEL,
	.device		= LPE_DEVID,
};
