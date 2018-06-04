/*
 * This file is part of the coreboot project.
 *
 * Copyright 2013 Google Inc.
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

#include <soc/gpio.h>
#include <soc/wakeup.h>

int wakeup_need_reset(void)
{
	/* The "wake up" event is not reliable (known as "bad wakeup") and needs
	 * reset if the TPM reset mask GPIO value is high. */
	return gpio_get_value(GPIO_X06);
}
