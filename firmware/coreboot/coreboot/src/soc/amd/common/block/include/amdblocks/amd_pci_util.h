/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Sage Electronic Engineering, LLC.
 * Copyright (C) 2017 Advanced Micro Devices, Inc.
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

#ifndef __AMD_PCI_UTIL_H__
#define __AMD_PCI_UTIL_H__

#include <stdint.h>
#include <soc/amd_pci_int_defs.h>

/* FCH index/data registers */
#define PCI_INTR_INDEX		0xc00
#define PCI_INTR_DATA		0xc01

struct pirq_struct {
	u8 devfn;
	u8 PIN[4];	/* PINA/B/C/D are index 0/1/2/3 */
};

struct irq_idx_name {
	uint8_t index;
	const char * const name;
};

extern const struct pirq_struct *pirq_data_ptr;
extern u32 pirq_data_size;
extern const u8 *intr_data_ptr;
extern const u8 *picr_data_ptr;

u8 read_pci_int_idx(u8 index, int mode);
void write_pci_int_idx(u8 index, int mode, u8 data);
void write_pci_cfg_irqs(void);
void write_pci_int_table(void);
const struct irq_idx_name *sb_get_apic_reg_association(size_t *size);

#endif /* __AMD_PCI_UTIL_H__ */
