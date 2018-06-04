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

#include <bootstate.h>
#include <boot/coreboot_tables.h>
#include <console/console.h>
#include <cpu/cpu.h>
#include <arch/io.h>
#include <string.h>
#include <cpu/x86/mp.h>
#include <cpu/x86/lapic.h>
#include <cpu/x86/tsc.h>
#include <arch/cpu.h>
#include <device/path.h>
#include <device/device.h>
#include <smp/spinlock.h>

#ifndef __x86_64__
/* Standard macro to see if a specific flag is changeable */
static inline int flag_is_changeable_p(uint32_t flag)
{
	uint32_t f1, f2;

	asm(
		"pushfl\n\t"
		"pushfl\n\t"
		"popl %0\n\t"
		"movl %0,%1\n\t"
		"xorl %2,%0\n\t"
		"pushl %0\n\t"
		"popfl\n\t"
		"pushfl\n\t"
		"popl %0\n\t"
		"popfl\n\t"
		: "=&r" (f1), "=&r" (f2)
		: "ir" (flag));
	return ((f1^f2) & flag) != 0;
}

/*
 * Cyrix CPUs without cpuid or with cpuid not yet enabled can be detected
 * by the fact that they preserve the flags across the division of 5/2.
 * PII and PPro exhibit this behavior too, but they have cpuid available.
 */

/*
 * Perform the Cyrix 5/2 test. A Cyrix won't change
 * the flags, while other 486 chips will.
 */
static inline int test_cyrix_52div(void)
{
	unsigned int test;

	__asm__ __volatile__(
	     "sahf\n\t"		/* clear flags (%eax = 0x0005) */
	     "div %b2\n\t"	/* divide 5 by 2 */
	     "lahf"		/* store flags into %ah */
	     : "=a" (test)
	     : "0" (5), "q" (2)
	     : "cc");

	/* AH is 0x02 on Cyrix after the divide.. */
	return (unsigned char) (test >> 8) == 0x02;
}

/*
 *	Detect a NexGen CPU running without BIOS hypercode new enough
 *	to have CPUID. (Thanks to Herbert Oppmann)
 */

static int deep_magic_nexgen_probe(void)
{
	int ret;

	__asm__ __volatile__ (
		"	movw	$0x5555, %%ax\n"
		"	xorw	%%dx,%%dx\n"
		"	movw	$2, %%cx\n"
		"	divw	%%cx\n"
		"	movl	$0, %%eax\n"
		"	jnz	1f\n"
		"	movl	$1, %%eax\n"
		"1:\n"
		: "=a" (ret) : : "cx", "dx");
	return  ret;
}
#endif

/* List of CPU vendor strings along with their normalized
 * id values.
 */
static struct {
	int vendor;
	const char *name;
} x86_vendors[] = {
	{ X86_VENDOR_INTEL,     "GenuineIntel", },
	{ X86_VENDOR_CYRIX,     "CyrixInstead", },
	{ X86_VENDOR_AMD,       "AuthenticAMD", },
	{ X86_VENDOR_UMC,       "UMC UMC UMC ", },
	{ X86_VENDOR_NEXGEN,    "NexGenDriven", },
	{ X86_VENDOR_CENTAUR,   "CentaurHauls", },
	{ X86_VENDOR_RISE,      "RiseRiseRise", },
	{ X86_VENDOR_TRANSMETA, "GenuineTMx86", },
	{ X86_VENDOR_TRANSMETA, "TransmetaCPU", },
	{ X86_VENDOR_NSC,       "Geode by NSC", },
	{ X86_VENDOR_SIS,       "SiS SiS SiS ", },
};

static const char * const x86_vendor_name[] = {
	[X86_VENDOR_INTEL]     = "Intel",
	[X86_VENDOR_CYRIX]     = "Cyrix",
	[X86_VENDOR_AMD]       = "AMD",
	[X86_VENDOR_UMC]       = "UMC",
	[X86_VENDOR_NEXGEN]    = "NexGen",
	[X86_VENDOR_CENTAUR]   = "Centaur",
	[X86_VENDOR_RISE]      = "Rise",
	[X86_VENDOR_TRANSMETA] = "Transmeta",
	[X86_VENDOR_NSC]       = "NSC",
	[X86_VENDOR_SIS]       = "SiS",
};

static const char *cpu_vendor_name(int vendor)
{
	const char *name;
	name = "<invalid CPU vendor>";
	if ((vendor < (ARRAY_SIZE(x86_vendor_name))) &&
		(x86_vendor_name[vendor] != 0))
		name = x86_vendor_name[vendor];
	return name;
}

