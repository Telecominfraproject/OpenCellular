/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
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
 */

#include <types.h>
#include <arch/io.h>
#include <console/console.h>
#include <cpu/x86/cache.h>
#include <device/pci_def.h>
#include <cpu/x86/smm.h>
#include <elog.h>
#include <halt.h>
#include <pc80/mc146818rtc.h>
#include <southbridge/intel/common/rcba.h>
#include "pch.h"

#include "nvs.h"

#include <northbridge/intel/sandybridge/sandybridge.h>
#include <arch/io.h>
#include <southbridge/intel/bd82x6x/me.h>
#include <southbridge/intel/common/gpio.h>
#include <cpu/intel/model_206ax/model_206ax.h>
#include <southbridge/intel/common/pmutil.h>

static global_nvs_t *gnvs;
global_nvs_t *smm_get_gnvs(void)
{
	return gnvs;
}

int southbridge_io_trap_handler(int smif)
{
	switch (smif) {
	case 0x32:
		printk(BIOS_DEBUG, "OS Init\n");
		/* gnvs->smif:
		 *  On success, the IO Trap Handler returns 0
		 *  On failure, the IO Trap Handler returns a value != 0
		 */
		gnvs->smif = 0;
		return 1; /* IO trap handled */
	}

	/* Not handled */
	return 0;
}

static void southbridge_gate_memory_reset_real(int offset,
					       u16 use, u16 io, u16 lvl)
{
	u32 reg32;

	/* Make sure it is set as GPIO */
	reg32 = inl(use);
	if (!(reg32 & (1 << offset))) {
		reg32 |= (1 << offset);
		outl(reg32, use);
	}

	/* Make sure it is set as output */
	reg32 = inl(io);
	if (reg32 & (1 << offset)) {
		reg32 &= ~(1 << offset);
		outl(reg32, io);
	}

	/* Drive the output low */
	reg32 = inl(lvl);
	reg32 &= ~(1 << offset);
	outl(reg32, lvl);
}

/*
 * Drive GPIO 60 low to gate memory reset in S3.
 *
 * Intel reference designs all use GPIO 60 but it is
 * not a requirement and boards could use a different pin.
 */
void southbridge_gate_memory_reset(void)
{
	u16 gpiobase;

	gpiobase = pci_read_config16(PCI_DEV(0, 0x1f, 0), GPIOBASE) & 0xfffc;
	if (!gpiobase)
		return;

	if (CONFIG_DRAM_RESET_GATE_GPIO >= 32)
		southbridge_gate_memory_reset_real(CONFIG_DRAM_RESET_GATE_GPIO - 32,
						   gpiobase + GPIO_USE_SEL2,
						   gpiobase + GP_IO_SEL2,
						   gpiobase + GP_LVL2);
	else
		southbridge_gate_memory_reset_real(CONFIG_DRAM_RESET_GATE_GPIO,
						   gpiobase + GPIO_USE_SEL,
						   gpiobase + GP_IO_SEL,
						   gpiobase + GP_LVL);
}

