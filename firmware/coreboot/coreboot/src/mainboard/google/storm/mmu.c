/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/cache.h>
#include <rules.h>
#include <soc/soc_services.h>
#include <symbols.h>
#include "mmu.h"

/* convenient shorthand (in MB) */
#define RPM_START           ((uintptr_t)_rpm / KiB)
#define RPM_END             ((uintptr_t)_erpm / KiB)
#define RPM_SIZE            (RPM_END - RPM_START)
#define SRAM_START          ((uintptr_t)_sram / KiB)
#define SRAM_END            ((uintptr_t)_esram / KiB)
#define DRAM_START          ((uintptr_t)_dram / MiB)
#define DRAM_SIZE           (CONFIG_DRAM_SIZE_MB)
#define DRAM_END            (DRAM_START + DRAM_SIZE)

/* DMA memory for drivers */
#define DMA_START            ((uintptr_t)_dma_coherent / MiB)
#define DMA_SIZE             (_dma_coherent_size / MiB)

void setup_dram_mappings(enum dram_state dram)
{
	if (dram == DRAM_INITIALIZED) {
		mmu_config_range(DRAM_START, DRAM_SIZE, DCACHE_WRITEBACK);
		/* Map DMA memory */
		mmu_config_range(DMA_START, DMA_SIZE, DCACHE_OFF);
#if ENV_ROMSTAGE
		/* Mark cbmem backing store as ready. */
		ipq_cbmem_backing_store_ready();
#endif
	} else {
		mmu_disable_range(DRAM_START, DRAM_SIZE);
		/* Map DMA memory */
		mmu_disable_range(DMA_START, DMA_SIZE);
	}
}

void setup_mmu(enum dram_state dram)
{
	dcache_mmu_disable();

	/* start with mapping everything as strongly ordered. */
	mmu_config_range(0, 4096, DCACHE_OFF);

	/* Map Device memory. */
	mmu_config_range_kb(RPM_START, RPM_SIZE, DCACHE_OFF);

	mmu_config_range_kb(SRAM_START, SRAM_END - SRAM_START,
		DCACHE_WRITEBACK);

	/* Map DRAM memory */
	setup_dram_mappings(dram);

	mmu_disable_range(DRAM_END, 4096 - DRAM_END);

	/* disable Page 0 for trapping NULL pointer references. */
	mmu_disable_range_kb(0, 1);

	mmu_init();

	dcache_mmu_enable();
}
