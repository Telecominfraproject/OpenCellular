/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
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

#include <stdint.h>
#include <arch/cpu.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <arch/io.h>
#include <halt.h>
#include <cpu/intel/microcode/microcode.c>
#include <soc/rcba.h>
#include <soc/msr.h>

static void set_var_mtrr(unsigned int reg, unsigned int base, unsigned int size,
	unsigned int type)
{
	/* Bit Bit 32-35 of MTRRphysMask should be set to 1 */
	msr_t basem, maskm;
	basem.lo = base | type;
	basem.hi = 0;
	wrmsr(MTRR_PHYS_BASE(reg), basem);
	maskm.lo = ~(size - 1) | MTRR_PHYS_MASK_VALID;
	maskm.hi = (1 << (CONFIG_CPU_ADDR_BITS - 32)) - 1;
	wrmsr(MTRR_PHYS_MASK(reg), maskm);
}

static void enable_rom_caching(void)
{
	msr_t msr;

	disable_cache();
	/* Why only top 4MiB ? */
	set_var_mtrr(1, CACHE_ROM_BASE, CACHE_ROM_SIZE, MTRR_TYPE_WRPROT);
	enable_cache();

	/* Enable Variable MTRRs */
	msr.hi = 0x00000000;
	msr.lo = 0x00000800;
	wrmsr(MTRR_DEF_TYPE_MSR, msr);
}

static void bootblock_mdelay(int ms)
{
	u32 target = ms * 24 * 1000;
	msr_t current;
	msr_t start = rdmsr(MSR_COUNTER_24_MHZ);

	do {
		current = rdmsr(MSR_COUNTER_24_MHZ);
	} while ((current.lo - start.lo) < target);
}

static void set_flex_ratio_to_tdp_nominal(void)
{
	msr_t flex_ratio, msr;
	u32 soft_reset;
	u8 nominal_ratio;

	/* Check for Flex Ratio support */
	flex_ratio = rdmsr(MSR_FLEX_RATIO);
	if (!(flex_ratio.lo & FLEX_RATIO_EN))
		return;

	/* Check for >0 configurable TDPs */
	msr = rdmsr(MSR_PLATFORM_INFO);
	if (((msr.hi >> 1) & 3) == 0)
		return;

	/* Use nominal TDP ratio for flex ratio */
	msr = rdmsr(MSR_CONFIG_TDP_NOMINAL);
	nominal_ratio = msr.lo & 0xff;

	/* See if flex ratio is already set to nominal TDP ratio */
	if (((flex_ratio.lo >> 8) & 0xff) == nominal_ratio)
		return;

	/* Set flex ratio to nominal TDP ratio */
	flex_ratio.lo &= ~0xff00;
	flex_ratio.lo |= nominal_ratio << 8;
	flex_ratio.lo |= FLEX_RATIO_LOCK;
	wrmsr(MSR_FLEX_RATIO, flex_ratio);

	/* Set flex ratio in soft reset data register bits 11:6.
	 * RCBA region is enabled in southbridge bootblock */
	soft_reset = RCBA32(SOFT_RESET_DATA);
	soft_reset &= ~(0x3f << 6);
	soft_reset |= (nominal_ratio & 0x3f) << 6;
	RCBA32(SOFT_RESET_DATA) = soft_reset;

	/* Set soft reset control to use register value */
	RCBA32_OR(SOFT_RESET_CTRL, 1);

	/* Delay before reset to avoid potential TPM lockout */
	bootblock_mdelay(30);

	/* Issue warm reset, will be "CPU only" due to soft reset data */
	outb(0x0, 0xcf9);
	outb(0x6, 0xcf9);
	halt();
}

static void check_for_clean_reset(void)
{
	msr_t msr;
	msr = rdmsr(MTRR_DEF_TYPE_MSR);

	/* Use the MTRR default type MSR as a proxy for detecting INIT#.
	 * Reset the system if any known bits are set in that MSR. That is
	 * an indication of the CPU not being properly reset. */
	if (msr.lo & (MTRR_DEF_TYPE_EN | MTRR_DEF_TYPE_FIX_EN)) {
		outb(0x0, 0xcf9);
		outb(0x6, 0xcf9);
		halt();
	}
}

static void bootblock_cpu_init(void)
{
	/* Set flex ratio and reset if needed */
	set_flex_ratio_to_tdp_nominal();
	check_for_clean_reset();
	enable_rom_caching();
	intel_update_microcode_from_cbfs();
}
