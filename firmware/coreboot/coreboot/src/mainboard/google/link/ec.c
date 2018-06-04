/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Google Inc.
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

#include <arch/acpi.h>
#include <types.h>
#include <console/console.h>
#include <ec/google/chromeec/ec.h>
#include "ec.h"

void link_ec_init(void)
{
	const struct google_chromeec_event_info info = {
		.log_events = LINK_EC_LOG_EVENTS,
		.sci_events = LINK_EC_SCI_EVENTS,
		.s3_wake_events = LINK_EC_S3_WAKE_EVENTS,
		.s5_wake_events = LINK_EC_S5_WAKE_EVENTS,
	};

	printk(BIOS_DEBUG, "link_ec_init\n");
	post_code(0xf0);

	google_chromeec_events_init(&info, acpi_is_wakeup_s3());

	post_code(0xf1);
}
