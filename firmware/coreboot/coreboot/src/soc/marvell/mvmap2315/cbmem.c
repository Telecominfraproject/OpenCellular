/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Marvell, Inc.
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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cbmem.h>
#include <soc/addressmap.h>
#include <soc/sdram.h>
#include <symbols.h>

void *cbmem_top(void)
{
	return (void *)min((uintptr_t)_dram + sdram_size_mb() * MiB,
			   MAX_DRAM_ADDRESS);
}
