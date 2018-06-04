/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013-2014 Sage Electronic Engineering, LLC.
 * Copyright (C) 2015 Intel Corp.
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

#ifndef FSP1_1_UTIL_H
#define FSP1_1_UTIL_H

#include <rules.h>
#include <fsp/api.h>
/* Current users expect to get the SoC's FSP definitions by including util.h. */
#include <fsp/soc_binding.h>
#include <program_loading.h>
#include <commonlib/region.h>

/* find_fsp() should only be called from assembly code. */
FSP_INFO_HEADER *find_fsp(uintptr_t fsp_base_address);
/* Set FSP's runtime information. */
void fsp_set_runtime(FSP_INFO_HEADER *fih, void *hob_list);
/* Use a new FSP_INFO_HEADER at runtime. */
void fsp_update_fih(FSP_INFO_HEADER *fih);
/* fsp_get_fih() is only valid after calling fsp_set_runtime(). */
FSP_INFO_HEADER *fsp_get_fih(void);
/* fsp_get_hob_list() is only valid after calling fsp_set_runtime(). */
void *fsp_get_hob_list(void);
void fsp_early_init(FSP_INFO_HEADER *fsp_info);
void fsp_notify(u32 phase);
void print_hob_type_structure(u16 hob_type, void *hob_list_ptr);
void print_fsp_info(FSP_INFO_HEADER *fsp_header);
void *get_next_type_guid_hob(UINT16 type, const EFI_GUID *guid,
	const void *hob_start);
void *get_next_resource_hob(const EFI_GUID *guid, const void *hob_start);
void *get_first_resource_hob(const EFI_GUID *guid);
void fsp_display_upd_value(const char *name, uint32_t size, uint64_t old,
	uint64_t new);

/* Return version of FSP associated with fih. */
static inline uint32_t fsp_version(FSP_INFO_HEADER *fih)
{
	return fih->ImageRevision;
}

/*
 * Relocate FSP entire binary into ram. Returns < 0 on error, 0 on success.
 * The FSP source is pointed to by region_device and the relocation information
 * is encoded in a struct prog with its entry point set to the FSP info header.
 */
int fsp_relocate(struct prog *fsp_relocd, const struct region_device *fsp_src);

/* Additional HOB types not included in the FSP:
 * #define EFI_HOB_TYPE_HANDOFF 0x0001
 * #define EFI_HOB_TYPE_MEMORY_ALLOCATION 0x0002
 * #define EFI_HOB_TYPE_RESOURCE_DESCRIPTOR 0x0003
 * #define EFI_HOB_TYPE_GUID_EXTENSION 0x0004
 * #define EFI_HOB_TYPE_FV 0x0005
 * #define EFI_HOB_TYPE_CPU 0x0006
 * #define EFI_HOB_TYPE_MEMORY_POOL 0x0007
 * #define EFI_HOB_TYPE_CV 0x0008
 * #define EFI_HOB_TYPE_UNUSED 0xFFFE
 * #define EFI_HOB_TYPE_END_OF_HOB_LIST 0xffff
 */
#define EFI_HOB_TYPE_HANDOFF		0x0001
#define EFI_HOB_TYPE_MEMORY_POOL	0x0007

/* The offset in bytes from the start of the info structure */
#define FSP_IMAGE_SIG_LOC			0
#define FSP_IMAGE_ID_LOC			16
#define FSP_IMAGE_BASE_LOC			28
#define FSP_IMAGE_ATTRIBUTE_LOC			32
#define  GRAPHICS_SUPPORT_BIT			(1 << 0)

#define ERROR_NO_FV_SIG				1
#define ERROR_NO_FFS_GUID			2
#define ERROR_NO_INFO_HEADER			3
#define ERROR_IMAGEBASE_MISMATCH		4
#define ERROR_INFO_HEAD_SIG_MISMATCH		5
#define ERROR_FSP_SIG_MISMATCH			6
#define ERROR_FSP_REV_MISMATCH			7

#if ENV_RAMSTAGE
extern void *FspHobListPtr;
#endif

void *get_hob_list(void);
void *get_next_hob(uint16_t type, const void *hob_start);
void *get_first_hob(uint16_t type);
void *get_next_guid_hob(const EFI_GUID *guid, const void *hob_start);
void *get_first_guid_hob(const EFI_GUID *guid);

/*
 * Writes number_of_bytes data bytes from buffer to the console.
 * The number of bytes actually written to the console is returned.
 *
 * If number_of_bytes is zero, don't output any data but instead wait until
 * the console has output all data, then return 0.
 */
__attribute__((cdecl)) size_t fsp_write_line(uint8_t *buffer,
	size_t number_of_bytes);

#endif	/* FSP1_1_UTIL_H */
