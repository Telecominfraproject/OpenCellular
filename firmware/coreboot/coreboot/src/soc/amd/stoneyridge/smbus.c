/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
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

#include <io.h>
#include <stdint.h>
#include <soc/smbus.h>

static int smbus_wait_until_ready(u16 smbus_io_base)
{
	u32 loops;
	loops = SMBUS_TIMEOUT;
	do {
		u8 val;
		val = inb(smbus_io_base + SMBHSTSTAT);
		val &= SMBHST_STAT_VAL_BITS;
		if (val == 0) {	/* ready now */
			return 0;
		}
		outb(val, smbus_io_base + SMBHSTSTAT);
	} while (--loops);
	return -2;		/* time out */
}

static int smbus_wait_until_done(u16 smbus_io_base)
{
	u32 loops;
	loops = SMBUS_TIMEOUT;
	do {
		u8 val;

		val = inb(smbus_io_base + SMBHSTSTAT);
		val &= SMBHST_STAT_VAL_BITS;	/* mask off reserved bits */
		if (val & SMBHST_STAT_ERROR_BITS)
			return -5;	/* error */
		if (val == SMBHST_STAT_NOERROR) {
			outb(val, smbus_io_base + SMBHSTSTAT); /* clear sts */
			return 0;
		}
	} while (--loops);
	return -3;		/* timeout */
}

int do_smbus_recv_byte(u16 smbus_io_base, u8 device)
{
	u8 byte;

	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;	/* not ready */

	/* set the device I'm talking to */
	outb(((device & 0x7f) << 1) | 1, smbus_io_base + SMBHSTADDR);

	byte = inb(smbus_io_base + SMBHSTCTRL);
	byte &= ~SMBHST_CTRL_MODE_BITS;			/* Clear [4:2] */
	byte |= SMBHST_CTRL_STRT | SMBHST_CTRL_BTE_RW;	/* set mode, start */
	outb(byte, smbus_io_base + SMBHSTCTRL);

	/* poll for transaction completion */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;	/* timeout or error */

	/* read results of transaction */
	byte = inb(smbus_io_base + SMBHSTDAT0);

	return byte;
}

int do_smbus_send_byte(u16 smbus_io_base, u8 device, u8 val)
{
	u8 byte;

	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;	/* not ready */

	/* set the command... */
	outb(val, smbus_io_base + SMBHSTDAT0);

	/* set the device I'm talking to */
	outb(((device & 0x7f) << 1) | 0, smbus_io_base + SMBHSTADDR);

	byte = inb(smbus_io_base + SMBHSTCTRL);
	byte &= ~SMBHST_CTRL_MODE_BITS;			/* Clear [4:2] */
	byte |= SMBHST_CTRL_STRT | SMBHST_CTRL_BTE_RW;	/* set mode, start */
	outb(byte, smbus_io_base + SMBHSTCTRL);

	/* poll for transaction completion */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;	/* timeout or error */

	return 0;
}

int do_smbus_read_byte(u16 smbus_io_base, u8 device, u8 address)
{
	u8 byte;

	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;	/* not ready */

	/* set the command/address... */
	outb(address & 0xff, smbus_io_base + SMBHSTCMD);

	/* set the device I'm talking to */
	outb(((device & 0x7f) << 1) | 1, smbus_io_base + SMBHSTADDR);

	byte = inb(smbus_io_base + SMBHSTCTRL);
	byte &= ~SMBHST_CTRL_MODE_BITS;			/* Clear [4:2] */
	byte |= SMBHST_CTRL_STRT | SMBHST_CTRL_BDT_RW;	/* set mode, start */
	outb(byte, smbus_io_base + SMBHSTCTRL);

	/* poll for transaction completion */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;	/* timeout or error */

	/* read results of transaction */
	byte = inb(smbus_io_base + SMBHSTDAT0);

	return byte;
}

