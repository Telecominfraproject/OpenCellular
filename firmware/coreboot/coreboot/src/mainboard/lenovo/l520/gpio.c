/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2008-2009 coresystems GmbH
 * Copyright (C) 2016 Patrick Rudolph <siro@das-labor.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <southbridge/intel/common/gpio.h>
const struct pch_gpio_set1 pch_gpio_set1_mode = {
	.gpio0 = GPIO_MODE_GPIO,
	.gpio1 = GPIO_MODE_GPIO,
	.gpio2 = GPIO_MODE_GPIO,
	.gpio3 = GPIO_MODE_GPIO,
	.gpio4 = GPIO_MODE_GPIO,
	.gpio5 = GPIO_MODE_GPIO,
	.gpio6 = GPIO_MODE_GPIO,
	.gpio7 = GPIO_MODE_GPIO,
	.gpio8 = GPIO_MODE_GPIO,
	.gpio12 = GPIO_MODE_GPIO,
	.gpio13 = GPIO_MODE_GPIO,
	.gpio15 = GPIO_MODE_GPIO,
	.gpio16 = GPIO_MODE_GPIO,
	.gpio17 = GPIO_MODE_GPIO,
	.gpio19 = GPIO_MODE_GPIO,
	.gpio21 = GPIO_MODE_GPIO,
	.gpio22 = GPIO_MODE_GPIO,
	.gpio23 = GPIO_MODE_GPIO,
	.gpio24 = GPIO_MODE_GPIO,
	.gpio27 = GPIO_MODE_GPIO,
	.gpio28 = GPIO_MODE_GPIO,
	.gpio29 = GPIO_MODE_GPIO,
	.gpio30 = GPIO_MODE_GPIO,
};

const struct pch_gpio_set1 pch_gpio_set1_direction = {
	.gpio0 = GPIO_DIR_OUTPUT,
	.gpio1 = GPIO_DIR_INPUT,
	.gpio2 = GPIO_DIR_OUTPUT,
	.gpio3 = GPIO_DIR_INPUT,
	.gpio4 = GPIO_DIR_OUTPUT,
	.gpio5 = GPIO_DIR_OUTPUT,
	.gpio6 = GPIO_DIR_INPUT,
	.gpio7 = GPIO_DIR_INPUT,
	.gpio8 = GPIO_DIR_OUTPUT,
	.gpio12 = GPIO_DIR_OUTPUT,
	.gpio13 = GPIO_DIR_OUTPUT,
	.gpio15 = GPIO_DIR_OUTPUT,
	.gpio16 = GPIO_DIR_INPUT,
	.gpio17 = GPIO_DIR_INPUT,
	.gpio19 = GPIO_DIR_OUTPUT,
	.gpio21 = GPIO_DIR_OUTPUT,
	.gpio22 = GPIO_DIR_INPUT,
	.gpio23 = GPIO_DIR_OUTPUT,
	.gpio24 = GPIO_DIR_OUTPUT,
	.gpio27 = GPIO_DIR_OUTPUT,
	.gpio28 = GPIO_DIR_OUTPUT,
	.gpio29 = GPIO_DIR_OUTPUT,
	.gpio30 = GPIO_DIR_OUTPUT,
};

const struct pch_gpio_set1 pch_gpio_set1_level = {
	.gpio0 = GPIO_LEVEL_HIGH,
	.gpio2 = GPIO_LEVEL_HIGH,
	.gpio4 = GPIO_LEVEL_HIGH,
	.gpio5 = GPIO_LEVEL_HIGH,
	.gpio8 = GPIO_LEVEL_LOW,
	.gpio12 = GPIO_LEVEL_LOW,
	.gpio13 = GPIO_LEVEL_LOW,
	.gpio15 = GPIO_LEVEL_LOW,
	.gpio19 = GPIO_LEVEL_LOW,
	.gpio21 = GPIO_LEVEL_LOW,
	.gpio23 = GPIO_LEVEL_LOW,
	.gpio24 = GPIO_LEVEL_HIGH,
	.gpio27 = GPIO_LEVEL_HIGH,
	.gpio28 = GPIO_LEVEL_HIGH,
	.gpio29 = GPIO_LEVEL_HIGH,
	.gpio30 = GPIO_LEVEL_HIGH,
};

const struct pch_gpio_set1 pch_gpio_set1_reset = {
	.gpio24 = GPIO_RESET_RSMRST,
	.gpio30 = GPIO_RESET_RSMRST,
};

const struct pch_gpio_set1 pch_gpio_set1_invert = {
	.gpio1 = GPIO_INVERT,
	.gpio6 = GPIO_INVERT,
};

const struct pch_gpio_set1 pch_gpio_set1_blink = {
};

