/*
 * This file is part of the coreboot project.
 *
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

#include <variant/thermal.h>

/* Thermal Zone */

Scope (\_TZ)
{
	ThermalZone (THRM)
	{
		/* Thermal constants for passive cooling */
		Name (_TC1, 0x02)
		Name (_TC2, 0x05)

		/* Thermal zone polling frequency: 10 seconds */
		Name (_TZP, 100)

		/* Thermal sampling period for passive cooling: 2 seconds */
		Name (_TSP, 20)

		/* Convert from Degrees C to 1/10 Kelvin for ACPI */
		Method (CTOK, 1) {
			/* 10th of Degrees C */
			Multiply (Arg0, 10, Local0)

			/* Convert to Kelvin */
			Add (Local0, 2732, Local0)

			Return (Local0)
		}

		/* Threshold for OS to shutdown */
		Method (_CRT, 0, Serialized)
		{
			Return (CTOK (\TCRT))
		}

		/* Threshold for passive cooling */
		Method (_PSV, 0, Serialized)
		{
			Return (CTOK (\TPSV))
		}

		/* Processors used for passive cooling */
		Method (_PSL, 0, Serialized)
		{
			Return (\PPKG ())
		}

		Method (_TMP, 0, Serialized)
		{
			/* Get temperature from EC in deci-kelvin */
			Store (\_SB.PCI0.LPCB.EC0.TSRD (TMPS), Local0)

			/* Critical temperature in deci-kelvin */
			Store (CTOK (\TCRT), Local1)

			If (LGreaterEqual (Local0, Local1)) {
				Store ("CRITICAL TEMPERATURE", Debug)
				Store (Local0, Debug)

				/* Wait 1 second for EC to re-poll */
				Sleep (1000)

				/* Re-read temperature from EC */
				Store (\_SB.PCI0.LPCB.EC0.TSRD (TMPS), Local0)

				Store ("RE-READ TEMPERATURE", Debug)
				Store (Local0, Debug)
			}

			Return (Local0)
		}

		/* No active fan control (_ACx) on Kahlee */
	}
}
