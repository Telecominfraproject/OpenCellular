/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Google Inc.
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
#include <assert.h>
#include <console/console.h>
#include <fsp/util.h>
#include <soc/cnl_lpddr4_init.h>
#include <spd_bin.h>
#include <string.h>

static void meminit_lpddr4(FSP_M_CONFIG *mem_cfg,
			const struct lpddr4_cfg *board_cfg,
			size_t spd_data_len, uintptr_t spd_data_ptr)
{
	/*
	 * DqByteMapChx expects 12 bytes of data, but the last 6 bytes
	 * are unused, so client passes in the relevant values and
	 * we null out the rest of the data.
	 */
	memset(&mem_cfg->DqByteMapCh0, 0, sizeof(mem_cfg->DqByteMapCh0));
	memcpy(&mem_cfg->DqByteMapCh0, &board_cfg->dq_map[LP4_CH0],
		sizeof(board_cfg->dq_map[LP4_CH0]));

	memset(&mem_cfg->DqByteMapCh1, 0, sizeof(mem_cfg->DqByteMapCh1));
	memcpy(&mem_cfg->DqByteMapCh1, &board_cfg->dq_map[LP4_CH1],
		sizeof(board_cfg->dq_map[LP4_CH1]));

	memcpy(&mem_cfg->DqsMapCpu2DramCh0, &board_cfg->dqs_map[LP4_CH0],
		sizeof(board_cfg->dqs_map[LP4_CH0]));
	memcpy(&mem_cfg->DqsMapCpu2DramCh1, &board_cfg->dqs_map[LP4_CH1],
		sizeof(board_cfg->dqs_map[LP4_CH1]));

	memcpy(&mem_cfg->RcompResistor, &board_cfg->rcomp_resistor,
		sizeof(mem_cfg->RcompResistor));

	/* Early cannonlake requires rcomp targets to be 0 */
	memcpy(&mem_cfg->RcompTarget, &board_cfg->rcomp_targets,
		sizeof(mem_cfg->RcompTarget));

	mem_cfg->MemorySpdDataLen = spd_data_len;
	mem_cfg->MemorySpdPtr00 = spd_data_ptr;

	/* Use the same spd data for channel 1, Dimm 0 */
	mem_cfg->MemorySpdPtr10 = mem_cfg->MemorySpdPtr00;
}

/*
 * Initialize default LPDDR4 settings using spd data contained in a buffer.
 */
static void meminit_lpddr4_spd_data(FSP_M_CONFIG *mem_cfg,
				const struct lpddr4_cfg *cnl_cfg,
				size_t spd_data_len, uintptr_t spd_data_ptr)
{
	assert(spd_data_ptr && spd_data_len);
	meminit_lpddr4(mem_cfg, cnl_cfg, spd_data_len, spd_data_ptr);
}

/*
 * Initialize default LPDDR4 settings using the spd file specified by
 * spd_index. The spd_index is an index into the SPD_SOURCES array defined
 * in spd/Makefile.inc.
 */
static void meminit_lpddr4_cbfs_spd_index(FSP_M_CONFIG *mem_cfg,
					const struct lpddr4_cfg *cnl_cfg,
					int spd_index)
{
	size_t spd_data_len;
	uintptr_t spd_data_ptr;
	struct region_device spd_rdev;

	printk(BIOS_DEBUG, "SPD INDEX = %d\n", spd_index);
	if (get_spd_cbfs_rdev(&spd_rdev, spd_index) < 0)
		die("spd.bin not found or incorrect index\n");
	spd_data_len = region_device_sz(&spd_rdev);
	/* Memory leak is ok since we have memory mapped boot media */
	assert(IS_ENABLED(CONFIG_BOOT_DEVICE_MEMORY_MAPPED));
	spd_data_ptr = (uintptr_t)rdev_mmap_full(&spd_rdev);
	meminit_lpddr4_spd_data(mem_cfg, cnl_cfg, spd_data_len, spd_data_ptr);
}

/* Initialize LPDDR4 settings for CannonLake */
void cannonlake_lpddr4_init(FSP_M_CONFIG *mem_cfg,
			const struct lpddr4_cfg *cnl_cfg,
			const struct spd_info *spd)
{
	/* Early Command Training Enabled */
	mem_cfg->ECT = cnl_cfg->ect;
	mem_cfg->DqPinsInterleaved = cnl_cfg->dq_pins_interleaved;
	mem_cfg->RefClk = 0; /* Auto Select CLK freq */
	mem_cfg->CaVrefConfig = 0; /* VREF_CA->CHA/CHB */

	if (spd->spd_by_index) {
		meminit_lpddr4_cbfs_spd_index(mem_cfg, cnl_cfg,
				spd->spd_spec.spd_index);
	} else {
		meminit_lpddr4_spd_data(mem_cfg, cnl_cfg,
				spd->spd_spec.spd_data_ptr_info.spd_data_len,
				spd->spd_spec.spd_data_ptr_info.spd_data_ptr);
	}
}
