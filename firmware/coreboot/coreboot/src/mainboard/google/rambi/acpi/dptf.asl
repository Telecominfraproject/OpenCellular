/*
 * This file is part of the coreboot project.
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

/* Include variant DPTF */
#include <variant/acpi/dptf.asl>

/* Mainboard specific _PDL is 1GHz */
Name (MPDL, 8)

Name (DTRT, Package () {
	/* CPU Throttle Effect on CPU */
	Package () { \_SB.DPTF.TCPU, \_SB.DPTF.TCPU, 100, 50, 0, 0, 0, 0 },

	/* CPU Effect on Temp Sensor 0 */
	Package () { \_SB.DPTF.TCPU, \_SB.DPTF.TSR0, 100, 600, 0, 0, 0, 0 },

#ifdef DPTF_ENABLE_CHARGER
	/* Charger Effect on Temp Sensor 1 */
	Package () { \_SB.DPTF.TCHG, \_SB.DPTF.TSR1, 200, 600, 0, 0, 0, 0 },
#endif

	/* CPU Effect on Temp Sensor 1 */
	Package () { \_SB.DPTF.TCPU, \_SB.DPTF.TSR1, 100, 600, 0, 0, 0, 0 },

	/* CPU Effect on Temp Sensor 2 */
	Package () { \_SB.DPTF.TCPU, \_SB.DPTF.TSR2, 100, 600, 0, 0, 0, 0 },
})

Name (MPPC, Package ()
{
	0x2,		// Revision
	Package () {	// Power Limit 1
		0,	// PowerLimitIndex, 0 for Power Limit 1
		1600,	// PowerLimitMinimum
		6200,	// PowerLimitMaximum
		1000,	// TimeWindowMinimum
		1000,	// TimeWindowMaximum
		200	// StepSize
	},
	Package () {	// Power Limit 2
		1,	// PowerLimitIndex, 1 for Power Limit 2
		8000,	// PowerLimitMinimum
		8000,	// PowerLimitMaximum
		1000,	// TimeWindowMinimum
		1000,	// TimeWindowMaximum
		1000	// StepSize
	}
})

/* Include SoC DPTF */
#include <soc/intel/baytrail/acpi/dptf/dptf.asl>