static void identify_cpu(struct device *cpu)
{
	char vendor_name[16];
	int i;

	vendor_name[0] = '\0'; /* Unset */

#ifndef __x86_64__
	/* Find the id and vendor_name */
	if (!cpu_have_cpuid()) {
		/* Its a 486 if we can modify the AC flag */
		if (flag_is_changeable_p(X86_EFLAGS_AC))
			cpu->device = 0x00000400; /* 486 */
		else
			cpu->device = 0x00000300; /* 386 */
		if ((cpu->device == 0x00000400) && test_cyrix_52div())
			memcpy(vendor_name, "CyrixInstead", 13);
			/* If we ever care we can enable cpuid here */
		/* Detect NexGen with old hypercode */
		else if (deep_magic_nexgen_probe())
			memcpy(vendor_name, "NexGenDriven", 13);
	}
#endif
	if (cpu_have_cpuid()) {
		int  cpuid_level;
		struct cpuid_result result;
		result = cpuid(0x00000000);
		cpuid_level     = result.eax;
		vendor_name[0]  = (result.ebx >>  0) & 0xff;
		vendor_name[1]  = (result.ebx >>  8) & 0xff;
		vendor_name[2]  = (result.ebx >> 16) & 0xff;
		vendor_name[3]  = (result.ebx >> 24) & 0xff;
		vendor_name[4]  = (result.edx >>  0) & 0xff;
		vendor_name[5]  = (result.edx >>  8) & 0xff;
		vendor_name[6]  = (result.edx >> 16) & 0xff;
		vendor_name[7]  = (result.edx >> 24) & 0xff;
		vendor_name[8]  = (result.ecx >>  0) & 0xff;
		vendor_name[9]  = (result.ecx >>  8) & 0xff;
		vendor_name[10] = (result.ecx >> 16) & 0xff;
		vendor_name[11] = (result.ecx >> 24) & 0xff;
		vendor_name[12] = '\0';

		/* Intel-defined flags: level 0x00000001 */
		if (cpuid_level >= 0x00000001)
			cpu->device = cpuid_eax(0x00000001);
		else
			/* Have CPUID level 0 only unheard of */
			cpu->device = 0x00000400;
	}
	cpu->vendor = X86_VENDOR_UNKNOWN;
	for (i = 0; i < ARRAY_SIZE(x86_vendors); i++) {
		if (memcmp(vendor_name, x86_vendors[i].name, 12) == 0) {
			cpu->vendor = x86_vendors[i].vendor;
			break;
		}
	}
}

struct cpu_driver *find_cpu_driver(struct device *cpu)
{
	struct cpu_driver *driver;
	for (driver = _cpu_drivers; driver < _ecpu_drivers; driver++) {
		const struct cpu_device_id *id;
		for (id = driver->id_table;
		     id->vendor != X86_VENDOR_INVALID; id++) {
			if ((cpu->vendor == id->vendor) &&
				(cpu->device == id->device))
				return driver;
			if (id->vendor == X86_VENDOR_ANY)
				return driver;
		}
	}
	return NULL;
}

static void set_cpu_ops(struct device *cpu)
{
	struct cpu_driver *driver = find_cpu_driver(cpu);
	cpu->ops = driver ? driver->ops : NULL;
}

void cpu_initialize(unsigned int index)
{
	/* Because we busy wait at the printk spinlock.
	 * It is important to keep the number of printed messages
	 * from secondary cpus to a minimum, when debugging is
	 * disabled.
	 */
	struct device *cpu;
	struct cpu_info *info;
	struct cpuinfo_x86 c;

	info = cpu_info();

	printk(BIOS_INFO, "Initializing CPU #%d\n", index);

	cpu = info->cpu;
	if (!cpu)
		die("CPU: missing CPU device structure");

	if (cpu->initialized)
		return;

	post_log_path(cpu);

	/* Find what type of CPU we are dealing with */
	identify_cpu(cpu);
	printk(BIOS_DEBUG, "CPU: vendor %s device %x\n",
		cpu_vendor_name(cpu->vendor), cpu->device);

	get_fms(&c, cpu->device);

	printk(BIOS_DEBUG, "CPU: family %02x, model %02x, stepping %02x\n",
		c.x86, c.x86_model, c.x86_mask);

	/* Lookup the cpu's operations */
	set_cpu_ops(cpu);

	if (!cpu->ops) {
		/* mask out the stepping and try again */
		cpu->device -= c.x86_mask;
		set_cpu_ops(cpu);
		cpu->device += c.x86_mask;
		if (!cpu->ops)
			die("Unknown cpu");
		printk(BIOS_DEBUG, "Using generic CPU ops (good)\n");
	}


	/* Initialize the CPU */
	if (cpu->ops && cpu->ops->init) {
		cpu->enabled = 1;
		cpu->initialized = 1;
		cpu->ops->init(cpu);
	}
	post_log_clear();

	printk(BIOS_INFO, "CPU #%d initialized\n", index);
}

void lb_arch_add_records(struct lb_header *header)
{
	uint32_t freq_khz;
	struct lb_tsc_info *tsc_info;

	/* Don't advertise a TSC rate unless it's constant. */
	if (!IS_ENABLED(CONFIG_TSC_CONSTANT_RATE))
		return;

	freq_khz = tsc_freq_mhz() * 1000;

	/* No use exposing a TSC frequency that is zero. */
	if (freq_khz == 0)
		return;

	tsc_info = (void *)lb_new_record(header);
	tsc_info->tag = LB_TAG_TSC_INFO;
	tsc_info->size = sizeof(*tsc_info);
	tsc_info->freq_khz = freq_khz;
}

void arch_bootstate_coreboot_exit(void)
{
	/* APs are already parked by existing infrastructure. */
	if (!IS_ENABLED(CONFIG_PARALLEL_MP_AP_WORK))
		return;

	/* APs are waiting for work. Last thing to do is park them. */
	mp_park_aps();
}
