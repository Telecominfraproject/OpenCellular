/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007 - 2009 coresystems GmbH
 * Copyright (C) 2014 - 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "../include/soc/iomap.h"

Scope(\)
{
	// IO-Trap at 0x800. This is the ACPI->SMI communication interface.

	OperationRegion(IO_T, SystemIO, 0x800, 0x10)
	Field(IO_T, ByteAcc, NoLock, Preserve)
	{
		Offset(0x8),
		TRP0, 8		// IO-Trap at 0x808
	}

	// Private Chipset Register(PCR). Memory Mapped through ILB
	OperationRegion(PCRR, SystemMemory, DEFAULT_PCR_BASE, 0x01000000)
	Field(PCRR, DWordAcc, Lock, Preserve)
	{
		Offset (0xD03100),	// Interrupt Routing Registers
		PRTA,	8,
		PRTB,	8,
		PRTC,	8,
		PRTD,	8,
		PRTE,	8,
		PRTF,	8,
		PRTG,	8,
		PRTH,	8,
	}
}

// PCI Express Ports 0:[9-11].0
#include "pcie.asl"

// SMBus 0:12.0
#include "smbus2.asl"

// SATA 0:13.0
#include "sata.asl"

// SATA 0:14.0
#include "sata2.asl"

// xHCI 0:15.0
#include "xhci.asl"

// Virtual root port 0
Device (VRP0) {
	Name   (_ADR, 0x00160000)
}

// Virtual root port 1
Device (VRP1) {
	Name   (_ADR, 0x00170000)
}

// ME HECI
Device (HECI) {
	Name   (_ADR, 0x00180000)
}

// ME HECI2
Device (HEC2) {
	Name   (_ADR, 0x00180001)
}

// MEKT on PCH
Device (MEKT) {
	Name   (_ADR, 0x00180003)
}

// ME HECI3
Device (HEC3) {
	Name   (_ADR, 0x00180004)
}

// UART 0
Device (UAR0) {
	Name   (_ADR, 0x001A0000)
}

// UART 1
Device (UAR1) {
	Name   (_ADR, 0x001A0001)
}

// UART 2
Device (UAR2) {
	Name   (_ADR, 0x001A0002)
}

// eMMC
Device (EMMC) {
	Name   (_ADR, 0x001C0000)
}

// LPC Bridge 0:1f.0
#include "lpc.asl"

// P2SB 0:1f.1
Device (P2SB)
{
	Name (_ADR, 0x001F0001)
}

// PMC 0:1f.2
#include "pmc.asl"

// SMBus 0:1f.4
#include "smbus.asl"

// Northpeak 0:1f.7
#include "npk.asl"

/* IRQ assignment is mainboard specific. Get it from mainboard ACPI code */
#include "acpi/mainboard_pci_irqs.asl"

Method (_OSC, 4)
{
	/* Check for proper GUID */
	If (LEqual (Arg0, ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")))
	{
		/* Let OS control everything */
		Return (Arg3)
	}
	Else
	{
		/* Unrecognized UUID */
		CreateDWordField (Arg3, 0, CDW1)
		Or (CDW1, 4, CDW1)
		Return (Arg3)
	}
}
