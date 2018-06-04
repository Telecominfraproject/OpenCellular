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

#ifndef _SOC_APOLLOLAKE_IOMAP_H_
#define _SOC_APOLLOLAKE_IOMAP_H_

#include <commonlib/helpers.h>

#define P2SB_BAR			CONFIG_PCR_BASE_ADDRESS
#define P2SB_SIZE			(16 * MiB)
#define MCH_BASE_ADDRESS		0xfed10000
#define MCH_BASE_SIZE			(32 * KiB)

#define ACPI_BASE_ADDRESS		0x400
#define ACPI_BASE_SIZE			0x100
#define R_ACPI_PM1_TMR			0x8

/* CST Range (R/W) IO port block size */
#define PMG_IO_BASE_CST_RNG_BLK_SIZE	0x5
/* ACPI PMIO Offset to C-state register*/
#define ACPI_PMIO_CST_REG	(ACPI_BASE_ADDRESS + 0x14)

/* Accesses to these BARs are hardcoded in FSP */
#define PMC_BAR0			0xfe042000
#define PMC_BAR1			0xfe044000
#define PMC_BAR0_SIZE			(8 * KiB)

#define SRAM_BASE_0			0xfe900000
#define SRAM_SIZE_0			(8 * KiB)
#define SRAM_BASE_2			0xfe902000
#define SRAM_SIZE_2			(4 * KiB)

#define HECI1_BASE_ADDRESS		0xfed1a000

/* Temporary BAR for SPI until PCI enumeration assigns a BAR in ramstage. */
#define PRERAM_SPI_BASE_ADDRESS		0xfe010000
#define EARLY_GSPI_BASE_ADDRESS		0xfe011000

/* Temporary BAR for early I2C bus access */
#define PRERAM_I2C_BASE_ADDRESS(x)	(0xfe020000 + (0x1000 * (x)))

#endif /* _SOC_APOLLOLAKE_IOMAP_H_ */
