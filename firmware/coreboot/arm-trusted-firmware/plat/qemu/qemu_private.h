/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __QEMU_PRIVATE_H
#define __QEMU_PRIVATE_H

#include <sys/types.h>

#include <xlat_tables_defs.h>
#include "../../bl1/bl1_private.h"

void qemu_configure_mmu_secure(unsigned long total_base,
			unsigned long total_size,
			unsigned long code_start, unsigned long code_limit,
			unsigned long ro_start, unsigned long ro_limit,
			unsigned long coh_start, unsigned long coh_limit);

void qemu_configure_mmu_el1(unsigned long total_base, unsigned long total_size,
			unsigned long code_start, unsigned long code_limit,
			unsigned long ro_start, unsigned long ro_limit,
			unsigned long coh_start, unsigned long coh_limit);

void qemu_configure_mmu_el3(unsigned long total_base, unsigned long total_size,
			unsigned long code_start, unsigned long code_limit,
			unsigned long ro_start, unsigned long ro_limit,
			unsigned long coh_start, unsigned long coh_limit);

void plat_qemu_io_setup(void);
unsigned int plat_qemu_calc_core_pos(u_register_t mpidr);

int dt_add_psci_node(void *fdt);
int dt_add_psci_cpu_enable_methods(void *fdt);

void qemu_console_init(void);

#endif /*__QEMU_PRIVATE_H*/
