/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2016 Intel Corporation.
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

#include <arch/io.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <rules.h>
#include <gpio.h>
#include <soc/gpio.h>
#include <string.h>
#include <ec/google/chromeec/ec.h>
#include <vendorcode/google/chromeos/chromeos.h>

#include "gpio.h"
#include "ec.h"

#if ENV_RAMSTAGE
#include <boot/coreboot_tables.h>

void fill_lb_gpios(struct lb_gpios *gpios)
{
	struct lb_gpio chromeos_gpios[] = {
		{-1, ACTIVE_HIGH, get_write_protect_state(), "write protect"},
		{-1, ACTIVE_HIGH, get_recovery_mode_switch(), "recovery"},
		{-1, ACTIVE_HIGH, get_lid_switch(), "lid"},
		{-1, ACTIVE_HIGH, 0, "power"},
		{-1, ACTIVE_HIGH, gfx_get_init_done(), "oprom"},
	};
	lb_add_gpios(gpios, chromeos_gpios, ARRAY_SIZE(chromeos_gpios));
}
#endif /* ENV_RAMSTAGE */

int get_lid_switch(void)
{
	if (IS_ENABLED(CONFIG_EC_GOOGLE_CHROMEEC))
		/* Read lid switch state from the EC. */
		return !!(google_chromeec_get_switches() & EC_SWITCH_LID_OPEN);

	/* Lid always open */
	return 1;
}

int get_recovery_mode_switch(void)
{
	if (IS_ENABLED(CONFIG_EC_GOOGLE_CHROMEEC)) {
		/* Check for dedicated recovery switch first. */
		if (google_chromeec_get_switches() &
			EC_SWITCH_DEDICATED_RECOVERY)
		return 1;

		/* Otherwise check if the EC has posted the keyboard recovery
		 * event. */
		return !!(google_chromeec_get_events_b() &
			  EC_HOST_EVENT_MASK(EC_HOST_EVENT_KEYBOARD_RECOVERY));
	}

	return 0;
}

int clear_recovery_mode_switch(void)
{
	if (IS_ENABLED(CONFIG_EC_GOOGLE_CHROMEEC))
		/* Clear keyboard recovery event. */
		return google_chromeec_clear_events_b(
			EC_HOST_EVENT_MASK(EC_HOST_EVENT_KEYBOARD_RECOVERY));

	return 0;
}

int get_write_protect_state(void)
{
	/* No write protect */
	return 0;
}

static const struct cros_gpio cros_gpios[] = {
	CROS_GPIO_REC_AL(CROS_GPIO_VIRTUAL, CROS_GPIO_DEVICE_NAME),
	CROS_GPIO_WP_AH(CROS_GPIO_VIRTUAL, CROS_GPIO_DEVICE_NAME),
};

void mainboard_chromeos_acpi_generate(void)
{
	chromeos_acpi_gpio_generate(cros_gpios, ARRAY_SIZE(cros_gpios));
}
