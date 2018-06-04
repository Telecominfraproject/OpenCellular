/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Intel Corporation.
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
#include <device/pci_def.h>
#include <device/early_smbus.h>
#include <intelblocks/smbus.h>
#include <reg_script.h>
#include <soc/pci_devs.h>
#include "smbuslib.h"

static const struct reg_script smbus_init_script[] = {
	/* Set SMBus I/O base address */
	REG_PCI_WRITE32(PCI_BASE_ADDRESS_4, SMBUS_IO_BASE),
	/* Set SMBus enable */
	REG_PCI_WRITE8(HOSTC, HST_EN),
	/* Enable I/O access */
	REG_PCI_WRITE16(PCI_COMMAND, PCI_COMMAND_IO),
	/* Disable interrupts */
	REG_IO_WRITE8(SMBUS_IO_BASE + SMBHSTCTL, 0),
	/* Clear errors */
	REG_IO_WRITE8(SMBUS_IO_BASE + SMBHSTSTAT, 0xff),
	/* Indicate the end of this array by REG_SCRIPT_END */
	REG_SCRIPT_END,
};

u16 smbus_read_word(u32 smbus_dev, u8 addr, u8 offset)
{
	return smbus_read16(SMBUS_IO_BASE, addr, offset);
}

u8 smbus_read_byte(u32 smbus_dev, u8 addr, u8 offset)
{
	return smbus_read8(SMBUS_IO_BASE, addr, offset);
}

u8 smbus_write_byte(u32 smbus_dev, u8 addr, u8 offset, u8 value)
{
	return smbus_write8(SMBUS_IO_BASE, addr, offset, value);
}

void smbus_common_init(void)
{
	reg_script_run_on_dev(PCH_DEV_SMBUS, smbus_init_script);
}
