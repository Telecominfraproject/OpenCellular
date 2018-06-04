/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Keith Hui <buurin@gmail.com>
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

#define __SIMPLE_DEVICE__

#include <arch/io.h>
#include <cbmem.h>
#include <commonlib/helpers.h>
#include "i440bx.h"

void *cbmem_top(void)
{
	/* Base of TSEG is top of usable DRAM */
	/*
	 * SMRAM - System Management RAM Control Register
	 * 0x72
	 * [7:4] Not relevant to this function.
	 * [3:3] Global SMRAM Enable (G_SMRAME)
	 * [2:0] Hardwired to 010.
	 *
	 * ESMRAMC - Extended System Management RAM Control
	 * 0x73
	 * [7:7] H_SMRAM_EN
	 *       1 = When G_SMRAME=1, High SMRAM space is enabled at
	 *           0x100A0000-0x100FFFFF and forwarded to DRAM address
	 *           0x000A0000-0x000FFFFF.
	 *       0 = When G_SMRAME=1, Compatible SMRAM space is enabled at
	 *           0x000A0000-0x000BFFFF.
	 * [6:3] Not relevant to this function.
	 * [2:1] TSEG Size (T_SZ)
	 *       Selects the size of the TSEG memory block, if enabled.
	 *       00 = 128KiB
	 *       01 = 256KiB
	 *       10 = 512KiB
	 *       11 = 1MiB
	 * [0:0] TSEG_EN
	 *       When SMRAM[G_SMRAME] and this bit are 1, TSEG is enabled to
	 *       appear between DRAM address (TOM-<TSEG Size>) to TOM.
	 *
	 * Source: 440BX datasheet, pages 3-28 thru 3-29.
	 */
	unsigned long tom = pci_read_config8(NB, DRB7) * 8 * MiB;

	int gsmrame = pci_read_config8(NB, SMRAM) & 0x8;
	/* T_SZ and TSEG_EN */
	int tseg = pci_read_config8(NB, ESMRAMC) & 0x7;
	if ((tseg & 0x1) && gsmrame) {
		int tseg_size = 128 * KiB * (1 << (tseg >> 1));
		tom -= tseg_size;
	}
	return (void *)tom;
}
