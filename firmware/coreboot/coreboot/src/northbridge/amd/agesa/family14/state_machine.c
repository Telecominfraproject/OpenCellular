/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Kyösti Mälkki
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

#include "Porting.h"
#include "AGESA.h"

#include <arch/io.h>
#include <cbmem.h>
#include <device/device.h>
#include <device/pci_def.h>
#include <device/pci_ops.h>
#include <halt.h>
#include <reset.h>
#include <smp/node.h>
#include <northbridge/amd/agesa/state_machine.h>
#include <northbridge/amd/agesa/agesa_helper.h>

#include <sb_cimx.h>

void platform_BeforeInitReset(struct sysinfo *cb, AMD_RESET_PARAMS *Reset)
{
	/* Reboots with outb(3,0x92), outb(4,0xcf9) or triple-fault all
	 * would fail later in AmdInitPost(), when DRAM is already configured
	 * and C6DramLock bit has been set.
	 *
	 * As a workaround, do a hard reset to clear C6DramLock bit.
	 */
#ifdef __SIMPLE_DEVICE__
	pci_devfn_t dev = PCI_DEV(0, 0x18, 2);
#else
	struct device *dev = dev_find_slot(0, PCI_DEVFN(0x18, 2));
#endif
	if (boot_cpu()) {
		u32 mct_cfg_lo = pci_read_config32(dev, 0x118);
		if (mct_cfg_lo & (1<<19)) {
			printk(BIOS_CRIT, "C6DramLock is set, resetting\n");
			hard_reset();
		}
	}
}

void platform_BeforeInitEarly(struct sysinfo *cb, AMD_EARLY_PARAMS *Early)
{
}

void platform_BeforeInitPost(struct sysinfo *cb, AMD_POST_PARAMS *Post)
{
}

void platform_AfterInitPost(struct sysinfo *cb, AMD_POST_PARAMS *Post)
{
	backup_top_of_low_cacheable(Post->MemConfig.Sub4GCacheTop);
}

void platform_BeforeInitResume(struct sysinfo *cb, AMD_RESUME_PARAMS *Resume)
{
	OemInitResume(&Resume->S3DataBlock);
}

void platform_AfterInitResume(struct sysinfo *cb, AMD_RESUME_PARAMS *Resume)
{
}

void platform_BeforeInitEnv(struct sysinfo *cb, AMD_ENV_PARAMS *Env)
{
	EmptyHeap();
}

void platform_AfterInitEnv(struct sysinfo *cb, AMD_ENV_PARAMS *Env)
{
	amd_initenv();
}

void platform_BeforeS3LateRestore(struct sysinfo *cb, AMD_S3LATE_PARAMS *S3Late)
{
	OemS3LateRestore(&S3Late->S3DataBlock);
}

void platform_AfterS3LateRestore(struct sysinfo *cb, AMD_S3LATE_PARAMS *S3Late)
{
}

void platform_BeforeInitMid(struct sysinfo *cb, AMD_MID_PARAMS *Mid)
{
	sb_After_Pci_Init();
	sb_Mid_Post_Init();

	amd_initcpuio();
}

void platform_AfterInitLate(struct sysinfo *cb, AMD_LATE_PARAMS *Late)
{
	sb_Late_Post();
}

void platform_AfterS3Save(struct sysinfo *cb, AMD_S3SAVE_PARAMS *S3Save)
{
	OemS3Save(&S3Save->S3DataBlock);
}
