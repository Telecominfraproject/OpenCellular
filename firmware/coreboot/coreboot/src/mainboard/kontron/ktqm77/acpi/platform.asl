/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
 * Copyright (C) 2011-2012 The Chromium OS Authors. All rights reserved.
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

/* The _PTS method (Prepare To Sleep) is called before the OS is
 * entering a sleep state. The sleep state number is passed in Arg0
 */

Method(_PTS,1)
{
	/* Let suspend LED flash slowly in S3 and S4 */
	If (LOr (LEqual (Arg0, 3), LEqual (Arg0, 4)))
	{
		\_SB.PCI0.LPCB.SIO0.SUSL (0x06)
	}
	Else
	{
		\_SB.PCI0.LPCB.SIO0.SUSL (0x02)
	}
}

/* The _WAK method is called on system wakeup */

Method(_WAK,1)
{
	/* Disable suspend LED during normal operation */
	\_SB.PCI0.LPCB.SIO0.SUSL (0x02)
	Return(Package(){0,0})
}
