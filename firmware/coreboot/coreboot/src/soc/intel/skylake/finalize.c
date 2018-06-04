/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 * Copyright (C) 2015 Intel Corporation.
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
#include <bootstate.h>
#include <chip.h>
#include <console/console.h>
#include <console/post_codes.h>
#include <cpu/x86/smm.h>
#include <device/pci.h>
#include <intelblocks/lpc_lib.h>
#include <intelblocks/p2sb.h>
#include <intelblocks/pcr.h>
#include <reg_script.h>
#include <spi-generic.h>
#include <soc/me.h>
#include <soc/p2sb.h>
#include <soc/pci_devs.h>
#include <soc/pcr_ids.h>
#include <soc/pm.h>
#include <soc/smbus.h>
#include <soc/systemagent.h>
#include <soc/thermal.h>
#include <stdlib.h>

#define PSF_BASE_ADDRESS	0xA00
#define PCR_PSFX_T0_SHDW_PCIEN	0x1C
#define PCR_PSFX_T0_SHDW_PCIEN_FUNDIS	(1 << 8)

static void pch_configure_endpoints(struct device *dev, int epmask_id,
	uint32_t mask)
{
	uint32_t reg32;

	reg32 = pci_read_config32(dev, PCH_P2SB_EPMASK(epmask_id));
	pci_write_config32(dev, PCH_P2SB_EPMASK(epmask_id), reg32 | mask);
}

static void disable_sideband_access(struct device *dev)
{
	u8 reg8;
	uint32_t mask;

	/*
	 * Set p2sb PCI offset EPMASK5 C4h [29, 28, 27, 26] to disable Sideband
	 * access for PCI Root Bridge.
	 * Set p2sb PCI offset EPMASK5 C4h [17, 16,10, 1] to disable Sideband
	 * access for MIPI controller.
	 */
	mask = (1 << 29) | (1 << 28) | (1 << 27) | (1 << 26) | (1 << 17) |
			 (1 << 16) | (1 << 10) | (1 << 1);
	pch_configure_endpoints(dev, 5, mask);

	/*
	 * Set p2sb PCI offset EPMASK7 CCh ports E6, E5 (bits 6, 5)
	 * to disable Sideband access for XHCI controller.
	 */
	mask = (1 << 6) | (1 << 5);
	pch_configure_endpoints(dev, 7, mask);

	/* Set the "Endpoint Mask Lock!", P2SB PCI offset E2h bit[1] to 1. */
	reg8 = pci_read_config8(dev, PCH_P2SB_E0 + 2);
	pci_write_config8(dev, PCH_P2SB_E0 + 2, reg8 | (1 << 1));

	/* hide p2sb device */
	p2sb_hide();
}

static void pch_disable_heci(void)
{
	struct device *dev = PCH_DEV_P2SB;

	/*
	 * if p2sb device 1f.1 is not present or hidden in devicetree
	 * p2sb device becomes NULL
	 */
	if (!dev)
		return;

	/* unhide p2sb device */
	p2sb_unhide();

	/* disable heci */
	pcr_or32(PID_PSF1, PSF_BASE_ADDRESS + PCR_PSFX_T0_SHDW_PCIEN,
		PCR_PSFX_T0_SHDW_PCIEN_FUNDIS);

	disable_sideband_access(dev);
}

static void pch_finalize_script(struct device *dev)
{
	uint32_t reg32;
	uint8_t *pmcbase;
	config_t *config;
	u8 reg8;

	/* Display me status before we hide it */
	intel_me_status();

	pmcbase = pmc_mmio_regs();
	config = dev->chip_info;

	/*
	 * Set low maximum temp value used for dynamic thermal sensor
	 * shutdown consideration.
	 *
	 * If Dynamic Thermal Shutdown is enabled then PMC logic shuts down the
	 * thermal sensor when CPU is in a C-state and DTS Temp <= LTT.
	 */
	pch_thermal_configuration();

	/*
	 * Disable ACPI PM timer based on dt policy
	 *
	 * Disabling ACPI PM timer is necessary for XTAL OSC shutdown.
	 * Disabling ACPI PM timer also switches off TCO
	 */

	if (config->PmTimerDisabled) {
		reg8 = read8(pmcbase + PCH_PWRM_ACPI_TMR_CTL);
		reg8 |= (1 << 1);
		write8(pmcbase + PCH_PWRM_ACPI_TMR_CTL, reg8);
	}

	/* Disable XTAL shutdown qualification for low power idle. */
	if (config->s0ix_enable) {
		reg32 = read32(pmcbase + CIR31C);
		reg32 |= XTALSDQDIS;
		write32(pmcbase + CIR31C, reg32);
	}

	/* we should disable Heci1 based on the devicetree policy */
	if (config->HeciEnabled == 0)
		pch_disable_heci();
}

static void soc_lockdown(struct device *dev)
{
	struct soc_intel_skylake_config *config;
	u8 reg8;

	config = dev->chip_info;

	/* Global SMI Lock */
	if (config->LockDownConfigGlobalSmi == 0) {
		reg8 = pci_read_config8(dev, GEN_PMCON_A);
		reg8 |= SMI_LOCK;
		pci_write_config8(dev, GEN_PMCON_A, reg8);
	}
}

static void soc_finalize(void *unused)
{
	struct device *dev;

	dev = PCH_DEV_PMC;

	/* Check if PMC is enabled, else return */
	if (dev == NULL || dev->chip_info == NULL)
		return;

	printk(BIOS_DEBUG, "Finalizing chipset.\n");

	pch_finalize_script(dev);

	soc_lockdown(dev);

	printk(BIOS_DEBUG, "Finalizing SMM.\n");
	outb(APM_CNT_FINALIZE, APM_CNT);

	/* Indicate finalize step with post code */
	post_code(POST_OS_BOOT);
}

BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, soc_finalize, NULL);
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, soc_finalize, NULL);
