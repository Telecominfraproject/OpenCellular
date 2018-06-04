/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Patrick Rudolph <siro@das-labor.org>
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

#include <southbridge/intel/common/gpio.h>
#include <console/console.h>
#include <device/device.h>
#include <ec/acpi/ec.h>
#include <option.h>

#include "h8.h"
#include "chip.h"

/* Controls radio-off pin in WWAN MiniPCIe slot.  */
void h8_wwan_enable(int on)
{
	if (on)
		ec_set_bit(0x3a, 6);
	else
		ec_clr_bit(0x3a, 6);
}

/*
 * Detect WWAN on supported MBs.
 */
bool h8_has_wwan(struct device *dev)
{
	struct ec_lenovo_h8_config *conf = dev->chip_info;

	if (!conf->has_wwan_detection) {
		printk(BIOS_INFO, "H8: WWAN detection not implemented. "
				  "Assuming WWAN installed\n");
		return true;
	}

	if (get_gpio(conf->wwan_gpio_num) == conf->wwan_gpio_lvl) {
		printk(BIOS_INFO, "H8: WWAN installed\n");
		return true;
	}

	printk(BIOS_INFO, "H8: WWAN not installed\n");
	return false;
}

/*
 * Return WWAN NVRAM setting.
 */
bool h8_wwan_nv_enable(void)
{
	u8 val;

	if (get_option(&val, "wwan") != CB_SUCCESS)
		return true;

	return val;
}