static void xhci_sleep(u8 slp_typ)
{
	u32 reg32, xhci_bar;
	u16 reg16;

	switch (slp_typ) {
	case ACPI_S3:
	case ACPI_S4:
		reg16 = pci_read_config16(PCH_XHCI_DEV, 0x74);
		reg16 &= ~0x03UL;
		pci_write_config32(PCH_XHCI_DEV, 0x74, reg16);

		reg32 = pci_read_config32(PCH_XHCI_DEV, PCI_COMMAND);
		reg32 |= (PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);
		pci_write_config32(PCH_XHCI_DEV, PCI_COMMAND, reg32);

		xhci_bar = pci_read_config32(PCH_XHCI_DEV,
				              PCI_BASE_ADDRESS_0) & ~0xFUL;

		if ((xhci_bar + 0x4C0) & 1)
			pch_iobp_update(0xEC000082, ~0UL, (3 << 2));
		if ((xhci_bar + 0x4D0) & 1)
			pch_iobp_update(0xEC000182, ~0UL, (3 << 2));
		if ((xhci_bar + 0x4E0) & 1)
			pch_iobp_update(0xEC000282, ~0UL, (3 << 2));
		if ((xhci_bar + 0x4F0) & 1)
			pch_iobp_update(0xEC000382, ~0UL, (3 << 2));

		reg32 = pci_read_config32(PCH_XHCI_DEV, PCI_COMMAND);
		reg32 &= ~(PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);
		pci_write_config32(PCH_XHCI_DEV, PCI_COMMAND, reg32);

		reg16 = pci_read_config16(PCH_XHCI_DEV, 0x74);
		reg16 |= 0x03;
		pci_write_config16(PCH_XHCI_DEV, 0x74, reg16);
		break;

	case ACPI_S5:
		reg16 = pci_read_config16(PCH_XHCI_DEV, 0x74);
		reg16 |= ((1 << 8) | 0x03);
		pci_write_config16(PCH_XHCI_DEV, 0x74, reg16);
		break;
	}
}

void southbridge_smi_monitor(void)
{
#define IOTRAP(x) (trap_sts & (1 << x))
	u32 trap_sts, trap_cycle;
	u32 data, mask = 0;
	int i;

	trap_sts = RCBA32(0x1e00); // TRSR - Trap Status Register
	RCBA32(0x1e00) = trap_sts; // Clear trap(s) in TRSR

	trap_cycle = RCBA32(0x1e10);
	for (i=16; i<20; i++) {
		if (trap_cycle & (1 << i))
			mask |= (0xff << ((i - 16) << 2));
	}


	/* IOTRAP(3) SMI function call */
	if (IOTRAP(3)) {
		if (gnvs && gnvs->smif)
			io_trap_handler(gnvs->smif); // call function smif
		return;
	}

	/* IOTRAP(2) currently unused
	 * IOTRAP(1) currently unused */

	/* IOTRAP(0) SMIC */
	if (IOTRAP(0)) {
		if (!(trap_cycle & (1 << 24))) { // It's a write
			printk(BIOS_DEBUG, "SMI1 command\n");
			data = RCBA32(0x1e18);
			data &= mask;
			// if (smi1)
			// 	southbridge_smi_command(data);
			// return;
		}
		// Fall through to debug
	}

	printk(BIOS_DEBUG, "  trapped io address = 0x%x\n", trap_cycle & 0xfffc);
	for (i=0; i < 4; i++) if (IOTRAP(i)) printk(BIOS_DEBUG, "  TRAP = %d\n", i);
	printk(BIOS_DEBUG, "  AHBE = %x\n", (trap_cycle >> 16) & 0xf);
	printk(BIOS_DEBUG, "  MASK = 0x%08x\n", mask);
	printk(BIOS_DEBUG, "  read/write: %s\n", (trap_cycle & (1 << 24)) ? "read" : "write");

	if (!(trap_cycle & (1 << 24))) {
		/* Write Cycle */
		data = RCBA32(0x1e18);
		printk(BIOS_DEBUG, "  iotrap written data = 0x%08x\n", data);
	}
#undef IOTRAP
}

void southbridge_smm_xhci_sleep(u8 slp_type)
{
	if (smm_get_gnvs()->xhci)
		xhci_sleep(slp_type);
}

void southbridge_update_gnvs(u8 apm_cnt, int *smm_done)
{
	em64t101_smm_state_save_area_t *state =
		smi_apmc_find_state_save(apm_cnt);
	if (state) {
		/* EBX in the state save contains the GNVS pointer */
		gnvs = (global_nvs_t *)((u32)state->rbx);
		*smm_done = 1;
		printk(BIOS_DEBUG, "SMI#: Setting GNVS to %p\n", gnvs);
	}
}

void southbridge_finalize_all(void)
{
	intel_me_finalize_smm();
	intel_pch_finalize_smm();
	intel_sandybridge_finalize_smm();
	intel_model_206ax_finalize_smm();
}