const struct pch_gpio_set2 pch_gpio_set2_mode = {
	.gpio33 = GPIO_MODE_GPIO,
	.gpio35 = GPIO_MODE_GPIO,
	.gpio36 = GPIO_MODE_GPIO,
	.gpio37 = GPIO_MODE_GPIO,
	.gpio38 = GPIO_MODE_GPIO,
	.gpio39 = GPIO_MODE_GPIO,
	.gpio40 = GPIO_MODE_GPIO,
	.gpio41 = GPIO_MODE_GPIO,
	.gpio48 = GPIO_MODE_GPIO,
	.gpio49 = GPIO_MODE_GPIO,
	.gpio50 = GPIO_MODE_GPIO,
	.gpio51 = GPIO_MODE_GPIO,
	.gpio52 = GPIO_MODE_GPIO,
	.gpio53 = GPIO_MODE_GPIO,
	.gpio54 = GPIO_MODE_GPIO,
	.gpio55 = GPIO_MODE_GPIO,
	.gpio57 = GPIO_MODE_GPIO,
	.gpio59 = GPIO_MODE_GPIO,
	.gpio60 = GPIO_MODE_GPIO,
	.gpio61 = GPIO_MODE_GPIO,
};

const struct pch_gpio_set2 pch_gpio_set2_direction = {
	.gpio33 = GPIO_DIR_OUTPUT,
	.gpio35 = GPIO_DIR_OUTPUT,
	.gpio36 = GPIO_DIR_INPUT,
	.gpio37 = GPIO_DIR_INPUT,
	.gpio38 = GPIO_DIR_INPUT,
	.gpio39 = GPIO_DIR_INPUT,
	.gpio40 = GPIO_DIR_INPUT,
	.gpio41 = GPIO_DIR_INPUT,
	.gpio48 = GPIO_DIR_INPUT,
	.gpio49 = GPIO_DIR_INPUT,
	.gpio50 = GPIO_DIR_OUTPUT,
	.gpio51 = GPIO_DIR_OUTPUT,
	.gpio52 = GPIO_DIR_OUTPUT,
	.gpio53 = GPIO_DIR_OUTPUT,
	.gpio54 = GPIO_DIR_OUTPUT,
	.gpio55 = GPIO_DIR_OUTPUT,
	.gpio57 = GPIO_DIR_INPUT,
	.gpio60 = GPIO_DIR_OUTPUT,
	.gpio61 = GPIO_DIR_OUTPUT,
};

const struct pch_gpio_set2 pch_gpio_set2_level = {
	.gpio33 = GPIO_LEVEL_LOW,
	.gpio35 = GPIO_LEVEL_HIGH,
	.gpio50 = GPIO_LEVEL_HIGH,
	.gpio51 = GPIO_LEVEL_HIGH,
	.gpio52 = GPIO_LEVEL_HIGH,
	.gpio53 = GPIO_LEVEL_HIGH,
	.gpio54 = GPIO_LEVEL_LOW,
	.gpio55 = GPIO_LEVEL_HIGH,
	.gpio57 = GPIO_LEVEL_LOW,
	.gpio60 = GPIO_LEVEL_LOW,
	.gpio61 = GPIO_LEVEL_LOW,
};

const struct pch_gpio_set2 pch_gpio_set2_reset = {
};

const struct pch_gpio_set3 pch_gpio_set3_mode = {
	.gpio64 = GPIO_MODE_GPIO,
	.gpio65 = GPIO_MODE_GPIO,
	.gpio66 = GPIO_MODE_GPIO,
	.gpio67 = GPIO_MODE_GPIO,
	.gpio68 = GPIO_MODE_GPIO,
	.gpio69 = GPIO_MODE_GPIO,
	.gpio70 = GPIO_MODE_GPIO,
	.gpio71 = GPIO_MODE_GPIO,
};

const struct pch_gpio_set3 pch_gpio_set3_direction = {
	.gpio64 = GPIO_DIR_OUTPUT,
	.gpio65 = GPIO_DIR_OUTPUT,
	.gpio66 = GPIO_DIR_OUTPUT,
	.gpio67 = GPIO_DIR_INPUT,
	.gpio68 = GPIO_DIR_INPUT,
	.gpio69 = GPIO_DIR_OUTPUT,
	.gpio70 = GPIO_DIR_OUTPUT,
	.gpio71 = GPIO_DIR_OUTPUT,
};

const struct pch_gpio_set3 pch_gpio_set3_level = {
	.gpio64 = GPIO_LEVEL_LOW,
	.gpio65 = GPIO_LEVEL_LOW,
	.gpio66 = GPIO_LEVEL_LOW,
	.gpio69 = GPIO_LEVEL_LOW,
	.gpio70 = GPIO_LEVEL_HIGH,
	.gpio71 = GPIO_LEVEL_LOW,
};

const struct pch_gpio_set3 pch_gpio_set3_reset = {
};

const struct pch_gpio_map mainboard_gpio_map = {
	.set1 = {
		.mode		= &pch_gpio_set1_mode,
		.direction	= &pch_gpio_set1_direction,
		.level		= &pch_gpio_set1_level,
		.blink		= &pch_gpio_set1_blink,
		.invert		= &pch_gpio_set1_invert,
		.reset		= &pch_gpio_set1_reset,
	},
	.set2 = {
		.mode		= &pch_gpio_set2_mode,
		.direction	= &pch_gpio_set2_direction,
		.level		= &pch_gpio_set2_level,
		.reset		= &pch_gpio_set2_reset,
	},
	.set3 = {
		.mode		= &pch_gpio_set3_mode,
		.direction	= &pch_gpio_set3_direction,
		.level		= &pch_gpio_set3_level,
		.reset		= &pch_gpio_set3_reset,
	},
};
