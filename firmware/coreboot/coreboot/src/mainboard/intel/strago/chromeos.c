/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
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

#include <rules.h>
#include <gpio.h>
#include <string.h>
#include <vendorcode/google/chromeos/chromeos.h>

#define WP_GPIO	GP_E_22

#if ENV_RAMSTAGE
#include <boot/coreboot_tables.h>

#define ACTIVE_LOW	0
#define ACTIVE_HIGH	1

void fill_lb_gpios(struct lb_gpios *gpios)
{
	struct lb_gpio chromeos_gpios[] = {
		{-1, ACTIVE_HIGH, get_write_protect_state(), "write protect"},
		{-1, ACTIVE_HIGH, vboot_recovery_mode_enabled(), "recovery"},
		{-1, ACTIVE_HIGH, get_lid_switch(), "lid"},
		{-1, ACTIVE_HIGH, 0, "power"},
		{-1, ACTIVE_HIGH, gfx_get_init_done(), "oprom"},
	};
	lb_add_gpios(gpios, chromeos_gpios, ARRAY_SIZE(chromeos_gpios));
}
#endif /* ENV_RAMSTAGE */

int get_write_protect_state(void)
{
	/*
	 * The vboot loader queries this function in romstage. The GPIOs have
	 * not been set up yet as that configuration is done in ramstage.
	 * Configuring this GPIO as input so that there isn't any ambiguity
	 * in the reading.
	 */
#if ENV_ROMSTAGE
	 gpio_input_pullup(WP_GPIO);
#endif

	/* WP is enabled when the pin is reading high. */
	return !!gpio_get(WP_GPIO);
}

static const struct cros_gpio cros_gpios[] = {
	CROS_GPIO_REC_AL(CROS_GPIO_VIRTUAL, CROS_GPIO_DEVICE_NAME),
	CROS_GPIO_WP_AH(0x10013, CROS_GPIO_DEVICE_NAME),
};

void mainboard_chromeos_acpi_generate(void)
{
	chromeos_acpi_gpio_generate(cros_gpios, ARRAY_SIZE(cros_gpios));
}
