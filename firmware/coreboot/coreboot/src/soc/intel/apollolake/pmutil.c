/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015-2016 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define __SIMPLE_DEVICE__

#include <arch/acpi.h>
#include <arch/io.h>
#include <cbmem.h>
#include <console/console.h>
#include <cpu/x86/msr.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <intelblocks/msr.h>
#include <intelblocks/pmclib.h>
#include <intelblocks/rtc.h>
#include <rules.h>
#include <soc/iomap.h>
#include <soc/cpu.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>
#include <timer.h>
#include <security/vboot/vbnv.h>
#include "chip.h"

static uintptr_t read_pmc_mmio_bar(void)
{
	return PMC_BAR0;
}

uintptr_t soc_read_pmc_base(void)
{
	return read_pmc_mmio_bar();
}

const char *const *soc_smi_sts_array(size_t *a)
{
	static const char *const smi_sts_bits[] = {
		[BIOS_SMI_STS] = "BIOS",
		[LEGACY_USB_SMI_STS] = "LEGACY USB",
		[SLP_SMI_STS] = "SLP_SMI",
		[APM_SMI_STS] = "APM",
		[SWSMI_TMR_SMI_STS] = "SWSMI_TMR",
		[FAKE_PM1_SMI_STS] = "PM1",
		[GPIO_SMI_STS] = "GPIO_SMI",
		[GPIO_UNLOCK_SMI_STS] = "GPIO_UNLOCK_SSMI",
		[MC_SMI_STS] = "MCSMI",
		[TCO_SMI_STS] = "TCO",
		[PERIODIC_SMI_STS] = "PERIODIC",
		[SERIRQ_SMI_STS] = "SERIRQ",
		[SMBUS_SMI_STS] = "SMBUS_SMI",
		[XHCI_SMI_STS] = "XHCI",
		[HSMBUS_SMI_STS] = "HOST_SMBUS",
		[SCS_SMI_STS] = "SCS",
		[PCIE_SMI_STS] = "PCI_EXP_SMI",
		[SCC2_SMI_STS] = "SCC2",
		[SPI_SSMI_STS] = "SPI_SSMI",
		[SPI_SMI_STS] = "SPI",
		[PMC_OCP_SMI_STS] = "OCP_CSE",
	};

	*a = ARRAY_SIZE(smi_sts_bits);
	return smi_sts_bits;
}

/*
 * For APL/GLK this check for power button status if nothing else
 * is indicating an SMI and SMIs aren't turned into SCIs.
 * Apparently, there is no PM1 status bit in the SMI status
 * register.  That makes things difficult for
 * determining if the power button caused an SMI.
 */
uint32_t soc_get_smi_status(uint32_t generic_sts)
{
	if (generic_sts == 0 && !(pmc_read_pm1_control() & SCI_EN)) {
		uint16_t pm1_sts = inw(ACPI_BASE_ADDRESS + PM1_STS);

		/* Fake PM1 status bit if power button pressed. */
		if (pm1_sts & PWRBTN_STS)
			generic_sts |= (1 << FAKE_PM1_SMI_STS);
	}

	return generic_sts;
}

const char *const *soc_tco_sts_array(size_t *a)
{
	static const char *const tco_sts_bits[] = {
		[3] = "TIMEOUT",
		[17] = "SECOND_TO",
	};

	*a = ARRAY_SIZE(tco_sts_bits);
	return tco_sts_bits;
}

const char *const *soc_std_gpe_sts_array(size_t *a)
{
	static const char *const gpe_sts_bits[] = {
		[0] = "PCIE_SCI",
		[2] = "SWGPE",
		[3] = "PCIE_WAKE0",
		[4] = "PUNIT",
		[6] = "PCIE_WAKE1",
		[7] = "PCIE_WAKE2",
		[8] = "PCIE_WAKE3",
		[9] = "PCI_EXP",
		[10] = "BATLOW",
		[11] = "CSE_PME",
		[12] = "XDCI_PME",
		[13] = "XHCI_PME",
		[14] = "AVS_PME",
		[15] = "GPIO_TIER1_SCI",
		[16] = "SMB_WAK",
		[17] = "SATA_PME",
	};

	*a = ARRAY_SIZE(gpe_sts_bits);
	return gpe_sts_bits;
}

uint32_t soc_reset_tco_status(void)
{
	uint32_t tco_sts = inl(ACPI_BASE_ADDRESS + TCO_STS);
	uint32_t tco_en = inl(ACPI_BASE_ADDRESS + TCO1_CNT);

	outl(tco_sts, ACPI_BASE_ADDRESS + TCO_STS);
	return tco_sts & tco_en;
}

