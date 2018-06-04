/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Google Inc.
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

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <bootblock_common.h>
#include <soc/gpio.h>

static void early_config_gpio(void)
{
	const struct pad_config *early_gpio_table;
	size_t num_gpios = 0;

	early_gpio_table = variant_early_gpio_table(&num_gpios);
	gpio_configure_pads(early_gpio_table, num_gpios);
}

void bootblock_mainboard_init(void)
{
	early_config_gpio();
}
