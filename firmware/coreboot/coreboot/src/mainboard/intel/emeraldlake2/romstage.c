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
#include <arch/acpi.h>
#include <cbmem.h>
#include <console/console.h>
#include <superio/smsc/sio1007/chip.h>
#include <northbridge/intel/sandybridge/sandybridge.h>
#include <northbridge/intel/sandybridge/raminit.h>
#include <northbridge/intel/sandybridge/raminit_native.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/common/rcba.h>
#include <southbridge/intel/common/gpio.h>
#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <halt.h>
#include <security/tpm/tis.h>

#define SIO_PORT 0x164e

void pch_enable_lpc(void)
{
	device_t dev = PCH_LPC_DEV;

	/* Set COM1/COM2 decode range */
	pci_write_config16(dev, LPC_IO_DEC, 0x0010);

	/* Enable SuperIO + PS/2 Keyboard/Mouse */
	u16 lpc_config = CNF1_LPC_EN | CNF2_LPC_EN | KBC_LPC_EN;
	pci_write_config16(dev, LPC_EN, lpc_config);

	/* Map 256 bytes at 0x1600 to the LPC bus. */
	pci_write_config32(dev, LPC_GEN1_DEC, 0xfc1601);

	/* Map a range for the runtime_port registers to the LPC bus. */
	pci_write_config32(dev, LPC_GEN2_DEC, 0xc0181);

	/* Enable COM1 */
	if (sio1007_enable_uart_at(SIO_PORT)) {
		pci_write_config16(dev, LPC_EN,
				   lpc_config | COMA_LPC_EN);
	}
}

void mainboard_rcba_config(void)
{
	southbridge_configure_default_intmap();
}

void mainboard_config_superio(void)
{
	const u16 port = SIO_PORT;
	const u16 runtime_port = 0x180;

	/* Turn on configuration mode. */
	outb(0x55, port);

	/* Set the GPIO direction, polarity, and type. */
	sio1007_setreg(port, 0x31, 1 << 0, 1 << 0);
	sio1007_setreg(port, 0x32, 0 << 0, 1 << 0);
	sio1007_setreg(port, 0x33, 0 << 0, 1 << 0);

	/* Set the base address for the runtime register block. */
	sio1007_setreg(port, 0x30, runtime_port >> 4, 0xff);
	sio1007_setreg(port, 0x21, runtime_port >> 12, 0xff);

	/* Turn on address decoding for it. */
	sio1007_setreg(port, 0x3a, 1 << 1, 1 << 1);

	/* Set the value of GPIO 10 by changing GP1, bit 0. */
	u8 byte;
	byte = inb(runtime_port + 0xc);
	byte |= (1 << 0);
	outb(byte, runtime_port + 0xc);

	/* Turn off address decoding for it. */
	sio1007_setreg(port, 0x3a, 0 << 1, 1 << 1);

	/* Turn off configuration mode. */
	outb(0xaa, port);
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
		.spd_addresses = { 0xa0, 0x00, 0xa4, 0x00 },
		.ts_addresses = { 0x00, 0x00, 0x00, 0x00 },
		.ec_present = 0,
		// 0 = leave channel enabled
		// 1 = disable dimm 0 on channel
		// 2 = disable dimm 1 on channel
		// 3 = disable dimm 0+1 on channel
		.dimm_channel0_disabled = 2,
		.dimm_channel1_disabled = 2,
		.max_ddr3_freq = 1600,
		.usb_port_config = {
			{ 1, 0, 0x0040 }, /* P0: Front port  (OC0) */
			{ 1, 1, 0x0040 }, /* P1: Back port   (OC1) */
			{ 1, 0, 0x0040 }, /* P2: MINIPCIE1   (no OC) */
			{ 1, 0, 0x0040 }, /* P3: MMC         (no OC) */
			{ 1, 2, 0x0040 }, /* P4: Front port  (OC2) */
			{ 0, 0, 0x0000 }, /* P5: Empty */
			{ 0, 0, 0x0000 }, /* P6: Empty */
			{ 0, 0, 0x0000 }, /* P7: Empty */
			{ 1, 4, 0x0040 }, /* P8: Back port   (OC4) */
			{ 1, 4, 0x0040 }, /* P9: MINIPCIE3   (no OC) */
			{ 1, 4, 0x0040 }, /* P10: BLUETOOTH  (no OC) */
			{ 0, 4, 0x0000 }, /* P11: Empty */
			{ 1, 6, 0x0040 }, /* P12: Back port  (OC6) */
			{ 1, 5, 0x0040 }, /* P13: Back port  (OC5) */
		},
	};
	*pei_data = pei_data_template;
}

const struct southbridge_usb_port mainboard_usb_ports[] = {
	/* enabled power  usb oc pin  */
	{ 1, 0, 0 }, /* P0: Front port  (OC0) */
	{ 1, 0, 1 }, /* P1: Back port   (OC1) */
	{ 1, 0, -1 }, /* P2: MINIPCIE1   (no OC) */
	{ 1, 0, -1 }, /* P3: MMC         (no OC) */
	{ 1, 0, 2 }, /* P4: Front port  (OC2) */
	{ 0, 0, -1 }, /* P5: Empty */
	{ 0, 0, -1 }, /* P6: Empty */
	{ 0, 0, -1 }, /* P7: Empty */
	{ 1, 0, 4 }, /* P8: Back port   (OC4) */
	{ 1, 0, -1 }, /* P9: MINIPCIE3   (no OC) */
	{ 1, 0, -1 }, /* P10: BLUETOOTH  (no OC) */
	{ 0, 0, -1 }, /* P11: Empty */
	{ 1, 0, 6 }, /* P12: Back port  (OC6) */
	{ 1, 0, 5 }, /* P13: Back port  (OC5) */
};

void mainboard_get_spd(spd_raw_data *spd, bool id_only) {
	read_spd(&spd[0], 0x50, id_only);
	read_spd(&spd[2], 0x52, id_only);
}

void mainboard_early_init(int s3resume)
{
}

int mainboard_should_reset_usb(int s3resume)
{
	return !s3resume;
}
