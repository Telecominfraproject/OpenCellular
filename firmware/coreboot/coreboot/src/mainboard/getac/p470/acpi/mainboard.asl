/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
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

Device (LID0)
{
	Name(_HID, EisaId("PNP0C0D"))
	Method(_LID, 0)
	{
		If(\_SB.PCI0.LPCB.EC0.LIDS) {
			Return (0)
		} Else {
			Return (1)
		}
	}
}

Device (SLPB)
{
	Name(_HID, EisaId("PNP0C0E"))
}

Device (PWRB)
{
	Name(_HID, EisaId("PNP0C0C"))

	// Wake
	Name(_PRW, Package(){0x1d, 0x04})
}

#include "acpi/battery.asl"
