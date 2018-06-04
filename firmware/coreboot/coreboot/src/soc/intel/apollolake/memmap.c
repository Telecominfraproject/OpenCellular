/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Intel Corp.
 * (Written by Andrey Petrov <andrey.petrov@intel.com> for Intel Corp.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * The device_t returned by dev_find_slot() is different than the device_t
 * passed to pci_write_config32(). If one needs to get access to the config.h
 * of a device and perform i/o things are incorrect. One is a pointer while
 * the other is a 32-bit integer.
 */
#define __SIMPLE_DEVICE__

#include <arch/io.h>
#include <assert.h>
#include <cbmem.h>
#include "chip.h"
#include <device/pci.h>
#include <fsp/memmap.h>
#include <intelblocks/smm.h>
#include <soc/systemagent.h>
#include <soc/pci_devs.h>

void *cbmem_top(void)
{
	const struct device *dev;
	const config_t *config;
	void *tolum = (void *)sa_get_tseg_base();

	if (!IS_ENABLED(CONFIG_SOC_INTEL_GLK))
		return tolum;

	dev = dev_find_slot(0, PCH_DEVFN_LPC);
	assert(dev != NULL);
	config = dev->chip_info;

	if (!config)
		die("Failed to get chip_info\n");

	/* FSP allocates 2x PRMRR Size Memory for alignment */
	if (config->sgx_enable)
		tolum -= config->PrmrrSize * 2;

	return tolum;
}

int smm_subregion(int sub, void **start, size_t *size)
{
	uintptr_t sub_base;
	size_t sub_size;
	void *smm_base;
	const size_t cache_size = CONFIG_SMM_RESERVED_SIZE;

	smm_region_info(&smm_base, &sub_size);
	sub_base = (uintptr_t)smm_base;

	assert(sub_size > CONFIG_SMM_RESERVED_SIZE);

	switch (sub) {
	case SMM_SUBREGION_HANDLER:
		/* Handler starts at the base of TSEG. */
		sub_size -= cache_size;
		break;
	case SMM_SUBREGION_CACHE:
		/* External cache is in the middle of TSEG. */
		sub_base += sub_size - cache_size;
		sub_size = cache_size;
		break;
	default:
		return -1;
	}

	*start = (void *)sub_base;
	*size = sub_size;

	return 0;
}