void soc_clear_pm_registers(uintptr_t pmc_bar)
{
	uint32_t gen_pmcon1;

	gen_pmcon1 = read32((void *)(pmc_bar + GEN_PMCON1));
	/* Clear the status bits. The RPS field is cleared on a 0 write. */
	write32((void *)(pmc_bar + GEN_PMCON1), gen_pmcon1 & ~RPS);
}

void soc_get_gpi_gpe_configs(uint8_t *dw0, uint8_t *dw1, uint8_t *dw2)
{
	DEVTREE_CONST struct soc_intel_apollolake_config *config;

	/* Look up the device in devicetree */
	DEVTREE_CONST struct device *dev = dev_find_slot(0, SA_DEVFN_ROOT);
	if (!dev || !dev->chip_info) {
		printk(BIOS_ERR, "BUG! Could not find SOC devicetree config\n");
		return;
	}
	config = dev->chip_info;

	/* Assign to out variable */
	*dw0 = config->gpe0_dw1;
	*dw1 = config->gpe0_dw2;
	*dw2 = config->gpe0_dw3;
}

void soc_fill_power_state(struct chipset_power_state *ps)
{
	uintptr_t pmc_bar0 = read_pmc_mmio_bar();

	ps->tco_sts = inl(ACPI_BASE_ADDRESS + TCO_STS);
	ps->prsts = read32((void *)(pmc_bar0 + PRSTS));
	ps->gen_pmcon1 = read32((void *)(pmc_bar0 + GEN_PMCON1));
	ps->gen_pmcon2 = read32((void *)(pmc_bar0 + GEN_PMCON2));
	ps->gen_pmcon3 = read32((void *)(pmc_bar0 + GEN_PMCON3));

	printk(BIOS_DEBUG, "prsts: %08x tco_sts: %08x\n",
	       ps->prsts, ps->tco_sts);
	printk(BIOS_DEBUG,
	       "gen_pmcon1: %08x gen_pmcon2: %08x gen_pmcon3: %08x\n",
	       ps->gen_pmcon1, ps->gen_pmcon2, ps->gen_pmcon3);
}

/* Return 0, 3, or 5 to indicate the previous sleep state. */
int soc_prev_sleep_state(const struct chipset_power_state *ps,
	int prev_sleep_state)
{
	/* WAK_STS bit will not be set when waking from G3 state */

	if (!(ps->pm1_sts & WAK_STS) && (ps->gen_pmcon1 & COLD_BOOT_STS))
		prev_sleep_state = ACPI_S5;
	return prev_sleep_state;
}

void enable_pm_timer_emulation(void)
{
	/* ACPI PM timer emulation */
	msr_t msr;
	/*
	 * The derived frequency is calculated as follows:
	 *    (CTC_FREQ * msr[63:32]) >> 32 = target frequency.
	 * Back solve the multiplier so the 3.579545MHz ACPI timer
	 * frequency is used.
	 */
	msr.hi = (3579545ULL << 32) / CTC_FREQ;
	/* Set PM1 timer IO port and enable */
	msr.lo = EMULATE_PM_TMR_EN | (ACPI_BASE_ADDRESS + R_ACPI_PM1_TMR);
	wrmsr(MSR_EMULATE_PM_TMR, msr);
}

static int rtc_failed(uint32_t gen_pmcon1)
{
	return !!(gen_pmcon1 & RPS);
}

int soc_get_rtc_failed(void)
{
	const struct chipset_power_state *ps = cbmem_find(CBMEM_ID_POWER_STATE);

	if (!ps) {
		printk(BIOS_ERR, "Could not find power state in cbmem, RTC init aborted\n");
		return 1;
	}

	return rtc_failed(ps->gen_pmcon1);
}

int vbnv_cmos_failed(void)
{
	uintptr_t pmc_bar = read_pmc_mmio_bar();
	uint32_t gen_pmcon1 = read32((void *)(pmc_bar + GEN_PMCON1));
	int rtc_failure = rtc_failed(gen_pmcon1);

	if (rtc_failure) {
		printk(BIOS_INFO, "RTC failed!\n");

		/* We do not want to write 1 to clear-1 bits. Set them to 0. */
		gen_pmcon1 &= ~GEN_PMCON1_CLR1_BITS;

		/* RPS is write 0 to clear. */
		gen_pmcon1 &= ~RPS;

		write32((void *)(pmc_bar + GEN_PMCON1), gen_pmcon1);
	}

	return rtc_failure;
}
