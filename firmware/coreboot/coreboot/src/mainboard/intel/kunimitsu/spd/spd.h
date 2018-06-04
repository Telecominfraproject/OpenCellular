/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 * Copyright (C) 2015 Intel Corporation.
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

#ifndef MAINBOARD_SPD_H

#include <gpio.h>
#include "../gpio.h"

#define MAINBOARD_SPD_H

#define SPD_LEN			256

#define SPD_DRAM_TYPE		2
#define  SPD_DRAM_DDR3		0x0b
#define  SPD_DRAM_LPDDR3	0xf1
#define SPD_DENSITY_BANKS	4
#define SPD_ADDRESSING		5
#define SPD_ORGANIZATION	7
#define SPD_BUS_DEV_WIDTH	8
#define SPD_PART_OFF		128
#define  SPD_PART_LEN		18
#define SPD_MANU_OFF		148

#define HYNIX_SINGLE_CHAN	0x1
#define SAMSUNG_SINGLE_CHAN	0x4
#define MIC_SINGLE_CHAN		0x5

/* PCH_MEM_CFG[3:0] */
#define MAX_MEMORY_CONFIG	0x10
#define RCOMP_TARGET_PARAMS	0x5
#define K4E6E304EE_MEM_ID	0x3

static inline int get_spd_index(void) {
	/* PCH_MEM_CFG[3:0] */
	gpio_t spd_gpios[] = {
		GPIO_MEM_CONFIG_0,
		GPIO_MEM_CONFIG_1,
		GPIO_MEM_CONFIG_2,
		GPIO_MEM_CONFIG_3,
	};
	return (gpio_base2_value(spd_gpios, ARRAY_SIZE(spd_gpios)));
}
void mainboard_fill_dq_map_data(void *dq_map_ptr);
void mainboard_fill_dqs_map_data(void *dqs_map_ptr);
void mainboard_fill_rcomp_res_data(void *rcomp_ptr);
void mainboard_fill_rcomp_strength_data(void *rcomp_strength_ptr);
uintptr_t mainboard_get_spd_data(void);
int mainboard_has_dual_channel_mem(void);
#endif
