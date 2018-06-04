/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
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

/* Global Variables */

Name (\PICM, 0)		// IOAPIC/8259

/*
 * Global ACPI memory region. This region is used for passing information
 * between coreboot (aka "the system bios"), ACPI, and the SMI handler.
 * Since we don't know where this will end up in memory at ACPI compile time,
 * we have to fix it up in coreboot's ACPI creation phase.
 */

External(NVSA)
OperationRegion (GNVS, SystemMemory, NVSA, 0x2000)
Field (GNVS, ByteAcc, NoLock, Preserve)
{
	/* Miscellaneous */
	Offset (0x00),
	OSYS,	16,	// 0x00 - Operating System
	SMIF,	8,	// 0x02 - SMI function
	PRM0,	8,	// 0x03 - SMI function parameter
	PRM1,	8,	// 0x04 - SMI function parameter
	SCIF,	8,	// 0x05 - SCI function
	PRM2,	8,	// 0x06 - SCI function parameter
	PRM3,	8,	// 0x07 - SCI function parameter
	LCKF,	8,	// 0x08 - Global Lock function for EC
	PRM4,	8,	// 0x09 - Lock function parameter
	PRM5,	8,	// 0x0a - Lock function parameter
	PCNT,	8,	// 0x0b - Processor Count
	PPCM,	8,	// 0x0c - Max PPC State
	TMPS,	8,	// 0x0d - Temperature Sensor ID
	TLVL,	8,	// 0x0e - Throttle Level Limit
	FLVL,	8,	// 0x0f - Current FAN Level
	TCRT,	8,	// 0x10 - Critical Threshold
	TPSV,	8,	// 0x11 - Passive Threshold
	TMAX,	8,	// 0x12 - CPU Tj_max
	S5U0,	8,	// 0x13 - Enable USB in S5
	S3U0,	8,	// 0x14 - Enable USB in S3
	S33G,	8,	// 0x15 - Enable 3G in S3
	LIDS,	8,	// 0x16 - LID State
	PWRS,	8,	// 0x17 - AC Power State
	CMEM,	32,	// 0x18 - 0x1b - CBMEM TOC
	CBMC,	32,	// 0x1c - 0x1f - coreboot Memory Console
	PM1I,	64,	// 0x20 - 0x27 - PM1 wake status bit
	GPEI,	64,	// 0x28 - 0x2f - GPE wake status bit

	/* IGD OpRegion */
	Offset (0xb4),
	ASLB,	32,	// 0xb4 - IGD OpRegion Base Address
	IBTT,	 8,	// 0xb8 - IGD boot panel device
	IPAT,	 8,	// 0xb9 - IGD panel type cmos option
	ITVF,	 8,	// 0xba - IGD TV format cmos option
	ITVM,	 8,	// 0xbb - IGD TV minor format option
	IPSC,	 8,	// 0xbc - IGD panel scaling
	IBLC,	 8,	// 0xbd - IGD BLC config
	IBIA,	 8,	// 0xbe - IGD BIA config
	ISSC,	 8,	// 0xbf - IGD SSC config
	I409,	 8,	// 0xc0 - IGD 0409 modified settings
	I509,	 8,	// 0xc1 - IGD 0509 modified settings
	I609,	 8,	// 0xc2 - IGD 0609 modified settings
	I709,	 8,	// 0xc3 - IGD 0709 modified settings
	IDMM,	 8,	// 0xc4 - IGD Power conservation feature
	IDMS,	 8,	// 0xc5 - IGD DVMT memory size
	IF1E,	 8,	// 0xc6 - IGD function 1 enable
	HVCO,	 8,	// 0xc7 - IGD HPLL VCO
	NXD1,	32,	// 0xc8 - IGD _DGS next DID1
	NXD2,	32,	// 0xcc - IGD _DGS next DID2
	NXD3,	32,	// 0xd0 - IGD _DGS next DID3
	NXD4,	32,	// 0xd4 - IGD _DGS next DID4
	NXD5,	32,	// 0xd8 - IGD _DGS next DID5
	NXD6,	32,	// 0xdc - IGD _DGS next DID6
	NXD7,	32,	// 0xe0 - IGD _DGS next DID7
	NXD8,	32,	// 0xe4 - IGD _DGS next DID8

	ISCI,	 8,	// 0xe8 - IGD SMI/SCI mode (0: SCI)
	PAVP,	 8,	// 0xe9 - IGD PAVP data
	Offset (0xeb),
	OSCC,	 8,	// 0xeb - PCIe OSC control
	NPCE,	 8,	// 0xec - native pcie support
	PLFL,	 8,	// 0xed - platform flavor
	BREV,	 8,	// 0xee - board revision
	DPBM,	 8,	// 0xef - digital port b mode
	DPCM,	 8,	// 0xf0 - digital port c mode
	DPDM,	 8,	// 0xf1 - digital port d mode
	ALFP,	 8,	// 0xf2 - active lfp
	IMON,	 8,	// 0xf3 - current graphics turbo imon value
	MMIO,	 8,	// 0xf4 - 64bit mmio support

	/* ChromeOS specific */
	Offset (0x100),
	#include <vendorcode/google/chromeos/acpi/gnvs.asl>

	/* Device specific */
	Offset (0x1000),
	#include "device_nvs.asl"
}

/* Set flag to enable USB charging in S3 */
Method (S3UE)
{
	Store (One, \S3U0)
}

/* Set flag to disable USB charging in S3 */
Method (S3UD)
{
	Store (Zero, \S3U0)
}

/* Set flag to enable USB charging in S5 */
Method (S5UE)
{
	Store (One, \S5U0)
}

/* Set flag to disable USB charging in S5 */
Method (S5UD)
{
	Store (Zero, \S5U0)
}

/* Set flag to enable 3G module in S3 */
Method (S3GE)
{
	Store (One, \S33G)
}

/* Set flag to disable 3G module in S3 */
Method (S3GD)
{
	Store (Zero, \S33G)
}
