/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 Advanced Micro Devices, Inc.
 * Copyright (C) 2013 Sage Electronic Engineering, LLC
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

/* Data to be patched by the BIOS during POST */
/* FIXME the patching is not done yet! */
/* Memory related values */
Name(LOMH, 0x0)	/* Start of unused memory in C0000-E0000 range */
Name(PBAD, 0x0)	/* Address of BIOS area (If TOM2 != 0, Addr >> 16) */
Name(PBLN, 0x0)	/* Length of BIOS area */

Name(PCBA, CONFIG_MMCONF_BASE_ADDRESS)	/* Base address of PCIe config space */
Name(HPBA, 0xFED00000)	/* Base address of HPET table */

/* Some global data */
Name(OSVR, 3)	/* Assume nothing. WinXp = 1, Vista = 2, Linux = 3, WinCE = 4 */
Name(OSV, Ones)	/* Assume nothing */
Name(PMOD, One)	/* Assume APIC */

Scope(\_SB) {
	Method(OSFL, 0){

		if(LNotEqual(OSVR, Ones)) {Return(OSVR)}	/* OS version was already detected */

		if(CondRefOf(\_OSI))
		{
			Store(1, OSVR)					/* Assume some form of XP */
			if (\_OSI("Windows 2006"))		/* Vista */
			{
				Store(2, OSVR)
			}
		} else {
			If(WCMP(\_OS,"Linux")) {
				Store(3, OSVR)				/* Linux */
			} Else {
				Store(4, OSVR)				/* Gotta be WinCE */
			}
		}
		Return(OSVR)
	}
}

Scope(\_SI) {
	Method(_SST, 1) {
		/* DBGO("\\_SI\\_SST\n") */
		/* DBGO("   New Indicator state: ") */
		/* DBGO(Arg0) */
		/* DBGO("\n") */
	}
} /* End Scope SI */
