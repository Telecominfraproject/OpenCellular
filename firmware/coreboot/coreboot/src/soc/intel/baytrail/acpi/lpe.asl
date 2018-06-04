/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

Device (LPEA)
{
	Name (_HID, "80860F28")
	Name (_CID, "80860F28")
	Name (_UID, 1)
	Name (_DDN, "Low Power Audio Controller")
	Name (_PR0, Package () { PLPE })

	Name (RBUF, ResourceTemplate()
	{
		Memory32Fixed (ReadWrite, 0, 0x00200000, BAR0)
		Memory32Fixed (ReadWrite, 0, 0x00001000, BAR1)
		Memory32Fixed (ReadWrite, 0, 0x00100000, BAR2)
		Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive,,,)
		{
			LPE_DMA0_IRQ
		}
		Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive,,,)
		{
			LPE_DMA1_IRQ
		}
		Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive,,,)
		{
			LPE_SSP0_IRQ
		}
		Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive,,,)
		{
			LPE_SSP1_IRQ
		}
		Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive,,,)
		{
			LPE_SSP2_IRQ
		}
		Interrupt (ResourceConsumer, Level, ActiveLow, Exclusive,,,)
		{
			LPE_IPC2HOST_IRQ
		}
	})

	Method (_CRS)
	{
		/* Update BAR0 from NVS */
		CreateDwordField (^RBUF, ^BAR0._BAS, BAS0)
		Store (\LPB0, BAS0)

		/* Update BAR1 from NVS */
		CreateDwordField (^RBUF, ^BAR1._BAS, BAS1)
		Store (\LPB1, BAS1)

		/* Update LPE FW from NVS */
		CreateDwordField (^RBUF, ^BAR2._BAS, BAS2)
		Store (\LPFW, BAS2)

		/* Append any Mainboard defined GPIOs */
		If (CondRefOf (^GBUF)) {
			ConcatenateResTemplate (^RBUF, ^GBUF, Local1)
			Return (Local1)
		}

		Return (^RBUF)
	}

	Method (_STA)
	{
		If (LEqual (\LPEN, 1)) {
			Return (0xF)
		} Else {
			Return (0x0)
		}
	}

	OperationRegion (KEYS, SystemMemory, LPB1, 0x100)
	Field (KEYS, DWordAcc, NoLock, WriteAsZeros)
	{
		Offset (0x84),
		PSAT, 32,
	}

	PowerResource (PLPE, 0, 0)
	{
		Method (_STA)
		{
			Return (1)
		}

		Method (_OFF)
		{
			Or (PSAT, 0x00000003, PSAT)
			Or (PSAT, 0x00000000, PSAT)
		}

		Method (_ON)
		{
			And (PSAT, 0xfffffffc, PSAT)
			Or (PSAT, 0x00000000, PSAT)
		}
	}
}
