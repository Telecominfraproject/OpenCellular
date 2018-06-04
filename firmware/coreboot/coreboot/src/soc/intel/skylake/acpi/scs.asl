/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Google Inc.
 * Copyright (C) 2015 Intel Corporation.
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

/* Intel Storage Controllers */

Device (EMMC)
{
	Name (_ADR, 0x001E0004)
	Name (_DDN, "eMMC Controller")
	Name (UUID, ToUUID ("E5C937D0-3553-4D7A-9117-EA4D19C3434D"))
	Name (TEMP, 0)

	OperationRegion (EMCR, PCI_Config, 0x00, 0x100)
	Field (EMCR, WordAcc, NoLock, Preserve)
	{
		Offset (0x84),	/* PMECTRLSTATUS */
		PMCR, 16,
		Offset (0xa2),	/* PG_CONFIG */
		, 2,
		PGEN, 1,	/* PG_ENABLE */
	}

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
			 * Function 9: Device Readiness Durations
			 * Returns a package of five integers covering
			 * various device related delay in PCIe Base Spec.
			 */
			If (LEqual (Arg2, 9)) {
				/*
				 * Function 9 support for revision 3.
				 * ECN link for function definitions
				 * [https://pcisig.com/sites/default/files/
				 * specification_documents/
				 * ECN_fw_latency_optimization_final.pdf]
				 */
				If (LEqual (Arg1, 3)) {
					/*
					 * Integer 0: FW reset time.
					 * Integer 1: FW data link up time.
					 * Integer 2: FW functional level reset
					 * time.
					 * Integer 3: FW D3 hot to D0 time.
					 * Integer 4: FW VF enable time.
					 * set ACPI constant Ones for elements
					 * where overriding the default value
					 * is not desired.
					 */
					Return (Package (5) {0, Ones, Ones,
								    Ones, Ones})
				}
			}
		}
		Return (Buffer() { 0x00 })
	}

	Method (_PS0, 0, Serialized)
	{
		/* Disable Power Gate */
		Store (0, ^PGEN)

		/* Clear bits 31, 6, 2, 0 */
		^^PCRA (PID_SCS, 0x600, 0x7FFFFFBA)
		Sleep (2)

		/* Set bits 31, 6, 2, 0 */
		^^PCRO (PID_SCS, 0x600, 0x80000045)

		/* Set Power State to D0 */
		And (PMCR, 0xFFFC, PMCR)
		Store (PMCR, ^TEMP)
	}

	Method (_PS3, 0, Serialized)
	{
		/* Enable Power Gate */
		Store (1, ^PGEN)

		/* Set Power State to D3 */
		Or (PMCR, 0x0003, PMCR)
		Store (PMCR, ^TEMP)
	}

	Device (CARD)
	{
		Name (_ADR, 0x00000008)
		Method (_RMV, 0, NotSerialized)
		{
			Return (0)
		}
	}
}

#if !IS_ENABLED(CONFIG_EXCLUDE_NATIVE_SD_INTERFACE)
Device (SDXC)
{
	Name (_ADR, 0x001E0006)
	Name (_DDN, "SD Controller")
	Name (TEMP, 0)

	OperationRegion (SDCR, PCI_Config, 0x00, 0x100)
	Field (SDCR, WordAcc, NoLock, Preserve)
	{
		Offset (0x84),	/* PMECTRLSTATUS */
		PMCR, 16,
		Offset (0xa2),	/* PG_CONFIG */
		, 2,
		PGEN, 1,	/* PG_ENABLE */
	}

	Method (_PS0, 0, Serialized)
	{
		/* Disable 20K pull-down on CLK, CMD and DAT lines */
		^^PCRA (PID_GPIOCOM3, 0x4c4, 0xFFFFEFFF)
		^^PCRA (PID_GPIOCOM3, 0x4cc, 0xFFFFEFFF)
		^^PCRA (PID_GPIOCOM3, 0x4d4, 0xFFFFEFFF)
		^^PCRA (PID_GPIOCOM3, 0x4dc, 0xFFFFEFFF)
		^^PCRA (PID_GPIOCOM3, 0x4e4, 0xFFFFEFFF)
		^^PCRA (PID_GPIOCOM3, 0x4f4, 0xFFFFEFFF)

		/* Disable Power Gate */
		Store (0, ^PGEN)

		/* Clear bits 8, 7, 2, 0 */
		^^PCRA (PID_SCS, 0x600, 0xFFFFFE7A)
		Sleep (2)

		/* Set bits 8, 7, 2, 0 */
		^^PCRO (PID_SCS, 0x600, 0x00000185)

		/* Set Power State to D0 */
		And (PMCR, 0xFFFC, PMCR)
		Store (PMCR, ^TEMP)
	}

	Method (_PS3, 0, Serialized)
	{
		/* Enable Power Gate */
		Store (1, ^PGEN)

		/* Set Power State to D3 */
		Or (PMCR, 0x0003, PMCR)
		Store (PMCR, ^TEMP)

		/* Enable 20K pull-down on CLK, CMD and DAT lines */
		^^PCRO (PID_GPIOCOM3, 0x4c4, 0x00001000)
		^^PCRO (PID_GPIOCOM3, 0x4cc, 0x00001000)
		^^PCRO (PID_GPIOCOM3, 0x4d4, 0x00001000)
		^^PCRO (PID_GPIOCOM3, 0x4dc, 0x00001000)
		^^PCRO (PID_GPIOCOM3, 0x4e4, 0x00001000)
		^^PCRO (PID_GPIOCOM3, 0x4f4, 0x00001000)
	}

	Device (CARD)
	{
		Name (_ADR, 0x00000008)
		Method (_RMV, 0, NotSerialized)
		{
			Return (1)
		}
	}
}
#endif
