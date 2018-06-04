/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Advanced Micro Devices, Inc.
 * Copyright (C) 2014 Edward O'Callaghan <eocallaghan@alterapraxis.com>
 * Copyright (C) 2014 Felix Held <felix-coreboot@felixheld.de>
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

/*
 * A generic romstage (pre-ram) driver for Nuvoton variant Super I/O chips.
 *
 * The following is derived directly from the vendor Nuvoton's data-sheets:
 *
 * To toggle between `configuration mode` and `normal operation mode` as to
 * manipulate the various LDN's in Nuvoton Super I/O's we are required to
 * pass magic numbers `passwords keys`.
 *
 *  NUVOTON_ENTRY_KEY :=  enable  configuration : 0x87
 *  NUVOTON_EXIT_KEY  :=  disable configuration : 0xAA
 *
 * To modify a LDN's configuration register, we use the index port to select
 * the index of the LDN and then write to the data port to alter the
 * parameters. A default index, data port pair is 0x4E, 0x4F respectively, a
 * user modified pair is 0x2E, 0x2F respectively.
 *
 */

#include <arch/io.h>
#include <device/pnp.h>
#include <stdint.h>
#include "nuvoton.h"

#define NUVOTON_ENTRY_KEY 0x87
#define NUVOTON_EXIT_KEY 0xAA

/* Enable configuration: pass entry key '0x87' into index port dev
 * two times. */
void nuvoton_pnp_enter_conf_state(pnp_devfn_t dev)
{
	u16 port = dev >> 8;
	outb(NUVOTON_ENTRY_KEY, port);
	outb(NUVOTON_ENTRY_KEY, port);
}

/* Disable configuration: pass exit key '0xAA' into index port dev. */
void nuvoton_pnp_exit_conf_state(pnp_devfn_t dev)
{
	u16 port = dev >> 8;
	outb(NUVOTON_EXIT_KEY, port);
}

/* Bring up early serial debugging output before the RAM is initialized. */
void nuvoton_enable_serial(pnp_devfn_t dev, u16 iobase)
{
	nuvoton_pnp_enter_conf_state(dev);
	if (IS_ENABLED(CONFIG_SUPERIO_NUVOTON_NCT6776_COM_A))
		/* Route GPIO8 pin group to COM A */
		pnp_write_config(dev, 0x2a, 0x40);
	pnp_set_logical_device(dev);
	pnp_set_enable(dev, 0);
	pnp_set_iobase(dev, PNP_IDX_IO0, iobase);
	pnp_set_enable(dev, 1);
	nuvoton_pnp_exit_conf_state(dev);
}
