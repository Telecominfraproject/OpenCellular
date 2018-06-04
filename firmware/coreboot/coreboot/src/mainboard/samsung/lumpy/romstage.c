/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2010 coresystems GmbH
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
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

#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <timestamp.h>
#include <arch/io.h>
#include <device/pci_def.h>
#include <device/pnp_def.h>
#include <cpu/x86/lapic.h>
#include <pc80/mc146818rtc.h>
#include <cbfs.h>
#include <arch/acpi.h>
#include <cbmem.h>
#include <console/console.h>
#include <bootmode.h>
#include <security/tpm/tis.h>
#include <northbridge/intel/sandybridge/sandybridge.h>
#include <northbridge/intel/sandybridge/raminit.h>
#include <northbridge/intel/sandybridge/raminit_native.h>
#include <southbridge/intel/common/rcba.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/common/gpio.h>
#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <halt.h>
#include "option_table.h"
#if IS_ENABLED(CONFIG_DRIVERS_UART_8250IO)
#include <superio/smsc/lpc47n207/lpc47n207.h>
#endif

void pch_enable_lpc(void)
{
	/* Set COM1/COM2 decode range */
	pci_write_config16(PCH_LPC_DEV, LPC_IO_DEC, 0x0010);

#if IS_ENABLED(CONFIG_DRIVERS_UART_8250IO)
	/* Enable SuperIO + EC + KBC + COM1 + lpc47n207 config*/
	pci_write_config16(PCH_LPC_DEV, LPC_EN, CNF1_LPC_EN | MC_LPC_EN |
		KBC_LPC_EN | CNF2_LPC_EN | COMA_LPC_EN);

	/* map full 256 bytes at 0x1600 to the LPC bus */
	pci_write_config32(PCH_LPC_DEV, LPC_GEN1_DEC, 0xfc1601);

	try_enabling_LPC47N207_uart();
#else
	/* Enable SuperIO + EC + KBC */
	pci_write_config16(PCH_LPC_DEV, LPC_EN, CNF1_LPC_EN | MC_LPC_EN |
		KBC_LPC_EN);
#endif
}

void mainboard_rcba_config(void)
{
	/*
	 *             GFX    INTA -> PIRQA (MSI)
	 * D28IP_P1IP  WLAN   INTA -> PIRQB
	 * D28IP_P4IP  ETH0   INTB -> PIRQC (MSI)
	 * D29IP_E1P   EHCI1  INTA -> PIRQD
	 * D26IP_E2P   EHCI2  INTA -> PIRQB
	 * D31IP_SIP   SATA   INTA -> PIRQA (MSI)
	 * D31IP_SMIP  SMBUS  INTC -> PIRQH
	 * D31IP_TTIP  THRT   INTB -> PIRQG
	 * D27IP_ZIP   HDA    INTA -> PIRQG (MSI)
	 *
	 * LIGHTSENSOR             -> PIRQE (Edge Triggered)
	 * TRACKPAD                -> PIRQF (Edge Triggered)
	 */

	/* Device interrupt pin register (board specific) */
	RCBA32(D31IP) = (INTB << D31IP_TTIP) | (NOINT << D31IP_SIP2) |
		(INTC << D31IP_SMIP) | (INTA << D31IP_SIP);
	RCBA32(D30IP) = (NOINT << D30IP_PIP);
	RCBA32(D29IP) = (INTA << D29IP_E1P);
	RCBA32(D28IP) = (INTA << D28IP_P1IP) | (INTC << D28IP_P3IP) |
		(INTB << D28IP_P4IP);
	RCBA32(D27IP) = (INTA << D27IP_ZIP);
	RCBA32(D26IP) = (INTA << D26IP_E2P);
	RCBA32(D25IP) = (NOINT << D25IP_LIP);
	RCBA32(D22IP) = (NOINT << D22IP_MEI1IP);

	/* Device interrupt route registers */
	DIR_ROUTE(D31IR, PIRQA, PIRQG, PIRQH, PIRQB);
	DIR_ROUTE(D29IR, PIRQD, PIRQE, PIRQG, PIRQH);
	DIR_ROUTE(D28IR, PIRQB, PIRQC, PIRQD, PIRQE);
	DIR_ROUTE(D27IR, PIRQG, PIRQH, PIRQA, PIRQB);
	DIR_ROUTE(D26IR, PIRQB, PIRQC, PIRQD, PIRQA);
	DIR_ROUTE(D25IR, PIRQA, PIRQB, PIRQC, PIRQD);
	DIR_ROUTE(D22IR, PIRQA, PIRQB, PIRQC, PIRQD);

	/* Enable IOAPIC (generic) */
	RCBA16(EOIC) = 0x0100;
	/* PCH BWG says to read back the IOAPIC enable register */
	(void) RCBA16(EOIC);
}

