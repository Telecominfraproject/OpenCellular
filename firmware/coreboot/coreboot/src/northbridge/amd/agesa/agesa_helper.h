/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Advanced Micro Devices, Inc.
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

#ifndef _AGESA_HELPER_H_
#define _AGESA_HELPER_H_

#include <stddef.h>
#include <arch/cpu.h>

enum {
	PICK_DMI,       /* DMI Interface */
	PICK_PSTATE,    /* Acpi Pstate SSDT Table */
	PICK_SRAT,      /* SRAT Table */
	PICK_SLIT,      /* SLIT Table */
	PICK_WHEA_MCE,  /* WHEA MCE table */
	PICK_WHEA_CMC,  /* WHEA CMV table */
	PICK_ALIB,      /* SACPI SSDT table with ALIB implementation */
	PICK_IVRS,      /* IOMMU ACPI IVRS(I/O Virtualization Reporting Structure) table */
	PICK_CRAT,      /* Component Resource Affinity Table table */
	PICK_CDIT,      /* Component Locality Distance Information table */
};

void agesawrapper_setlateinitptr (void *Late);
void *agesawrapper_getlateinitptr (int pick);

void amd_initcpuio(void);
void amd_initmmio(void);
void amd_initenv(void);

void *GetHeapBase(void);
void EmptyHeap(void);

#define BSP_STACK_BASE_ADDR		0x30000

#if 1
/* This covers node 0 only. */
#define HIGH_ROMSTAGE_STACK_SIZE	(0x48000 - BSP_STACK_BASE_ADDR)
#else
/* This covers total of 8 nodes. */
#define HIGH_ROMSTAGE_STACK_SIZE	(0xA0000 - BSP_STACK_BASE_ADDR)
#endif

#define HIGH_MEMORY_SCRATCH		0x30000

void fixup_cbmem_to_UC(int s3resume);
void recover_postcar_frame(struct postcar_frame *pcf, int s3resume);

void restore_mtrr(void);
void backup_mtrr(void *mtrr_store, u32 *mtrr_store_size);
const void *OemS3Saved_MTRR_Storage(void);

#endif /* _AGESA_HELPER_H_ */
