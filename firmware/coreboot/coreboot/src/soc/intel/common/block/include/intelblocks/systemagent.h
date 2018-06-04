/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Intel Corporation.
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

#ifndef SOC_INTEL_COMMON_BLOCK_SA_H
#define SOC_INTEL_COMMON_BLOCK_SA_H

#include <device/device.h>
#include <soc/iomap.h>
#include <stddef.h>

/* Device 0:0.0 PCI configuration space */
#define MCHBAR		0x48
#define PCIEXBAR		0x60
#define TOUUD	0xa8 /* Top of Upper Usable DRAM */
#define BDSM	0xb0 /* Base Data Stolen Memory */
#define BGSM	0xb4 /* Base GTT Stolen Memory */
#define TSEG	0xb8 /* TSEG base */
#define TOLUD	0xbc /* Top of Low Used Memory */

/* MCHBAR */
#define MCHBAR8(x)	(*(volatile u8 *)(MCH_BASE_ADDRESS + x))
#define MCHBAR16(x)	(*(volatile u16 *)(MCH_BASE_ADDRESS + x))
#define MCHBAR32(x)	(*(volatile u32 *)(MCH_BASE_ADDRESS + x))

/* Perform System Agent Initialization during Bootblock phase */
void bootblock_systemagent_early_init(void);

/*
 * Fixed MMIO range
 *   INDEX = Either PCI configuration space registers or MMIO offsets
 *   mapped from REG.
 *   BASE = 32 bit Address.
 *   SIZE = base length
 *   DESCRIPTION = Name of the register/offset.
 */
struct sa_mmio_descriptor {
	unsigned int index;
	uintptr_t base;
	size_t size;
	const char *description;
};

/* API to set Fixed MMIO address into PCI configuration space */
void sa_set_pci_bar(const struct sa_mmio_descriptor *fixed_set_resources,
		size_t count);
/* API to set Fixed MMIO address into MCH base address */
void sa_set_mch_bar(const struct sa_mmio_descriptor *fixed_set_resources,
		size_t count);
/*
 * API to program fixed mmio resource range based on SoC input
 * struct sa_mmio_descriptor
 */
void sa_add_fixed_mmio_resources(struct device *dev, int *resource_cnt,
	const struct sa_mmio_descriptor *sa_fixed_resources, size_t count);
/*
 * API to set BIOS Reset CPL through MCHBAR
 * SoC to provide BIOS_RESET_CPL register offset through soc/systemagent.h
 */
void enable_bios_reset_cpl(void);
/* API to enable PAM registers */
void enable_pam_region(void);
/* API to enable Power Aware Interrupt Routing through MCHBAR */
void enable_power_aware_intr(void);
/* API to get TOLUD base address */
uintptr_t sa_get_tolud_base(void);
/* API to get DSM size */
size_t sa_get_dsm_size(void);
/* API to get GSM size */
size_t sa_get_gsm_size(void);
/* API to get TSEG base address */
uintptr_t sa_get_tseg_base(void);
/* API to get TSEG size */
size_t sa_get_tseg_size(void);
/* API to get DPR size */
size_t sa_get_dpr_size(void);
/*
 * SoC overrides
 *
 * All new SoC must implement below functionality for ramstage.
 */

/* Perform System Agent Initialization during Ramstage phase */
void soc_systemagent_init(struct device *dev);
/*
 * SoC call to provide all known fixed memory ranges for Device 0:0.0.
 * SoC function should provide fixed resource ranges in form of
 * struct sa_mmio_descriptor along with resource count.
 */
void soc_add_fixed_mmio_resources(struct device *dev, int *resource_cnt);

/* SoC specific APIs to get UNCORE PRMRR base and mask values
 * returns 0, if able to get base and mask values; otherwise returns -1 */
int soc_get_uncore_prmmr_base_and_mask(uint64_t *base, uint64_t *mask);

/* SoC call to summarize all Intel Reserve MMIO size and report to SA */
size_t soc_reserved_mmio_size(void);

#endif	/* SOC_INTEL_COMMON_BLOCK_SA_H */
