/*
 * This file is part of the coreboot project.
 *
 * Copyright 2018 Google Inc.
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

#ifndef __DRIVERS_GENERIC_GPIO_KEYS_H__
#define __DRIVERS_GENERIC_GPIO_KEYS_H__

#include <arch/acpi_device.h>
#include <stdint.h>

/* Linux input type */
enum {
	/* Switch event */
	EV_SW = 0x5,
};

/* Switch events type (Linux code emitted for EV_SW) */
enum {
	SW_PEN_INSERTED = 0xf,
};

/* Details of the child node defining key */
struct key_info {
	/* Device name of the child node - Mandatory */
	const char *dev_name;
	/* Keycode emitted for this key - Mandatory */
	uint32_t linux_code;
	/*
	 * Event type generated for this key
	 * See EV_* above.
	 */
	uint32_t linux_input_type;
	/* Descriptive name of the key */
	const char *label;
	/* Can this key wake-up the system? */
	bool is_wakeup_source;
	/* Can this key be disabled? */
	bool can_be_disabled;
	/* Debounce interval time in milliseconds */
	uint32_t debounce_interval;
};

struct drivers_generic_gpio_keys_config {
	/* Device name of the parent gpio-keys node */
	const char *name;
	/* GPIO line providing the key - Mandatory */
	struct acpi_gpio gpio;
	/* Is this a polled GPIO button? - Optional */
	bool is_polled;
	/* Poll inverval - Mandatory only if GPIO is polled. */
	uint32_t poll_interval;
	/* Details about the key - Mandatory */
	struct key_info key;
};

#endif /* __DRIVERS_GENERIC_GPIO_KEYS_H__ */
