/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Advanced Micro Devices, Inc.
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

#include <console/console.h>
#include <arch/acpi.h>
#include <arch/io.h>
#include <arch/stages.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/bist.h>
#include <cpu/amd/car.h>
#include <northbridge/amd/agesa/state_machine.h>
#include <northbridge/amd/pi/agesawrapper.h>
#include <northbridge/amd/pi/agesawrapper_call.h>
#include <southbridge/amd/pi/hudson/hudson.h>

void cache_as_ram_main(unsigned long bist, unsigned long cpu_init_detectedx)
{
	u32 val;

	hudson_lpc_port80();

	if (!cpu_init_detectedx && boot_cpu()) {
		post_code(0x30);

#if IS_ENABLED(CONFIG_HUDSON_UART)
		configure_hudson_uart();
#endif
		post_code(0x31);
		console_init();
	}

	/* Halt if there was a built in self test failure */
	post_code(0x34);
	report_bist_failure(bist & 0x7FFFFFFF); /* Mask bit 31. One result of Silicon Observation */

	/* Load MPB */
	val = cpuid_eax(1);
	printk(BIOS_DEBUG, "BSP Family_Model: %08x\n", val);
	printk(BIOS_DEBUG, "cpu_init_detectedx = %08lx\n", cpu_init_detectedx);

	post_code(0x37);
	AGESAWRAPPER(amdinitreset);
	post_code(0x38);
	printk(BIOS_DEBUG, "Got past agesawrapper_amdinitreset\n");

	post_code(0x39);
	AGESAWRAPPER(amdinitearly);

	post_code(0x40);
	AGESAWRAPPER(amdinitpost);
}

void agesa_postcar(struct sysinfo *cb)
{
	post_code(0x41);
	AGESAWRAPPER(amdinitenv);

	if (acpi_is_wakeup_s4()) {
		outb(0xEE, PM_INDEX);
		outb(0x8, PM_DATA);
	}
}
