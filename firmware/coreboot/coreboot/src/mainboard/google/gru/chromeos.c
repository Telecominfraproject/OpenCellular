/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Rockchip Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <bootmode.h>
#include <boot/coreboot_tables.h>
#include <gpio.h>
#include <security/tpm/tis.h>

#include "board.h"

static const uint32_t wp_polarity = IS_ENABLED(CONFIG_GRU_BASEBOARD_SCARLET) ?
				    ACTIVE_LOW : ACTIVE_HIGH;

int get_write_protect_state(void)
{
	int raw = gpio_get(GPIO_WP);
	return wp_polarity == ACTIVE_HIGH ? raw : !raw;
}

void fill_lb_gpios(struct lb_gpios *gpios)
{
	struct lb_gpio chromeos_gpios[] = {
		{GPIO_WP.raw, wp_polarity, gpio_get(GPIO_WP),
		 "write protect"},
		{-1, ACTIVE_HIGH, get_recovery_mode_switch(), "recovery"},
#if IS_ENABLED(CONFIG_GRU_BASEBOARD_SCARLET)
		{GPIO_BACKLIGHT.raw, ACTIVE_HIGH, -1, "backlight"},
#endif
		{GPIO_EC_IN_RW.raw, ACTIVE_HIGH, -1, "EC in RW"},
		{GPIO_EC_IRQ.raw, ACTIVE_LOW, -1, "EC interrupt"},
		{GPIO_RESET.raw, ACTIVE_HIGH, -1, "reset"},
		{GPIO_SPK_PA_EN.raw, ACTIVE_HIGH, -1, "speaker enable"},
#if IS_ENABLED(CONFIG_GRU_HAS_TPM2)
		{GPIO_TPM_IRQ.raw, ACTIVE_HIGH, -1, "TPM interrupt"},
#endif
	};

	lb_add_gpios(gpios, chromeos_gpios, ARRAY_SIZE(chromeos_gpios));
}

void setup_chromeos_gpios(void)
{
	if (IS_ENABLED(CONFIG_GRU_BASEBOARD_SCARLET))
		gpio_input(GPIO_WP);
	else
		gpio_input_pullup(GPIO_WP);
	gpio_input_pullup(GPIO_EC_IN_RW);
	gpio_input_pullup(GPIO_EC_IRQ);
}

#if IS_ENABLED(CONFIG_GRU_HAS_TPM2)
int tis_plat_irq_status(void)
{
	return gpio_irq_status(GPIO_TPM_IRQ);
}
#endif
