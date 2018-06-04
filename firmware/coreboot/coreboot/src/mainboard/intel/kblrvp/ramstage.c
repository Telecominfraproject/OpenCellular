/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Intel Corporation
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

#include <bootstate.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <soc/ramstage.h>
#include <variant/gpio.h>

void mainboard_silicon_init_params(FSP_SIL_UPD *params)
{
	/* Configure pads prior to SiliconInit() in case there's any
	 * dependencies during hardware initialization. */
	gpio_configure_pads(gpio_table, ARRAY_SIZE(gpio_table));
	params->CdClock = 3;

	/* Enable Virtual Channel 1 */
	params->PchHdaVcType = 0x1;
}

static void ioexpander_init(void *unused)
{
	printk(BIOS_DEBUG, "Programming TCA6424A I/O expander\n");

	/* I/O Expander 1, Port 0 Data */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_0_ADDR, IO_EXPANDER_P0DOUT,
		0xF7);
	/* Port 0 Configuration */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_0_ADDR, IO_EXPANDER_P0CONF,
		0xE0);

	/* Port 1 Data */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_0_ADDR, IO_EXPANDER_P1DOUT,
		0x9E);
	/* Port 1 Configuration */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_0_ADDR, IO_EXPANDER_P1CONF,
		0x8C);

	/* Port 2 Data */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_0_ADDR, IO_EXPANDER_P2DOUT,
		0xDA);
	/* Port 2 Configuration */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_0_ADDR, IO_EXPANDER_P2CONF,
		0x08);

	/* I/O Expander 2, Port 0 Data */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_1_ADDR, IO_EXPANDER_P0DOUT,
		0xFF);
	/* Port 0 Configuration */
	i2c_writeb(IO_EXPANDER_BUS, IO_EXPANDER_1_ADDR, IO_EXPANDER_P0CONF,
		0x00);

}

BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_EXIT, ioexpander_init, NULL);
