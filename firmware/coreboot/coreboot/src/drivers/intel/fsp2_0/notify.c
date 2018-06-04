/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2016 Intel Corp.
 * (Written by Andrey Petrov <andrey.petrov@intel.com> for Intel Corp.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <arch/cpu.h>
#include <bootstate.h>
#include <compiler.h>
#include <console/console.h>
#include <fsp/util.h>
#include <soc/intel/common/util.h>
#include <string.h>
#include <timestamp.h>

static void fsp_notify(enum fsp_notify_phase phase)
{
	uint32_t ret;
	fsp_notify_fn fspnotify;
	struct fsp_notify_params notify_params = { .phase = phase };

	if (!fsps_hdr.notify_phase_entry_offset)
		die("Notify_phase_entry_offset is zero!\n");

	fspnotify = (void *) (fsps_hdr.image_base +
			    fsps_hdr.notify_phase_entry_offset);
	fsp_before_debug_notify(fspnotify, &notify_params);

	if (phase == AFTER_PCI_ENUM) {
		timestamp_add_now(TS_FSP_BEFORE_ENUMERATE);
		post_code(POST_FSP_NOTIFY_BEFORE_ENUMERATE);
	} else if (phase == READY_TO_BOOT) {
		timestamp_add_now(TS_FSP_BEFORE_FINALIZE);
		post_code(POST_FSP_NOTIFY_BEFORE_FINALIZE);
	} else if (phase == END_OF_FIRMWARE) {
		timestamp_add_now(TS_FSP_BEFORE_END_OF_FIRMWARE);
		post_code(POST_FSP_NOTIFY_BEFORE_END_OF_FIRMWARE);
	}

	ret = fspnotify(&notify_params);

	if (phase == AFTER_PCI_ENUM) {
		timestamp_add_now(TS_FSP_AFTER_ENUMERATE);
		post_code(POST_FSP_NOTIFY_BEFORE_ENUMERATE);
	} else if (phase == READY_TO_BOOT) {
		timestamp_add_now(TS_FSP_AFTER_FINALIZE);
		post_code(POST_FSP_NOTIFY_BEFORE_FINALIZE);
	} else if (phase == END_OF_FIRMWARE) {
		timestamp_add_now(TS_FSP_AFTER_END_OF_FIRMWARE);
		post_code(POST_FSP_NOTIFY_AFTER_END_OF_FIRMWARE);
	}
	fsp_debug_after_notify(ret);

	/* Handle any errors returned by FspNotify */
	fsp_handle_reset(ret);
	if (ret != FSP_SUCCESS) {
		printk(BIOS_SPEW, "FspNotify returned 0x%08x\n", ret);
		die("FspNotify returned an error!\n");
	}

	/* Allow the platform to run something after FspNotify */
	platform_fsp_notify_status(phase);
}

static void fsp_notify_dummy(void *arg)
{
	enum fsp_notify_phase phase = (uint32_t)arg;

	/* Display the MTRRs */
	if (IS_ENABLED(CONFIG_DISPLAY_MTRRS))
		soc_display_mtrrs();

	fsp_notify(phase);
	if (phase == READY_TO_BOOT)
		fsp_notify(END_OF_FIRMWARE);
}

BOOT_STATE_INIT_ENTRY(BS_DEV_ENABLE, BS_ON_ENTRY, fsp_notify_dummy,
						(void *) AFTER_PCI_ENUM);
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, fsp_notify_dummy,
						(void *) READY_TO_BOOT);
BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, fsp_notify_dummy,
						(void *) READY_TO_BOOT);

__weak void platform_fsp_notify_status(
	enum fsp_notify_phase phase)
{
}
