/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Google Inc.
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

#include <soc/romstage.h>
#include <string.h>
#include <spd_bin.h>

#include <fsp/soc_binding.h>

void mainboard_memory_init_params(FSPM_UPD *mupd)
{
	FSP_M_CONFIG *mem_cfg = &mupd->FspmConfig;
	const FSPM_ARCH_UPD *arch_upd = &mupd->FspmArchUpd;
	/* Rcomp resistor */
	const u16 rcomp_resistor[] = { 121, 81, 100 };
	/* Rcomp target */
	const u16 rcomp_target[] = { 100, 40, 20, 20, 26 };

	/* SPD was saved in S0/S5 path, skips it when resumes from S3 */
	if (arch_upd->BootMode == FSP_BOOT_ON_S3_RESUME)
		return;

	memcpy(&mem_cfg->RcompResistor, rcomp_resistor, sizeof(rcomp_resistor));
	memcpy(&mem_cfg->RcompTarget, rcomp_target, sizeof(rcomp_target));

	/* Read spd block to get memory config */
	struct spd_block blk = {
		.addr_map = { 0x50, 0x52, },
	};
	mem_cfg->DqPinsInterleaved = 1;
	get_spd_smbus(&blk);
	mem_cfg->MemorySpdDataLen = blk.len;
	mem_cfg->MemorySpdPtr00 = (uintptr_t)blk.spd_array[0];
	mem_cfg->MemorySpdPtr10 = (uintptr_t)blk.spd_array[1];

	dump_spd_info(&blk);
}
