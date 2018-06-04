/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 The ChromiumOS Authors.  All rights reserved.
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

#include <bootstate.h>
#include <console/console.h>
#include <elog.h>
#include <security/vboot/vboot_common.h>

#if IS_ENABLED(CONFIG_HAVE_ACPI_RESUME)
#include <arch/acpi.h>
#endif

static void elog_add_boot_reason(void *unused)
{
	int rec = vboot_recovery_mode_enabled();
	int dev = vboot_developer_mode_enabled();

	if (!rec && !dev)
		return;

	if (rec) {
		u8 reason = vboot_check_recovery_request();
		elog_add_event_byte(ELOG_TYPE_CROS_RECOVERY_MODE, reason);
		printk(BIOS_DEBUG, "%s: Logged recovery mode boot%s, "
		       "reason: 0x%02x\n", __func__,
		       dev ? " (Dev-switch on)" : "", reason);
	}

	if (dev) {
		int log_event = 1;

#if IS_ENABLED(CONFIG_HAVE_ACPI_RESUME)
		/* Skip logging developer mode in ACPI resume path */
		if (acpi_is_wakeup())
			log_event = 0;
#endif

		if (log_event) {
			elog_add_event(ELOG_TYPE_CROS_DEVELOPER_MODE);
			printk(BIOS_DEBUG, "%s: Logged dev mode boot\n",
			       __func__);
		}
	}
}

BOOT_STATE_INIT_ENTRY(BS_POST_DEVICE, BS_ON_ENTRY, elog_add_boot_reason, NULL);
