/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 The Chromium OS Authors. All rights reserved.
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

#include <string.h>
#include <bootmode.h>
#include <arch/io.h>
#include <device/device.h>
#include <device/pci.h>
#include <southbridge/intel/bd82x6x/pch.h>
#include <southbridge/intel/common/gpio.h>
#include <vendorcode/google/chromeos/chromeos.h>

#ifndef __PRE_RAM__
#include <boot/coreboot_tables.h>

#define GPIO_COUNT	6

void fill_lb_gpios(struct lb_gpios *gpios)
{
	device_t dev = dev_find_slot(0, PCI_DEVFN(0x1f,0));
	u16 gpio_base = pci_read_config32(dev, GPIOBASE) & 0xfffe;

	if (!gpio_base)
		return;

	u32 gp_lvl = inl(gpio_base + 0x0c);
	u32 gp_lvl2 = inl(gpio_base + 0x38);
	/* u32 gp_lvl3 = inl(gpio_base + 0x48); */

	gpios->size = sizeof(*gpios) + (GPIO_COUNT * sizeof(struct lb_gpio));
	gpios->count = GPIO_COUNT;

	/* Write Protect: GPIO48 */
	gpios->gpios[0].port = 48;
	gpios->gpios[0].polarity = ACTIVE_LOW;
	gpios->gpios[0].value = (gp_lvl2 >> (48-32)) & 1;
	strncpy((char *)gpios->gpios[0].name,"write protect",
							GPIO_MAX_NAME_LENGTH);

	/* Recovery: GPIO22 */
	gpios->gpios[1].port = 22;
	gpios->gpios[1].polarity = ACTIVE_LOW;
	gpios->gpios[1].value = (gp_lvl >> 22) & 1;
	strncpy((char *)gpios->gpios[1].name,"recovery", GPIO_MAX_NAME_LENGTH);

	/* Hard code the lid switch GPIO to open. */
	gpios->gpios[3].port = -1;
	gpios->gpios[3].polarity = ACTIVE_HIGH;
	gpios->gpios[3].value = 1;
	strncpy((char *)gpios->gpios[3].name,"lid", GPIO_MAX_NAME_LENGTH);

	/* Power Button */
	gpios->gpios[4].port = -1;
	gpios->gpios[4].polarity = ACTIVE_HIGH;
	gpios->gpios[4].value = 0;
	strncpy((char *)gpios->gpios[4].name,"power", GPIO_MAX_NAME_LENGTH);

	/* Did we load the VGA option ROM? */
	gpios->gpios[5].port = -1;
	gpios->gpios[5].polarity = ACTIVE_HIGH;
	gpios->gpios[5].value = gfx_get_init_done();
	strncpy((char *)gpios->gpios[5].name,"oprom", GPIO_MAX_NAME_LENGTH);
}
#endif

int get_recovery_mode_switch(void)
{
	/* Recovery: GPIO22, active low */
	return !get_gpio(22);
}

static const struct cros_gpio cros_gpios[] = {
	CROS_GPIO_REC_AL(22, CROS_GPIO_DEVICE_NAME),
	CROS_GPIO_DEV_AH(57, CROS_GPIO_DEVICE_NAME),
	CROS_GPIO_WP_AL(48, CROS_GPIO_DEVICE_NAME),
};

void mainboard_chromeos_acpi_generate(void)
{
	chromeos_acpi_gpio_generate(cros_gpios, ARRAY_SIZE(cros_gpios));
}
