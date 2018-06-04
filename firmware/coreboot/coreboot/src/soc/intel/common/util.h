/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Intel Corporation.
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

#ifndef _INTEL_COMMON_UTIL_H_
#define _INTEL_COMMON_UTIL_H_

#include <arch/cpu.h>
#include <cpu/x86/msr.h>
#include <stdint.h>

asmlinkage void soc_display_mtrrs(void);
uint32_t soc_get_variable_mtrr_count(uint64_t *msr);

#endif /* _INTEL_COMMON_UTIL_H_ */
