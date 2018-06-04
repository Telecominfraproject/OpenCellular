/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2005 Yinghai Lu <yinghailu@gmail.com>
 * Copyright (C) 2008-2009 coresystems GmbH
 * Copyright (C) 2014 Google Inc.
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
#include <device/device.h>
#include <device/path.h>
#include <device/smbus_def.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <soc/ramstage.h>
#include <soc/smbus.h>

static void smbus_delay(void)
{
	inb(0x80);
}

static int smbus_wait_until_ready(u16 smbus_base)
{
	unsigned loops = SMBUS_TIMEOUT;
	unsigned char byte;
	do {
		smbus_delay();
		if (--loops == 0)
			break;
		byte = inb(smbus_base + SMBHSTSTAT);
	} while (byte & 1);
	return loops ? 0 : -1;
}

static int smbus_wait_until_done(u16 smbus_base)
{
	unsigned loops = SMBUS_TIMEOUT;
	unsigned char byte;
	do {
		smbus_delay();
		if (--loops == 0)
			break;
		byte = inb(smbus_base + SMBHSTSTAT);
	} while ((byte & 1) || (byte & ~((1 << 6) | (1 << 0))) == 0);
	return loops ? 0 : -1;
}

int do_smbus_read_byte(unsigned smbus_base, unsigned device, unsigned address)
{
	unsigned char global_status_register;
	unsigned char byte;

	if (smbus_wait_until_ready(smbus_base) < 0) {
		return SMBUS_WAIT_UNTIL_READY_TIMEOUT;
	}
	/* Setup transaction */
	/* Disable interrupts */
	outb(inb(smbus_base + SMBHSTCTL) & (~1), smbus_base + SMBHSTCTL);
	/* Set the device I'm talking to */
	outb(((device & 0x7f) << 1) | 1, smbus_base + SMBXMITADD);
	/* Set the command/address... */
	outb(address & 0xff, smbus_base + SMBHSTCMD);
	/* Set up for a byte data read */
	outb((inb(smbus_base + SMBHSTCTL) & 0xe3) | (0x2 << 2),
	     (smbus_base + SMBHSTCTL));
	/* Clear any lingering errors, so the transaction will run */
	outb(inb(smbus_base + SMBHSTSTAT), smbus_base + SMBHSTSTAT);

	/* Clear the data byte... */
	outb(0, smbus_base + SMBHSTDAT0);

	/* Start the command */
	outb((inb(smbus_base + SMBHSTCTL) | 0x40),
	     smbus_base + SMBHSTCTL);

	/* Poll for transaction completion */
	if (smbus_wait_until_done(smbus_base) < 0) {
		return SMBUS_WAIT_UNTIL_DONE_TIMEOUT;
	}

	global_status_register = inb(smbus_base + SMBHSTSTAT);

	/* Ignore the "In Use" status... */
	global_status_register &= ~(3 << 5);

	/* Read results of transaction */
	byte = inb(smbus_base + SMBHSTDAT0);
	if (global_status_register != (1 << 1)) {
		return SMBUS_ERROR;
	}
	return byte;
}

int do_smbus_write_byte(unsigned smbus_base, unsigned device,
			unsigned address, unsigned data)
{
	unsigned char global_status_register;

	if (smbus_wait_until_ready(smbus_base) < 0)
		return SMBUS_WAIT_UNTIL_READY_TIMEOUT;

	/* Setup transaction */
	/* Disable interrupts */
	outb(inb(smbus_base + SMBHSTCTL) & (~1), smbus_base + SMBHSTCTL);
	/* Set the device I'm talking to */
	outb(((device & 0x7f) << 1) & ~0x01, smbus_base + SMBXMITADD);
	/* Set the command/address... */
	outb(address & 0xff, smbus_base + SMBHSTCMD);
	/* Set up for a byte data read */
	outb((inb(smbus_base + SMBHSTCTL) & 0xe3) | (0x2 << 2),
	     (smbus_base + SMBHSTCTL));
	/* Clear any lingering errors, so the transaction will run */
	outb(inb(smbus_base + SMBHSTSTAT), smbus_base + SMBHSTSTAT);

	/* Clear the data byte... */
	outb(data, smbus_base + SMBHSTDAT0);

	/* Start the command */
	outb((inb(smbus_base + SMBHSTCTL) | 0x40),
	     smbus_base + SMBHSTCTL);

	/* Poll for transaction completion */
	if (smbus_wait_until_done(smbus_base) < 0) {
		printk(BIOS_ERR, "SMBUS transaction timeout\n");
		return SMBUS_WAIT_UNTIL_DONE_TIMEOUT;
	}

	global_status_register = inb(smbus_base + SMBHSTSTAT);

	/* Ignore the "In Use" status... */
	global_status_register &= ~(3 << 5);

	/* Read results of transaction */
	if (global_status_register != (1 << 1)) {
		printk(BIOS_ERR, "SMBUS transaction error\n");
		return SMBUS_ERROR;
	}

	return 0;
}