int do_smbus_write_byte(u16 smbus_io_base, u8 device, u8 address, u8 val)
{
	u8 byte;

	if (smbus_wait_until_ready(smbus_io_base) < 0)
		return -2;	/* not ready */

	/* set the command/address... */
	outb(address & 0xff, smbus_io_base + SMBHSTCMD);

	/* set the device I'm talking to */
	outb(((device & 0x7f) << 1) | 0, smbus_io_base + SMBHSTADDR);

	/* output value */
	outb(val, smbus_io_base + SMBHSTDAT0);

	byte = inb(smbus_io_base + SMBHSTCTRL);
	byte &= ~SMBHST_CTRL_MODE_BITS;			/* Clear [4:2] */
	byte |= SMBHST_CTRL_STRT | SMBHST_CTRL_BDT_RW;	/* set mode, start */
	outb(byte, smbus_io_base + SMBHSTCTRL);

	/* poll for transaction completion */
	if (smbus_wait_until_done(smbus_io_base) < 0)
		return -3;	/* timeout or error */

	return 0;
}

void alink_ab_indx(u32 reg_space, u32 reg_addr, u32 mask, u32 val)
{
	u32 tmp;

	outl((reg_space & 0x7) << 29 | reg_addr, AB_INDX);
	tmp = inl(AB_DATA);
	/* rpr 4.2
	 * For certain revisions of the chip, the ABCFG registers,
	 * with an address of 0x100NN (where 'N' is any hexadecimal
	 * number), require an extra programming step.*/
	outl(0, AB_INDX);

	tmp &= ~mask;
	tmp |= val;

	// printk(BIOS_DEBUG, "about write %x, index=%x", tmp,
	//				(reg_space&0x3)<<29 | reg_addr);

	/* probably we dont have to do it again. */
	outl((reg_space & 0x7) << 29 | reg_addr, AB_INDX);
	outl(tmp, AB_DATA);
	outl(0, AB_INDX);
}

void alink_rc_indx(u32 reg_space, u32 reg_addr, u32 port, u32 mask, u32 val)
{
	u32 tmp;

	outl((reg_space & 0x7) << 29 | (port & 3) << 24 | reg_addr, AB_INDX);
	tmp = inl(AB_DATA);
	/* rpr 4.2
	 * For certain revisions of the chip, the ABCFG registers,
	 * with an address of 0x100NN (where 'N' is any hexadecimal
	 * number), require an extra programming step.*/
	outl(0, AB_INDX);

	tmp &= ~mask;
	tmp |= val;

	//printk(BIOS_DEBUG, "about write %x, index=%x", tmp,
	//		(reg_space&0x3)<<29 | (port&3) << 24 | reg_addr);

	/* probably we dont have to do it again. */
	outl((reg_space & 0x7) << 29 | (port & 3) << 24 | reg_addr, AB_INDX);
	outl(tmp, AB_DATA);
	outl(0, AB_INDX);
}

/*
 * space = 0: AX_INDXC, AX_DATAC
 * space = 1: AX_INDXP, AX_DATAP
 */
void alink_ax_indx(u32 space /*c or p? */, u32 axindc, u32 mask, u32 val)
{
	u32 tmp;

	/* read axindc to tmp */
	outl(space << 29 | space << 3 | 0x30, AB_INDX);
	outl(axindc, AB_DATA);
	outl(0, AB_INDX);
	outl(space << 29 | space << 3 | 0x34, AB_INDX);
	tmp = inl(AB_DATA);
	outl(0, AB_INDX);

	tmp &= ~mask;
	tmp |= val;

	/* write tmp */
	outl(space << 29 | space << 3 | 0x30, AB_INDX);
	outl(axindc, AB_DATA);
	outl(0, AB_INDX);
	outl(space << 29 | space << 3 | 0x34, AB_INDX);
	outl(tmp, AB_DATA);
	outl(0, AB_INDX);
}
