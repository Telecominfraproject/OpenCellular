/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Intel Corporation.
 * Copyright (C) 2015 Google Inc.
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

/* Audio Controller - Device 31, Function 3 */

Device (HDAS)
{
	Name (_ADR, 0x001F0003)
	Name (_DDN, "Audio Controller")
	Name (UUID, ToUUID ("A69F886E-6CEB-4594-A41F-7B5DCE24C553"))

	/* Device is D3 wake capable */
	Name (_S0W, 3)

	/* NHLT Table Address populated from GNVS values */
	Name (NBUF, ResourceTemplate () {
		QWordMemory (ResourceConsumer, PosDecode, MinFixed,
			     MaxFixed, NonCacheable, ReadOnly,
			     0, 0, 0, 0, 1,,, NHLT, AddressRangeACPI)
	})

	/*
	 * Device Specific Method
	 * Arg0 - UUID
	 * Arg1 - Revision
	 * Arg2 - Function Index
	 */
	Method (_DSM, 4)
	{
		If (LEqual (Arg0, ^UUID)) {
			/*
			 * Function 0: Function Support Query
			 * Returns a bitmask of functions supported.
			 */
			If (LEqual (Arg2, Zero)) {
				/*
				 * NHLT Query only supported for revision 1 and
				 * if NHLT address and length are set in NVS.
				 */
				If (LAnd (LEqual (Arg1, One),
					  LAnd (LNotEqual (NHLA, Zero),
						LNotEqual (NHLL, Zero)))) {
					Return (Buffer (One) { 0x03 })
				} Else {
					Return (Buffer (One) { 0x01 })
				}
			}

			/*
			 * Function 1: Query NHLT memory address used by
			 * Intel Offload Engine Driver to discover any non-HDA
			 * devices that are supported by the DSP.
			 *
			 * Returns a pointer to NHLT table in memory.
			 */
			If (LEqual (Arg2, One)) {
				CreateQWordField (NBUF, ^NHLT._MIN, NBAS)
				CreateQWordField (NBUF, ^NHLT._MAX, NMAS)
				CreateQWordField (NBUF, ^NHLT._LEN, NLEN)

				Store (NHLA, NBAS)
				Store (NHLA, NMAS)
				Store (NHLL, NLEN)

				Return (NBUF)
			}
		}

		Return (Buffer (One) { 0x00 })
	}
}
