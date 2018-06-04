/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
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
#include <arch/acpi.h>
#include <cbmem.h>
#include <console/console.h>
#include <console/streams.h>
#include <cpu/x86/tsc.h>
#include <program_loading.h>
#include <rmodule.h>
#include <stage_cache.h>

#include <soc/ramstage.h>
#include <soc/efi_wrapper.h>

static void ABI_X86 send_to_console(unsigned char b)
{
	console_tx_byte(b);
}

static efi_wrapper_entry_t load_refcode_from_cache(void)
{
	struct prog refcode;

	printk(BIOS_DEBUG, "refcode loading from cache.\n");

	stage_cache_load_stage(STAGE_REFCODE, &refcode);

	return (efi_wrapper_entry_t)prog_entry(&refcode);
}

static efi_wrapper_entry_t load_reference_code(void)
{
	struct prog prog =
		PROG_INIT(PROG_REFCODE, CONFIG_CBFS_PREFIX "/refcode");
	struct rmod_stage_load refcode = {
		.cbmem_id = CBMEM_ID_REFCODE,
		.prog = &prog,
	};

	if (acpi_is_wakeup_s3()) {
		return load_refcode_from_cache();
	}

	if (prog_locate(&prog)) {
		printk(BIOS_DEBUG, "Couldn't locate reference code.\n");
		return NULL;
	}

	if (rmodule_stage_load(&refcode)) {
		printk(BIOS_DEBUG, "Error loading reference code.\n");
		return NULL;
	}

	/* Cache loaded reference code. */
	stage_cache_add(STAGE_REFCODE, &prog);

	return prog_entry(&prog);
}

void baytrail_run_reference_code(void)
{
	int ret;
	efi_wrapper_entry_t entry;
	struct efi_wrapper_params wrp = {
		.version = EFI_WRAPPER_VER,
		.console_out = send_to_console,
	};

	entry = load_reference_code();

	if (entry == NULL)
		return;

	wrp.tsc_ticks_per_microsecond = tsc_freq_mhz();

	/* Call into reference code. */
	ret = entry(&wrp);

	if (ret != 0) {
		printk(BIOS_DEBUG, "Reference code returned %d\n", ret);
		return;
	}
}
