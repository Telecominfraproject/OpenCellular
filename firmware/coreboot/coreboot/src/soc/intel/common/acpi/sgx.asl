/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Intel Corp.
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

External(\_SB.EPCS, IntObj) // Enclave Page Cache (EPC) Status
External(\_SB.EMNA, IntObj) // EPC base address
External(\_SB.ELNG, IntObj) // EPC length

Scope(\_SB)
{
	// Secure Enclave memory
	Device (EPC)
	{
		Name (_HID, EISAID ("INT0E0C"))
		Name (_STR, Unicode ("Enclave Page Cache 1.0"))
		Name (_MLS, Package () {
			Package (2) { "en", Unicode ("Enclave Page Cache 1.0") }
		})

		Name (RBUF, ResourceTemplate ()
		{
			// _MIN, _MAX and  _LEN get patched runtime
			QWordMemory (
			ResourceConsumer, // ResourceUsage
			PosDecode,	  // Decode		_DEC
			MinNotFixed,	  // IsMinFixed		_MIF
			MaxNotFixed,	  // IsMaxFixed		_MAF
			NonCacheable,	  // Cacheable		_MEM
			ReadWrite,	  // ReadAndWrite	_RW
			0,		  // AddressGranularity	_GRA
			0,		  // AddressMinimum	_MIN
			0,		  // AddressMaximum	_MAX
			0,		  // AddressTranslation	_TRA
			1,		  // RangeLength	_LEN
			,		  // ResourceSourceIndex
			,		  // ResourceSource
			BAR0		  // DescriptorName
			)
		})

		Method (_CRS, 0x0, NotSerialized)
		{
			CreateQwordField (RBUF, ^BAR0._MIN, EMIN)
			CreateQwordField (RBUF, ^BAR0._MAX, EMAX)
			CreateQwordField (RBUF, ^BAR0._LEN, ELEN)
			Store (\_SB.EMNA, EMIN)
			Store (\_SB.ELNG, ELEN)
			Subtract (Add (\_SB.EMNA, \_SB.ELNG), 1, EMAX)
			Return (RBUF)
		}

		Method (_STA, 0x0, NotSerialized)
		{
			If (LNotEqual (\_SB.EPCS, 0))
			{
				Return (0xF)
			}
			Return (0x0)
		}

	} // end EPC Device
} // End of Scope(\_SB)
