/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 BAP - Bruhnspace Advanced Projects
 * (Written by Fabian Kunkel <fabi@adv.bruhnspace.com> for BAP)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pnp.h>
#include "fintek_internal.h"
#include "f81866d.h"

#define LDN_REG			0x07
#define PORT_SELECT_REGISTER	0x27
#define MULTI_FUNC_SEL3_REG	0x29
#define IRQ_SHARE_REGISTER	0xF0
#define FIFO_SEL_MODE		0xF6

/*
 * f81866d_uart_init enables all necessary registers for UART 3/4
 * Fintek needs to know if pins are used as GPIO or UART pins
 * Share interrupt usage needs to be enabled
 */
void f81866d_uart_init(struct device *dev)
{
	struct resource *res = find_resource(dev, PNP_IDX_IO0);
	u8 tmp;

	if (!res) {
		printk(BIOS_WARNING, "%s: No UART resource found.\n", __func__);
		return;
	}

	pnp_enter_conf_mode(dev);

	// Set Port Select Register (Bit 0) = 0
	// before accessing Multi Function Select 3 Register
	tmp = pnp_read_config(dev, PORT_SELECT_REGISTER);
	pnp_write_config(dev, PORT_SELECT_REGISTER, tmp & 0xFE);

	// Set UART 3 function (Bit 4/5), otherwise pin 36-43 are GPIO
	if (dev->path.pnp.device ==  F81866D_SP3) {
		tmp = pnp_read_config(dev, MULTI_FUNC_SEL3_REG);
		pnp_write_config(dev, MULTI_FUNC_SEL3_REG, tmp | 0x30);
	}

	// Set UART 4 function (Bit 6/7), otherwise pin 44-51 are GPIO
	if (dev->path.pnp.device ==  F81866D_SP4) {
		tmp = pnp_read_config(dev, MULTI_FUNC_SEL3_REG);
		pnp_write_config(dev, MULTI_FUNC_SEL3_REG, tmp | 0xC0);
	}

	// Select UART X in LDN register
	pnp_write_config(dev, LDN_REG, dev->path.pnp.device & 0xff);
	// Set IRQ trigger mode from active low to high (Bit 3)
	tmp = pnp_read_config(dev, FIFO_SEL_MODE);
	pnp_write_config(dev, FIFO_SEL_MODE, tmp | 0x8);
	// Enable share interrupt (Bit 0)
	pnp_write_config(dev, IRQ_SHARE_REGISTER, 0x01);

	pnp_exit_conf_mode(dev);
}
