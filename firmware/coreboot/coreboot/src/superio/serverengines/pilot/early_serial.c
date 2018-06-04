/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2009 University of Heidelberg
 * Written by Mondrian Nuessle <nuessle@uni-heidelberg.de> for Univ. Heidelberg
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

/* PILOT Super I/O is only based on LPC observation done on factory system. */

#include <arch/io.h>
#include <device/pnp.h>
#include "pilot.h"

/* Pilot uses 0x5A/0xA5 pattern to actiavte deactivate config access. */
void pnp_enter_ext_func_mode(pnp_devfn_t dev)
{
	u16 port = dev >> 8;
	outb(0x5A, port);
}

void pnp_exit_ext_func_mode(pnp_devfn_t dev)
{
	u16 port = dev >> 8;
	outb(0xA5, port);
}

/* Serial config is a fairly standard procedure. */
void pilot_enable_serial(pnp_devfn_t dev, u16 iobase)
{
	pnp_enter_ext_func_mode(dev);
	pnp_set_logical_device(dev);
	pnp_set_iobase(dev, PNP_IDX_IO0, iobase);
	pnp_set_enable(dev, 1);
	pnp_exit_ext_func_mode(dev);
}

void pilot_disable_serial(pnp_devfn_t dev)
{
	pnp_enter_ext_func_mode(dev);
	pnp_set_logical_device(dev);
	pnp_set_iobase(dev, PNP_IDX_IO0, 0x0000);
	pnp_set_enable(dev, 0);
	pnp_exit_ext_func_mode(dev);
}
