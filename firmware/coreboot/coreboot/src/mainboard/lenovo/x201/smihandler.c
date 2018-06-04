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

#include <arch/io.h>
#include <console/console.h>
#include <cpu/x86/smm.h>
#include "southbridge/intel/ibexpeak/nvs.h"
#include "southbridge/intel/ibexpeak/pch.h"
#include "southbridge/intel/ibexpeak/me.h"
#include <northbridge/intel/nehalem/nehalem.h>
#include <cpu/intel/model_2065x/model_2065x.h>
#include <ec/acpi/ec.h>
#include <pc80/mc146818rtc.h>
#include <ec/lenovo/h8/h8.h>
#include <delay.h>
#include "dock.h"
#include "smi.h"

#define GPE_EC_SCI	1
#define GPE_EC_WAKE	13

static void mainboard_smm_init(void)
{
	printk(BIOS_DEBUG, "initializing SMI\n");
	/* Enable 0x1600/0x1600 register pair */
	ec_set_bit(0x00, 0x05);
}

int mainboard_io_trap_handler(int smif)
{
	static int smm_initialized;

	if (!smm_initialized) {
		mainboard_smm_init();
		smm_initialized = 1;
	}

	switch (smif) {
	case SMI_DOCK_CONNECT:
		ec_clr_bit(0x03, 2);
		udelay(250000);
		dock_connect();
		ec_set_bit(0x03, 2);
		/* set dock LED to indicate status */
		ec_write(0x0c, 0x09);
		ec_write(0x0c, 0x88);
		break;

	case SMI_DOCK_DISCONNECT:
		ec_clr_bit(0x03, 2);
		dock_disconnect();
		break;

	default:
		return 0;
	}

	/* On success, the IO Trap Handler returns 1
	 * On failure, the IO Trap Handler returns a value != 1 */
	return 1;
}

static void mainboard_smi_brightness_up(void)
{
	u8 value;

	if ((value = pci_read_config8(PCI_DEV(0, 2, 1), 0xf4)) < 0xf0)
		pci_write_config8(PCI_DEV(0, 2, 1), 0xf4, (value + 0x10) | 0xf);
}

static void mainboard_smi_brightness_down(void)
{
	u8 value;

	if ((value = pci_read_config8(PCI_DEV(0, 2, 1), 0xf4)) > 0x10)
		pci_write_config8(PCI_DEV(0, 2, 1), 0xf4,
				  (value - 0x10) & 0xf0);
}

static void mainboard_smi_handle_ec_sci(void)
{
	u8 status = inb(EC_SC);
	u8 event;

	if (!(status & EC_SCI_EVT))
		return;

	event = ec_query();
	printk(BIOS_DEBUG, "EC event %02x\n", event);

	switch (event) {
	case 0x14:
		/* brightness up */
		mainboard_smi_brightness_up();
		break;
	case 0x15:
		/* brightness down */
		mainboard_smi_brightness_down();
		break;
	case 0x18:
		/* Fn-F9 key */
	case 0x27:
		/* Power loss */
	case 0x50:
		/* Undock Key */
		mainboard_io_trap_handler(SMI_DOCK_DISCONNECT);
		break;
	case 0x37:
	case 0x58:
		/* Dock Event */
		mainboard_io_trap_handler(SMI_DOCK_CONNECT);
		break;
	default:
		break;
	}
}

void mainboard_smi_gpi(u32 gpi_sts)
{
	if (gpi_sts & (1 << GPE_EC_SCI))
		mainboard_smi_handle_ec_sci();
}

static int mainboard_finalized = 0;

int mainboard_smi_apmc(u8 data)
{
	switch (data) {
	case APM_CNT_FINALIZE:
		printk(BIOS_DEBUG, "APMC: FINALIZE\n");
		if (mainboard_finalized) {
			printk(BIOS_DEBUG, "APMC#: Already finalized\n");
			return 0;
		}

		intel_me_finalize_smm();
		intel_pch_finalize_smm();
		intel_nehalem_finalize_smm();
		intel_model_2065x_finalize_smm();

		mainboard_finalized = 1;
		break;
	case APM_CNT_ACPI_ENABLE:
		/* use 0x1600/0x1604 to prevent races with userspace */
		ec_set_ports(0x1604, 0x1600);
		/* route H8SCI to SCI */
		gpi_route_interrupt(GPE_EC_SCI, GPI_IS_SCI);
		/* discard all events, and enable attention */
		ec_write(0x80, 0x01);
		break;
	case APM_CNT_ACPI_DISABLE:
		/* we have to use port 0x62/0x66, as 0x1600/0x1604 doesn't
		   provide a EC query function */
		ec_set_ports(0x66, 0x62);
		/* route H8SCI# to SMI */
		gpi_route_interrupt(GPE_EC_SCI, GPI_IS_SMI);
		/* discard all events, and enable attention */
		ec_write(0x80, 0x01);
		break;
	default:
		break;
	}
	return 0;
}

void mainboard_smi_sleep(u8 slp_typ)
{
	h8_usb_always_on();

	if (slp_typ == 3) {
		u8 ec_wake = ec_read(0x32);
		/* If EC wake events are enabled, enable wake on EC WAKE GPE.  */
		if (ec_wake & 0x14) {
			/* Redirect EC WAKE GPE to SCI.  */
			gpi_route_interrupt(GPE_EC_WAKE, GPI_IS_SCI);
		}
	}
}
