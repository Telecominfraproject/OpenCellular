/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
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

// Use simple device model for this file even in ramstage
#define __SIMPLE_DEVICE__

#include <arch/io.h>
#include <arch/cpu.h>
#include <cbmem.h>
#include "i945.h"
#include <console/console.h>
#include <cpu/intel/romstage.h>
#include <cpu/x86/mtrr.h>
#include <program_loading.h>

static uintptr_t smm_region_start(void)
{
	uintptr_t tom;

	if (pci_read_config8(PCI_DEV(0, 0x0, 0), DEVEN) & (DEVEN_D2F0 | DEVEN_D2F1))
		/* IGD enabled, get top of Memory from BSM register */
		tom = pci_read_config32(PCI_DEV(0, 2, 0), BSM);
	else
		tom = (pci_read_config8(PCI_DEV(0, 0, 0), TOLUD) & 0xf7) << 24;

	/* if TSEG enabled subtract size */
	switch (pci_read_config8(PCI_DEV(0, 0, 0), ESMRAMC) & 0x07) {
	case 0x01:
		/* 1MB TSEG */
		tom -= 0x100000;
		break;
	case 0x03:
		/* 2MB TSEG */
		tom -= 0x200000;
		break;
	case 0x05:
		/* 8MB TSEG */
		tom -= 0x800000;
		break;
	default:
		/* TSEG either disabled or invalid */
		break;
	}
	return tom;
}

/* Depending of UMA and TSEG configuration, TSEG might start at any
 * 1 MiB aligment. As this may cause very greedy MTRR setup, push
 * CBMEM top downwards to 4 MiB boundary.
 */
void *cbmem_top(void)
{
	uintptr_t top_of_ram = ALIGN_DOWN(smm_region_start(), 4*MiB);
	return (void *) top_of_ram;
}

/** Decodes used Graphics Mode Select (GMS) to kilobytes. */
u32 decode_igd_memory_size(const u32 gms)
{
	static const u16 ggc2uma[] = { 0, 1, 4, 8, 16, 32,
			48, 64 };

	if (gms > ARRAY_SIZE(ggc2uma))
		die("Bad Graphics Mode Select (GMS) setting.\n");

	return ggc2uma[gms] << 10;
}

#define ROMSTAGE_RAM_STACK_SIZE 0x5000

/* setup_stack_and_mtrrs() determines the stack to use after
 * cache-as-ram is torn down as well as the MTRR settings to use. */
void *setup_stack_and_mtrrs(void)
{
	struct postcar_frame pcf;
	uintptr_t top_of_ram;

	if (postcar_frame_init(&pcf, ROMSTAGE_RAM_STACK_SIZE))
		die("Unable to initialize postcar frame.\n");

	/* Cache the ROM as WP just below 4GiB. */
	postcar_frame_add_mtrr(&pcf, CACHE_ROM_BASE, CACHE_ROM_SIZE,
		MTRR_TYPE_WRPROT);

	/* Cache RAM as WB from 0 -> CACHE_TMP_RAMTOP. */
	postcar_frame_add_mtrr(&pcf, 0, CACHE_TMP_RAMTOP, MTRR_TYPE_WRBACK);

	/* Cache two separate 4 MiB regions below the top of ram, this
	 * satisfies MTRR alignment requirements. If you modify this to
	 * cover TSEG, make sure UMA region is not set with WRBACK as it
	 * causes hard-to-recover boot failures.
	 */
	top_of_ram = (uintptr_t)cbmem_top();
	postcar_frame_add_mtrr(&pcf, top_of_ram - 4*MiB, 4*MiB, MTRR_TYPE_WRBACK);
	postcar_frame_add_mtrr(&pcf, top_of_ram - 8*MiB, 4*MiB, MTRR_TYPE_WRBACK);

	/* Save the number of MTRRs to setup. Return the stack location
	 * pointing to the number of MTRRs.
	 */
	return postcar_commit_mtrrs(&pcf);
}