static const uint8_t *locate_spd(void)
{
	typedef const uint8_t spd_blob[256];
	spd_blob *spd_data;
	size_t spd_file_len;

	u32 gp_lvl2 = inl(DEFAULT_GPIOBASE + 0x38);
	u8 gpio33, gpio41, gpio49;
	gpio33 = (gp_lvl2 >> (33-32)) & 1;
	gpio41 = (gp_lvl2 >> (41-32)) & 1;
	gpio49 = (gp_lvl2 >> (49-32)) & 1;
	printk(BIOS_DEBUG, "Memory Straps:\n");
	printk(BIOS_DEBUG, " - memory capacity %dGB\n",
		gpio33 ? 2 : 1);
	printk(BIOS_DEBUG, " - die revision %d\n",
		gpio41 ? 2 : 1);
	printk(BIOS_DEBUG, " - vendor %s\n",
		gpio49 ? "Samsung" : "Other");

	int spd_index = 0;

	switch ((gpio49 << 2) | (gpio41 << 1) | gpio33) {
	case 0: // Other 1G Rev 1
		spd_index = 0;
		break;
	case 2: // Other 1G Rev 2
		spd_index = 1;
		break;
	case 1: // Other 2G Rev 1
	case 3: // Other 2G Rev 2
		spd_index = 2;
		break;
	case 4: // Samsung 1G Rev 1
		spd_index = 3;
		break;
	case 6: // Samsung 1G Rev 2
		spd_index = 4;
		break;
	case 5: // Samsung 2G Rev 1
	case 7: // Samsung 2G Rev 2
		spd_index = 5;
		break;
	}

	spd_data = cbfs_boot_map_with_leak("spd.bin", CBFS_TYPE_SPD,
						&spd_file_len);
	if (!spd_data)
		die("SPD data not found.");
	if (spd_file_len < (spd_index + 1) * 256)
		die("Missing SPD data.");
	// leave onboard dimm address at f0, and copy spd data there.
	return spd_data[spd_index];
}

void mainboard_fill_pei_data(struct pei_data *pei_data)
{
	struct pei_data pei_data_template = {
		.pei_version = PEI_VERSION,
		.mchbar = (uintptr_t)DEFAULT_MCHBAR,
		.dmibar = (uintptr_t)DEFAULT_DMIBAR,
		.epbar = DEFAULT_EPBAR,
		.pciexbar = CONFIG_MMCONF_BASE_ADDRESS,
		.smbusbar = SMBUS_IO_BASE,
		.wdbbar = 0x4000000,
		.wdbsize = 0x1000,
		.hpet_address = CONFIG_HPET_ADDRESS,
		.rcba = (uintptr_t)DEFAULT_RCBABASE,
		.pmbase = DEFAULT_PMBASE,
		.gpiobase = DEFAULT_GPIOBASE,
		.thermalbase = 0xfed08000,
		.system_type = 0, // 0 Mobile, 1 Desktop/Server
		.tseg_size = CONFIG_SMM_TSEG_SIZE,
		.spd_addresses = { 0xa0, 0x00,0x00,0x00 },
		.ts_addresses = { 0x30, 0x00, 0x00, 0x00 },
		.ec_present = 1,
		// 0 = leave channel enabled
		// 1 = disable dimm 0 on channel
		// 2 = disable dimm 1 on channel
		// 3 = disable dimm 0+1 on channel
		.dimm_channel0_disabled = 2,
		.dimm_channel1_disabled = 2,
		.max_ddr3_freq = 1333,
		.usb_port_config = {
			{ 1, 0, 0x0080 }, /* P0: Port 0      (OC0) */
			{ 1, 1, 0x0080 }, /* P1: Port 1      (OC1) */
			{ 1, 0, 0x0040 }, /* P2: MINIPCIE1   (no OC) */
			{ 1, 0, 0x0040 }, /* P3: MMC         (no OC) */
			{ 0, 0, 0x0000 }, /* P4: Empty */
			{ 0, 0, 0x0000 }, /* P5: Empty */
			{ 0, 0, 0x0000 }, /* P6: Empty */
			{ 0, 0, 0x0000 }, /* P7: Empty */
			{ 1, 4, 0x0040 }, /* P8: MINIPCIE2   (no OC) */
			{ 0, 4, 0x0000 }, /* P9: Empty */
			{ 0, 4, 0x0000 }, /* P10: Empty */
			{ 1, 4, 0x0040 }, /* P11: Camera     (no OC) */
			{ 0, 4, 0x0000 }, /* P12: Empty */
			{ 0, 4, 0x0000 }, /* P13: Empty */
		},
	};
	*pei_data = pei_data_template;
	// leave onboard dimm address at f0, and copy spd data there.
	memcpy(pei_data->spd_data[0], locate_spd(), 256);
}

const struct southbridge_usb_port mainboard_usb_ports[] = {
	/* enabled power  usb oc pin  */
	{ 1, 1, 0 }, /* P0: Port 0      (OC0) */
	{ 1, 1, 1 }, /* P1: Port 1      (OC1) */
	{ 1, 0, -1 }, /* P2: MINIPCIE1   (no OC) */
	{ 1, 0, -1 }, /* P3: MMC         (no OC) */
	{ 0, 0, -1 }, /* P4: Empty */
	{ 0, 0, -1 }, /* P5: Empty */
	{ 0, 0, -1 }, /* P6: Empty */
	{ 0, 0, -1 }, /* P7: Empty */
	{ 1, 0, -1 }, /* P8: MINIPCIE2   (no OC) */
	{ 0, 0, -1 }, /* P9: Empty */
	{ 0, 0, -1 }, /* P10: Empty */
	{ 1, 0, -1 }, /* P11: Camera     (no OC) */
	{ 0, 0, -1 }, /* P12: Empty */
	{ 0, 0, -1 }, /* P13: Empty */
};

void mainboard_get_spd(spd_raw_data *spd, bool id_only)
{
	/* get onboard dimm spd */
	memcpy(&spd[2], locate_spd(), 256);
	/* read removable dimm spd */
	read_spd(&spd[0], 0x50, id_only);
}

void mainboard_early_init(int s3resume)
{
	init_bootmode_straps();
}

int mainboard_should_reset_usb(int s3resume)
{
	return !s3resume;
}

void mainboard_config_superio(void)
{
}
