/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015-2017 Intel Corp.
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
#include <console/console.h>
#include <cpu/x86/smm.h>
#include <intelblocks/fast_spi.h>
#include <intelblocks/pcr.h>
#include <intelblocks/smihandler.h>
#include <soc/gpio.h>
#include <soc/iomap.h>
#include <soc/nvs.h>
#include <soc/pcr_ids.h>
#include <soc/pm.h>
#include <soc/pmc.h>

/* IO Trap PCRs */
/* Trap status Register */
#define PCR_PSTH_TRPST  0x1E00
/* Trapped cycle */
#define PCR_PSTH_TRPC   0x1E10
/* Trapped write data */
#define PCR_PSTH_TRPD   0x1E18

/* Inherited from cpu/x86/smm.h resulting in a different signature */
int southbridge_io_trap_handler(int smif)
{
	global_nvs_t *gnvs = smm_get_gnvs();
	switch (smif) {
	case 0x32:
		printk(BIOS_DEBUG, "OS Init\n");
		/*
		 * gnvs->smif:
		 * - On success, the IO Trap Handler returns 0
		 * - On failure, the IO Trap Handler returns a value != 0
		 */
		gnvs->smif = 0;
		return 1; /* IO trap handled */
	}

	/* Not handled */
	return 0;
}

void smihandler_southbridge_mc(
	const struct smm_save_state_ops *save_state_ops)
{
	u32 reg32 = inl(ACPI_BASE_ADDRESS + SMI_EN);

	/* Are microcontroller SMIs enabled? */
	if ((reg32 & MCSMI_EN) == 0)
		return;

	printk(BIOS_DEBUG, "Microcontroller SMI.\n");
}

void smihandler_southbridge_monitor(
	const struct smm_save_state_ops *save_state_ops)
{
#define IOTRAP(x) (trap_sts & (1 << x))
	u32 trap_cycle;
	u32 data, mask = 0;
	u8 trap_sts;
	int i;
	global_nvs_t *gnvs = smm_get_gnvs();

	/* TRSR - Trap Status Register */
	trap_sts = pcr_read8(PID_PSTH, PCR_PSTH_TRPST);
	/* Clear trap(s) in TRSR */
	pcr_write8(PID_PSTH, PCR_PSTH_TRPST, trap_sts);

	/* TRPC - Trapped cycle */
	trap_cycle = pcr_read32(PID_PSTH, PCR_PSTH_TRPC);
	for (i = 16; i < 20; i++) {
		if (trap_cycle & (1 << i))
			mask |= (0xff << ((i - 16) << 2));
	}

	/* IOTRAP(3) SMI function call */
	if (IOTRAP(3)) {
		if (gnvs && gnvs->smif)
			io_trap_handler(gnvs->smif);
		return;
	}

	/*
	 * IOTRAP(2) currently unused
	 * IOTRAP(1) currently unused
	 */

	/* IOTRAP(0) SMIC */
	if (IOTRAP(0)) {
		if (!(trap_cycle & (1 << 24))) { /* It's a write */
			printk(BIOS_DEBUG, "SMI1 command\n");
			/* Trapped write data */
			data = pcr_read32(PID_PSTH, PCR_PSTH_TRPD);
			data &= mask;
		}
	}

	printk(BIOS_DEBUG, "  trapped io address = 0x%x\n",
		trap_cycle & 0xfffc);
	for (i = 0; i < 4; i++)
		if (IOTRAP(i))
			printk(BIOS_DEBUG, "  TRAP = %d\n", i);
	printk(BIOS_DEBUG, "  AHBE = %x\n", (trap_cycle >> 16) & 0xf);
	printk(BIOS_DEBUG, "  MASK = 0x%08x\n", mask);
	printk(BIOS_DEBUG, "  read/write: %s\n",
		(trap_cycle & (1 << 24)) ? "read" : "write");

	if (!(trap_cycle & (1 << 24))) {
		/* Write Cycle */
		data = pcr_read32(PID_PSTH, PCR_PSTH_TRPD);
		printk(BIOS_DEBUG, "  iotrap written data = 0x%08x\n", data);
	}
#undef IOTRAP
}
