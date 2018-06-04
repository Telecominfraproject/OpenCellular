/*
 * This file is part of the coreboot project.
 *
 * Copyright (c) 2011 Sven Schnelle <svens@stackframe.org>
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


Scope (\_SI)
{
	Method(_SST, 1, NotSerialized)
	{
		If (LEqual (Arg0, 0)) {
			/* Indicator off */

			/* power TLED off */
			\_SB.PCI0.LPCB.EC.TLED(0x00)
			/* suspend TLED off */
			\_SB.PCI0.LPCB.EC.TLED(0x07)
		}

		If (LEqual (Arg0, 1)) {
			/* working state */

			/* power TLED on */
			\_SB.PCI0.LPCB.EC.TLED(0x80)
			/* suspend TLED off */
			\_SB.PCI0.LPCB.EC.TLED(0x07)
		}

		If (LEqual (Arg0, 2)) {
			/* waking state */

			/* power LED on */
			\_SB.PCI0.LPCB.EC.TLED(0x80)
			/* suspend LED blinking */
			\_SB.PCI0.LPCB.EC.TLED(0xc7)
		}

		If (LEqual (Arg0, 3)) {
			/* sleep state */

			/* power TLED pulsing */
			\_SB.PCI0.LPCB.EC.TLED(0xa0)
			/* suspend TLED on */
			\_SB.PCI0.LPCB.EC.TLED(0x87)
		}
	}
}
