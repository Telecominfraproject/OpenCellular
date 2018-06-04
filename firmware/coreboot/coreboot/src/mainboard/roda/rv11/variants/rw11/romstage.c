/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 secunet Security Networks AG
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
#include <device/pci.h>
#include <device/pnp.h>
#include <northbridge/intel/sandybridge/raminit.h>
#include <northbridge/intel/sandybridge/raminit_native.h>
#include <northbridge/intel/sandybridge/sandybridge.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <superio/ite/it8783ef/it8783ef.h>
#include <superio/ite/common/ite.h>

void pch_enable_lpc(void)
{
	/* COMA on 0x3f8, COMB on 0x2f8 */
	pci_write_config16(PCH_LPC_DEV, LPC_IO_DEC, 0x0010);
	/* Enable KBC on 0x60/0x64 (KBC),
		  EC on 0x62/0x66 (MC),
		  SIO on 0x2e/0x2f (CNF1) */
	pci_write_config16(PCH_LPC_DEV, LPC_EN,
			   CNF1_LPC_EN | MC_LPC_EN | KBC_LPC_EN |
			   COMB_LPC_EN | COMA_LPC_EN);
}

void mainboard_config_superio(void)
{
	const pnp_devfn_t dev = PNP_DEV(0x2e, IT8783EF_GPIO);

	pnp_enter_conf_state(dev);
	pnp_set_logical_device(dev);

	pnp_write_config(dev, 0x23, ITE_UART_CLK_PREDIVIDE_24);

	/* Switch multi function for UART4 */
	pnp_write_config(dev, 0x2a, 0x04);
	/* Switch multi function for UART3 */
	pnp_write_config(dev, 0x2c, 0x13);

	/* No GPIOs used: Clear any output / pull-up that's set by default */
	pnp_write_config(dev, 0xb8, 0x00);
	pnp_write_config(dev, 0xc0, 0x00);
	pnp_write_config(dev, 0xc3, 0x00);
	pnp_write_config(dev, 0xc8, 0x00);
	pnp_write_config(dev, 0xcb, 0x00);
	pnp_write_config(dev, 0xef, 0x00);

	pnp_exit_conf_state(dev);
}

void mainboard_fill_pei_data(struct pei_data *const pei_data)
{
	const struct pei_data pei_data_template = {
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
		.spd_addresses = { 0xA0, 0xA2, 0xA4, 0xA6 },
		.ts_addresses = { 0x00, 0x00, 0x00, 0x00 },
		.ec_present = 1,
		.gbe_enable = 1,
		.ddr3lv_support = 0,
		// 0 = leave channel enabled
		// 1 = disable dimm 0 on channel
		// 2 = disable dimm 1 on channel
		// 3 = disable dimm 0+1 on channel
		.dimm_channel0_disabled = 0,
		.dimm_channel1_disabled = 0,
		.max_ddr3_freq = 1600,
		.usb_port_config = {
			/* Enabled / OC PIN / Length */
			{ 1, 0, 0x0080 }, /* P00: 1st (left)     USB3 (OC #0) */
			{ 1, 0, 0x0080 }, /* P01: 2nd (left)     USB3 (OC #0) */
			{ 1, 1, 0x0080 }, /* P02: 1st Multibay   USB3 (OC #1) */
			{ 1, 1, 0x0080 }, /* P03: 2nd Multibay   USB3 (OC #1) */
			{ 1, 8, 0x0040 }, /* P04: MiniPCIe 1     USB2 (no OC) */
			{ 1, 8, 0x0040 }, /* P05: MiniPCIe 2     USB2 (no OC) */
			{ 1, 8, 0x0040 }, /* P06: USB Hub x4     USB2 (no OC) */
			{ 1, 8, 0x0040 }, /* P07: MiniPCIe 4     USB2 (no OC) */
			{ 1, 8, 0x0080 }, /* P08: SD card reader USB2 (no OC) */
			{ 1, 4, 0x0080 }, /* P09: 3rd (right)    USB2 (OC #4) */
			{ 1, 5, 0x0040 }, /* P10: 4th (right)    USB2 (OC #5) */
			{ 1, 8, 0x0040 }, /* P11: 3rd Multibay   USB2 (no OC) */
			{ 1, 8, 0x0080 }, /* P12: misc internal  USB2 (no OC) */
			{ 1, 6, 0x0080 }, /* P13: misc internal  USB2 (OC #6) */
		},
		.usb3 = {
			.mode =			3,	/* Smart Auto? */
			.hs_port_switch_mask =	0xf,	/* All four ports. */
			.preboot_support =	1,	/* preOS driver? */
			.xhci_streams =		1,	/* Enable. */
		},
		.pcie_init = 1,
	};
	*pei_data = pei_data_template;
}

const struct southbridge_usb_port mainboard_usb_ports[] = {
	/* Enabled / Power / OC PIN */
	{ 1, 1, 0 }, /* P00: 1st (left)     USB3 (OC #0) */
	{ 1, 1, 0 }, /* P01: 2nd (left)     USB3 (OC #0) */
	{ 1, 1, 1 }, /* P02: 1st Multibay   USB3 (OC #1) */
	{ 1, 1, 1 }, /* P03: 2nd Multibay   USB3 (OC #1) */
	{ 1, 0, 8 }, /* P04: MiniPCIe 1     USB2 (no OC) */
	{ 1, 0, 8 }, /* P05: MiniPCIe 2     USB2 (no OC) */
	{ 1, 0, 8 }, /* P06: USB Hub x4     USB2 (no OC) */
	{ 1, 0, 8 }, /* P07: MiniPCIe 4     USB2 (no OC) */
	{ 1, 1, 8 }, /* P08: SD card reader USB2 (no OC) */
	{ 1, 1, 4 }, /* P09: 3rd (right)    USB2 (OC #4) */
	{ 1, 0, 5 }, /* P10: 4th (right)    USB2 (OC #5) */
	{ 1, 0, 8 }, /* P11: 3rd Multibay   USB2 (no OC) */
	{ 1, 1, 8 }, /* P12: misc internal  USB2 (no OC) */
	{ 1, 1, 6 }, /* P13: misc internal  USB2 (OC #6) */
};

void mainboard_get_spd(spd_raw_data *spd, bool id_only)
{
	read_spd(&spd[0], 0x50, id_only);
	read_spd(&spd[1], 0x51, id_only);
	read_spd(&spd[2], 0x52, id_only);
	read_spd(&spd[3], 0x53, id_only);
}
