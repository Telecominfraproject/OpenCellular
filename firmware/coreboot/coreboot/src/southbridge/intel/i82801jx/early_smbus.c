/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
 *               2012 secunet Security Networks AG
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
#include <device/pci_ids.h>
#include <device/pci_def.h>
#include <southbridge/intel/common/smbus.h>
#include "i82801jx.h"

void enable_smbus(void)
{
	pci_devfn_t dev;

	/* Set the SMBus device statically. */
	dev = PCI_DEV(0x0, 0x1f, 0x3);

	/* Set SMBus I/O base. */
	pci_write_config32(dev, SMB_BASE,
			   SMBUS_IO_BASE | PCI_BASE_ADDRESS_SPACE_IO);

	/* Set SMBus enable. */
	pci_write_config8(dev, HOSTC, HST_EN);

	/* Set SMBus I/O space enable. */
	pci_write_config16(dev, PCI_COMMAND, PCI_COMMAND_IO);

	/* Disable interrupt generation. */
	outb(0, SMBUS_IO_BASE + SMBHSTCTL);

	/* Clear any lingering errors, so transactions can run. */
	outb(inb(SMBUS_IO_BASE + SMBHSTSTAT), SMBUS_IO_BASE + SMBHSTSTAT);
	printk(BIOS_DEBUG, "SMBus controller enabled.\n");
}

int smbus_read_byte(unsigned device, unsigned address)
{
	return do_smbus_read_byte(SMBUS_IO_BASE, device, address);
}

int i2c_block_read(unsigned int device, unsigned int offset, u32 bytes, u8 *buf)
{
	return do_i2c_block_read(SMBUS_IO_BASE, device, offset, bytes, buf);
}

int smbus_block_read(unsigned int device, unsigned int cmd, u8 bytes, u8 *buf)
{
	return do_smbus_block_read(SMBUS_IO_BASE, device, cmd, bytes, buf);
}

int smbus_block_write(unsigned int device, unsigned int cmd, u8 bytes,
		const u8 *buf)
{
	return do_smbus_block_write(SMBUS_IO_BASE, device, cmd, bytes, buf);
}
