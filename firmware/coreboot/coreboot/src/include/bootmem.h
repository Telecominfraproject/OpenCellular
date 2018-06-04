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

#ifndef BOOTMEM_H
#define BOOTMEM_H

#include <memrange.h>
#include <stdint.h>
#include <boot/coreboot_tables.h>

/**
 * Bootmem types match to LB_MEM tags, except for the following:
 * BM_MEM_RAMSTAGE : Memory where any kind of boot firmware resides and that
 *                   should not be touched by bootmem (by example: stack,
 *                   TTB, program, ...).
 * BM_MEM_PAYLOAD  : Memory where any kind of payload resides and that should
 *                   not be touched by bootmem.
 * Start at 0x10000 to make sure that the caller doesn't provide LB_MEM tags.
 */
enum bootmem_type {
	BM_MEM_FIRST = 0x10000,	/* First entry in this list */
	BM_MEM_RAM,		/* Memory anyone can use */
	BM_MEM_RESERVED,	/* Don't use this memory region */
	BM_MEM_ACPI,		/* ACPI Tables */
	BM_MEM_NVS,		/* ACPI NVS Memory */
	BM_MEM_UNUSABLE,	/* Unusable address space */
	BM_MEM_VENDOR_RSVD,	/* Vendor Reserved */
	BM_MEM_TABLE,		/* Ram configuration tables are kept in */
	/* Tags below this point are ignored for the OS table. */
	BM_MEM_OS_CUTOFF = BM_MEM_TABLE,
	BM_MEM_RAMSTAGE,
	BM_MEM_PAYLOAD,
	BM_MEM_LAST,		/* Last entry in this list */
};

/**
 * Write memory coreboot table. Current resource map is serialized into
 * memtable (LB_MEM_* types). bootmem library is unusable until this function
 * is called first in the write tables path before payload is loaded.
 *
 * Bootmem types match to LB_MEM tags, except for the following:
 * BM_MEM_RAMSTAGE : Translates to LB_MEM_RAM.
 * BM_MEM_PAYLOAD  : Translates to LB_MEM_RAM.
 */
void bootmem_write_memory_table(struct lb_memory *mem);

/* Architecture hook to add bootmem areas the architecture controls when
 * bootmem_write_memory_table() is called. */
void bootmem_arch_add_ranges(void);

/* Platform hook to add bootmem areas the platform / board controls. */
void bootmem_platform_add_ranges(void);

/* Add a range of a given type to the bootmem address space. */
void bootmem_add_range(uint64_t start, uint64_t size,
		       const enum bootmem_type tag);

/* Print current range map of boot memory. */
void bootmem_dump_ranges(void);

typedef bool (*range_action_t)(const struct range_entry *r, void *arg);

/**
 * Walk memory tables from OS point of view and call the provided function,
 * for every region. The caller has to return false to break out of the loop any
 * time, or return true to continue.
 *
 * @param action The function to call for each memory range.
 * @param arg Pointer passed to function @action. Set to NULL if unused.
 * @return true if the function 'action' returned false.
 */
bool bootmem_walk_os_mem(range_action_t action, void *arg);

/**
 * Walk memory tables and call the provided function, for every region.
 * The caller has to return false to break out of the loop any time, or
 * return true to continue.
 *
 * @param action The function to call for each memory range.
 * @param arg Pointer passed to function @action. Set to NULL if unused.
 * @return true if the function 'action' returned false.
 */
bool bootmem_walk(range_action_t action, void *arg);

/* Return 1 if region targets usable RAM, 0 otherwise. */
int bootmem_region_targets_usable_ram(uint64_t start, uint64_t size);

/* Allocate a temporary buffer from the unused RAM areas. */
void *bootmem_allocate_buffer(size_t size);

#endif /* BOOTMEM_H */
