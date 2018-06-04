/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Edward O'Callaghan <eocallaghan@alterapraxis.com>
 * Copyright (C) 2017 Nicola Corna <nicola@corna.info>
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
#include "chip.h"

/* Intel Ibex Peak/PECI/AMD TSI */
#define HWM_PECI_TSI_CTRL_REG	0x0a
#define HWM_TCC_TEMPERATURE_REG	0x0c

/* Fan 1 control */
#define HWM_FAN1_SEG1_SPEED_REG	0xaa
#define HWM_FAN1_SEG2_SPEED_REG	0xab
#define HWM_FAN1_SEG3_SPEED_REG	0xac
#define HWM_FAN1_SEG4_SPEED_REG	0xad
#define HWM_FAN1_SEG5_SPEED_REG	0xae
#define HWM_FAN1_TEMP_SRC_REG	0xaf

/* Fan 2 control */
#define HWM_FAN2_SEG1_SPEED_REG	0xba
#define HWM_FAN2_SEG2_SPEED_REG	0xbb
#define HWM_FAN2_SEG3_SPEED_REG	0xbc
#define HWM_FAN2_SEG4_SPEED_REG	0xbd
#define HWM_FAN2_SEG5_SPEED_REG	0xbe
#define HWM_FAN2_TEMP_SRC_REG	0xbf

void f71808a_hwm_init(struct device *dev)
{
	struct resource *res = find_resource(dev, PNP_IDX_IO0);

	if (!res) {
		printk(BIOS_WARNING, "Super I/O HWM: No HWM resource found.\n");
		return;
	}

	const struct superio_fintek_f71808a_config *reg = dev->chip_info;
	u16 port = res->base;

	pnp_enter_conf_mode(dev);

	pnp_write_index(port, HWM_PECI_TSI_CTRL_REG, reg->hwm_peci_tsi_ctrl);
	pnp_write_index(port, HWM_TCC_TEMPERATURE_REG, reg->hwm_tcc_temp);

	pnp_write_index(port, HWM_FAN1_SEG1_SPEED_REG,
				reg->hwm_fan1_seg1_speed);
	pnp_write_index(port, HWM_FAN1_SEG2_SPEED_REG,
				reg->hwm_fan1_seg2_speed);
	pnp_write_index(port, HWM_FAN1_SEG3_SPEED_REG,
				reg->hwm_fan1_seg3_speed);
	pnp_write_index(port, HWM_FAN1_SEG4_SPEED_REG,
				reg->hwm_fan1_seg4_speed);
	pnp_write_index(port, HWM_FAN1_SEG5_SPEED_REG,
				reg->hwm_fan1_seg5_speed);
	pnp_write_index(port, HWM_FAN1_TEMP_SRC_REG, reg->hwm_fan1_temp_src);

	pnp_write_index(port, HWM_FAN2_SEG1_SPEED_REG,
				reg->hwm_fan2_seg1_speed);
	pnp_write_index(port, HWM_FAN2_SEG2_SPEED_REG,
				reg->hwm_fan2_seg2_speed);
	pnp_write_index(port, HWM_FAN2_SEG3_SPEED_REG,
				reg->hwm_fan2_seg3_speed);
	pnp_write_index(port, HWM_FAN2_SEG4_SPEED_REG,
				reg->hwm_fan2_seg4_speed);
	pnp_write_index(port, HWM_FAN2_SEG5_SPEED_REG,
				reg->hwm_fan2_seg5_speed);
	pnp_write_index(port, HWM_FAN2_TEMP_SRC_REG, reg->hwm_fan2_temp_src);

	pnp_exit_conf_mode(dev);
}
