/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2014 - 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <device/device.h>
#include <device/pci.h>
#include <console/console.h>
#include <arch/io.h>
#include <cpu/cpu.h>
#include <cpu/x86/smm.h>
#include <string.h>

#include <soc/iomap.h>
#include <soc/soc_util.h>
#include <soc/pm.h>
#include <soc/smm.h>

/* Save the gpio route register. The settings are committed from
 * southcluster_smm_enable_smi(). */
static uint32_t gpio_route;

void southcluster_smm_save_gpio_route(uint32_t route) { gpio_route = route; }

void southcluster_smm_clear_state(void)
{
	uint32_t smi_en;

	printk(BIOS_DEBUG, "Initializing Southbridge SMI...");
	printk(BIOS_SPEW, " pmbase = 0x%04x\n", get_pmbase());

	smi_en = inl((uint16_t)(get_pmbase() + SMI_EN));
	if (smi_en & APMC_EN) {
		printk(BIOS_INFO, "SMI# handler already enabled?\n");
		return;
	}

	/* Dump and clear status registers */
	clear_smi_status();
	clear_pm1_status();
	clear_tco_status();
	clear_gpe_status();
	clear_pmc_status();
}

void southcluster_smm_enable_smi(void)
{

	printk(BIOS_DEBUG, "Enabling SMIs.\n");
	/* Configure events Disable pcie wake. */
	enable_pm1(PWRBTN_EN | GBL_EN | PCIEXPWAK_DIS);
	disable_gpe(PME_B0_EN);

	/* Enable SMI generation:
	 *  - on APMC writes (io 0xb2)
	 *  - on writes to SLP_EN (sleep states)
	 *  - on writes to GBL_RLS (bios commands)
	 * No SMIs:
	 *  - on TCO events
	 *  - on microcontroller writes (io 0x62/0x66)
	 */
	enable_smi(APMC_EN | SLP_SMI_EN | GBL_SMI_EN | EOS);
}

void smm_setup_structures(void *gnvs, void *tcg, void *smi1)
{
	/*
	 * Issue SMI to set the gnvs pointer in SMM.
	 * tcg and smi1 are unused.
	 *
	 * EAX = APM_CNT_GNVS_UPDATE
	 * EBX = gnvs pointer
	 * EDX = APM_CNT
	 */
	asm volatile("outb %%al, %%dx\n\t"
		     : /* ignore result */
		     : "a"(APM_CNT_GNVS_UPDATE), "b"((uint32_t)gnvs),
		       "d"(APM_CNT));
}
