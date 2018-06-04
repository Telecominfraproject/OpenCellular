/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Intel Corp.
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

#define __SIMPLE_DEVICE__

#include <assert.h>
#include <arch/io.h>
#include <console/console.h>
#include <device/i2c_simple.h>
#include <soc/pci_devs.h>
#include <soc/reg_access.h>
#include "reg_access.h"

static uint64_t reg_read(struct reg_script_context *ctx)
{
	int ret_code;
	const struct reg_script *step;
	uint8_t value = 0;

	step = ctx->step;
	switch (step->id) {
	default:
		printk(BIOS_ERR,
			"ERROR - Unknown register set (0x%08x)!\n",
			step->id);
		ctx->display_features = REG_SCRIPT_DISPLAY_NOTHING;
		break;

	case GEN1_I2C_GPIO_EXP_0x20:
	case GEN1_I2C_GPIO_EXP_0x21:
	case GEN2_I2C_GPIO_EXP0:
	case GEN2_I2C_GPIO_EXP1:
	case GEN2_I2C_GPIO_EXP2:
	case GEN2_I2C_LED_PWM:
		if (ctx->display_features)
			printk(BIOS_INFO, "I2C chip 0x%02x: ", step->id);
		ret_code = i2c_readb(0, step->id, (uint8_t)step->reg, &value);
		ASSERT(ret_code == 2);
		break;
	}
	return value;
}

static void reg_write(struct reg_script_context *ctx)
{
	int ret_code;
	const struct reg_script *step;
	uint8_t value;

	step = ctx->step;
	switch (step->id) {
	default:
		printk(BIOS_ERR,
			"ERROR - Unknown register set (0x%08x)!\n",
			step->id);
		ctx->display_features = REG_SCRIPT_DISPLAY_NOTHING;
		break;

	case GEN1_I2C_GPIO_EXP_0x20:
	case GEN1_I2C_GPIO_EXP_0x21:
	case GEN2_I2C_GPIO_EXP0:
	case GEN2_I2C_GPIO_EXP1:
	case GEN2_I2C_GPIO_EXP2:
	case GEN2_I2C_LED_PWM:
	case RMU_TEMP_REGS:
		if (ctx->display_features)
			printk(BIOS_INFO, "I2C chip 0x%02x: ", step->id);
		value = (uint8_t)step->value;
		ret_code = i2c_writeb(0, step->id, (uint8_t)step->reg, value);
		ASSERT(ret_code == 2);
		break;
	}
}

const struct reg_script_bus_entry mainboard_reg_script_bus_table = {
	MAINBOARD_TYPE, reg_read, reg_write
};

REG_SCRIPT_BUS_ENTRY(mainboard_reg_script_bus_table);
