/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 MediaTek Inc.
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

#ifndef __SOC_DA9212_H_
#define __SOC_DA9212_H_

void da9212_probe(uint8_t i2c_num);

enum {
	/* Page selection */
	DA9212_REG_PAGE_CON = 0x0,

	/* Regulator Registers */
	DA9212_REG_BUCKA_CONT = 0x5D,
	DA9212_REG_BUCKB_CONT = 0x5E,
	DA9212_REG_BUCKA_CONF = 0xD1,
	DA9212_REG_BUCKB_CONF = 0xD2,
};

/* DA9212_REG_PAGE_CON (addr=0x0) */
enum {
	DA9212_REG_PAGE_SHIFT = 0,
	DA9212_REG_PAGE_MASK = 0xf
};

enum {
	DA9212_REG_PAGE0 = 0,
	DA9212_REG_PAGE2 = 2,
	DA9212_REG_PAGE4 = 4,
	DA9212_PAGE_WRITE_MODE = 0x0,
	DA9212_REPEAT_WRITE_MODE = 0x40,
	DA9212_PAGE_REVERT = 0x80
};

/* DA9212_REG_BUCKA/B_CONT (addr=0x5D/0x5E) */
enum {
	DA9212_BUCK_EN_SHIFT = 0,
	DA9212_BUCK_OFF = 0x0,
	DA9212_BUCK_ON = 0x1,
	DA9212_BUCK_GPI_SHIFT = 1,
	DA9212_BUCK_GPI_MASK = 0x3,
	DA9212_BUCK_GPI_OFF = 0x0,
	DA9212_BUCK_GPI_GPIO0 = 0x1,
	DA9212_BUCK_GPI_GPIO1 = 0x2,
	DA9212_BUCK_GPI_GPIO4 = 0x3,
	DA9212_VBUCK_SEL_SHIFT = 4,
	DA9212_VBUCK_SEL_MASK = 0x1,
	DA9212_VBUCK_SEL_A = 0x0,
	DA9212_VBUCK_SEL_B = 0x1,
};

/* DA9212_REG_BUCKA/B_CONF (addr=0xD1/0xD2) */
enum {
	DA9212_BUCK_MODE_SHIFT = 0,
	DA9212_BUCK_MODE_MASK = 0x3,
	DA9212_BUCK_MODE_MANUAL = 0x0,
	DA9212_BUCK_MODE_PFM = 0x1,
	DA9212_BUCK_MODE_PWM = 0x2,
	DA9212_BUCK_MODE_AUTO = 0x3,
};

/* DA9212_REG_CONFIG_E (addr=0x147) */
enum {
	/* DEVICE IDs */
	DA9212_REG_DEVICE_ID = 0x1,
	DA9212_ID	     = 0x22,
	DA9213_ID	     = 0x23,
	DA9212_REG_VARIANT_ID = 0x2,
	DA9212_VARIANT_ID_AB  = 0x10,
	DA9212_VARIANT_ID_AC  = 0x20
};

#endif
