/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Edward O'Callaghan <eocallaghan@alterapraxis.com>
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

/* Setup only for Fan2
 * Todo: Add support for Fan1 and Fan3
 */

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pnp.h>
#include "fintek_internal.h"
#include "chip.h"


/* Register addresses */
// Choose between AMD and Intel
#define HWM_AMD_TSI_ADDR             0x08
#define HWM_AMD_TSI_CONTROL_REG      0x0A

// Set temp sensors type
#define TEMP_SENS_TYPE_REG	0x6B

// FAN prog sel
#define HWM_FAN3_CONTROL	0x9A
#define HWM_FAN_SEL		0x94
#define HWM_FAN_MODE		0x96
#define HWM_FAN2_TEMP_MAP_SEL      0xBF

// Fan 2 - 4 Boundries
#define HWM_FAN2_BOUND1	0xB6
#define HWM_FAN2_BOUND2	0xB7
#define HWM_FAN2_BOUND3	0xB8
#define HWM_FAN2_BOUND4	0xB9
// Fan 2 - 5 Segment speeds
#define HWM_FAN2_SEG1_SPEED_COUNT  0xBA
#define HWM_FAN2_SEG2_SPEED_COUNT  0xBB
#define HWM_FAN2_SEG3_SPEED_COUNT  0xBC
#define HWM_FAN2_SEG4_SPEED_COUNT  0xBD
#define HWM_FAN2_SEG5_SPEED_COUNT  0xBE


void f81866d_hwm_init(struct device *dev)
{
	struct resource *res = find_resource(dev, PNP_IDX_IO0);

	if (!res) {
		printk(BIOS_WARNING, "Super I/O HWM: No HWM resource found.\n");
		return;
	}

	const struct superio_fintek_f81866d_config *reg = dev->chip_info;
	u16 port = res->base;

	pnp_enter_conf_mode(dev);

	/* Use AMD TSI */
	pnp_write_index(port, HWM_AMD_TSI_ADDR, reg->hwm_amd_tsi_addr);
	pnp_write_index(port, HWM_AMD_TSI_CONTROL_REG, reg->hwm_amd_tsi_control);

	/* Set temp1 sensor to thermistor */
	pnp_write_index(port, TEMP_SENS_TYPE_REG, reg->hwm_temp_sens_type);

	/* Select FAN Type */
	pnp_write_index(port, HWM_FAN_SEL, reg->hwm_fan_select);

	/* Select FAN Mode*/
	pnp_write_index(port, HWM_FAN_MODE, reg->hwm_fan_mode);

	/* Set Boundries */
	pnp_write_index(port, HWM_FAN2_BOUND1, reg->hwm_fan2_bound1);
	pnp_write_index(port, HWM_FAN2_BOUND2, reg->hwm_fan2_bound2);
	pnp_write_index(port, HWM_FAN2_BOUND3, reg->hwm_fan2_bound3);
	pnp_write_index(port, HWM_FAN2_BOUND4, reg->hwm_fan2_bound4);

	/* Set Speed */
	pnp_write_index(port, HWM_FAN2_SEG1_SPEED_COUNT, reg->hwm_fan2_seg1_speed);
	pnp_write_index(port, HWM_FAN2_SEG2_SPEED_COUNT, reg->hwm_fan2_seg2_speed);
	pnp_write_index(port, HWM_FAN2_SEG3_SPEED_COUNT, reg->hwm_fan2_seg3_speed);
	pnp_write_index(port, HWM_FAN2_SEG4_SPEED_COUNT, reg->hwm_fan2_seg4_speed);
	pnp_write_index(port, HWM_FAN2_SEG5_SPEED_COUNT, reg->hwm_fan2_seg5_speed);

	/* Set Fan control freq */
	pnp_write_index(port, HWM_FAN3_CONTROL, reg->hwm_fan3_control);
	pnp_write_index(port, HWM_FAN2_TEMP_MAP_SEL, reg->hwm_fan2_temp_map_select);

	pnp_exit_conf_mode(dev);
}
