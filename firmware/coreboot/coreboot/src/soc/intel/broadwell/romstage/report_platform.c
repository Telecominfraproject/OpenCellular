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

#include <arch/cpu.h>
#include <arch/io.h>
#include <console/console.h>
#include <device/pci.h>
#include <string.h>
#include <cpu/x86/msr.h>
#include <soc/cpu.h>
#include <soc/pch.h>
#include <soc/pci_devs.h>
#include <soc/romstage.h>
#include <soc/systemagent.h>

static struct {
	u32 cpuid;
	const char *name;
} cpu_table[] = {
	{ CPUID_HASWELL_A0,     "Haswell A0" },
	{ CPUID_HASWELL_B0,     "Haswell B0" },
	{ CPUID_HASWELL_C0,     "Haswell C0" },
	{ CPUID_HASWELL_ULT_B0, "Haswell ULT B0" },
	{ CPUID_HASWELL_ULT,    "Haswell ULT C0 or D0" },
	{ CPUID_HASWELL_HALO,   "Haswell Perf Halo" },
	{ CPUID_BROADWELL_C0,   "Broadwell C0" },
	{ CPUID_BROADWELL_D0,   "Broadwell D0" },
	{ CPUID_BROADWELL_E0,   "Broadwell E0 or F0" },
};

static struct {
	u8 revid;
	const char *name;
} mch_rev_table[] = {
	{ MCH_BROADWELL_REV_D0, "Broadwell D0" },
	{ MCH_BROADWELL_REV_E0, "Broadwell E0" },
	{ MCH_BROADWELL_REV_F0, "Broadwell F0" },
};

static struct {
	u16 lpcid;
	const char *name;
} pch_table[] = {
	{ PCH_LPT_LP_SAMPLE,     "LynxPoint LP Sample" },
	{ PCH_LPT_LP_PREMIUM,    "LynxPoint LP Premium" },
	{ PCH_LPT_LP_MAINSTREAM, "LynxPoint LP Mainstream" },
	{ PCH_LPT_LP_VALUE,      "LynxPoint LP Value" },
	{ PCH_WPT_HSW_U_SAMPLE,  "Haswell U Sample" },
	{ PCH_WPT_BDW_U_SAMPLE,  "Broadwell U Sample" },
	{ PCH_WPT_BDW_U_PREMIUM, "Broadwell U Premium" },
	{ PCH_WPT_BDW_U_BASE,    "Broadwell U Base" },
	{ PCH_WPT_BDW_Y_SAMPLE,  "Broadwell Y Sample" },
	{ PCH_WPT_BDW_Y_PREMIUM, "Broadwell Y Premium" },
	{ PCH_WPT_BDW_Y_BASE,    "Broadwell Y Base" },
	{ PCH_WPT_BDW_H,         "Broadwell H" },
};

static struct {
	u16 igdid;
	const char *name;
} igd_table[] = {
	{ IGD_HASWELL_ULT_GT1,     "Haswell ULT GT1" },
	{ IGD_HASWELL_ULT_GT2,     "Haswell ULT GT2" },
	{ IGD_HASWELL_ULT_GT3,     "Haswell ULT GT3" },
	{ IGD_BROADWELL_U_GT1,     "Broadwell U GT1" },
	{ IGD_BROADWELL_U_GT2,     "Broadwell U GT2" },
	{ IGD_BROADWELL_U_GT3_15W, "Broadwell U GT3 (15W)" },
	{ IGD_BROADWELL_U_GT3_28W, "Broadwell U GT3 (28W)" },
	{ IGD_BROADWELL_Y_GT2,     "Broadwell Y GT2" },
	{ IGD_BROADWELL_H_GT2,     "Broadwell U GT2" },
	{ IGD_BROADWELL_H_GT3,     "Broadwell U GT3" },
};

static void report_cpu_info(void)
{
	struct cpuid_result cpuidr;
	u32 i, index;
	char cpu_string[50], *cpu_name = cpu_string; /* 48 bytes are reported */
	int vt, txt, aes;
	msr_t microcode_ver;
	const char *mode[] = {"NOT ", ""};
	const char *cpu_type = "Unknown";

	index = 0x80000000;
	cpuidr = cpuid(index);
	if (cpuidr.eax < 0x80000004) {
		strcpy(cpu_string, "Platform info not available");
	} else {
		u32 *p = (u32 *)cpu_string;
		for (i = 2; i <= 4 ; i++) {
			cpuidr = cpuid(index + i);
			*p++ = cpuidr.eax;
			*p++ = cpuidr.ebx;
			*p++ = cpuidr.ecx;
			*p++ = cpuidr.edx;
		}
	}
	/* Skip leading spaces in CPU name string */
	while (cpu_name[0] == ' ')
		cpu_name++;

	microcode_ver.lo = 0;
	microcode_ver.hi = 0;
	wrmsr(0x8B, microcode_ver);
	cpuidr = cpuid(1);
	microcode_ver = rdmsr(0x8b);

	/* Look for string to match the name */
	for (i = 0; i < ARRAY_SIZE(cpu_table); i++) {
		if (cpu_table[i].cpuid == cpuidr.eax) {
			cpu_type = cpu_table[i].name;
			break;
		}
	}

	printk(BIOS_DEBUG, "CPU: %s\n", cpu_name);
	printk(BIOS_DEBUG, "CPU: ID %x, %s, ucode: %08x\n",
	       cpuidr.eax, cpu_type, microcode_ver.hi);

	aes = (cpuidr.ecx & (1 << 25)) ? 1 : 0;
	txt = (cpuidr.ecx & (1 << 6)) ? 1 : 0;
	vt = (cpuidr.ecx & (1 << 5)) ? 1 : 0;
	printk(BIOS_DEBUG, "CPU: AES %ssupported, TXT %ssupported, "
	       "VT %ssupported\n", mode[aes], mode[txt], mode[vt]);
}

static void report_mch_info(void)
{
	int i;
	u16 mch_device = pci_read_config16(SA_DEV_ROOT, PCI_DEVICE_ID);
	u8 mch_revision = pci_read_config8(SA_DEV_ROOT, PCI_REVISION_ID);
	const char *mch_type = "Unknown";

	/* Look for string to match the revision for Broadwell U/Y */
	if (mch_device == MCH_BROADWELL_ID_U_Y) {
		for (i = 0; i < ARRAY_SIZE(mch_rev_table); i++) {
			if (mch_rev_table[i].revid == mch_revision) {
				mch_type = mch_rev_table[i].name;
				break;
			}
		}
	}

	printk(BIOS_DEBUG, "MCH: device id %04x (rev %02x) is %s\n",
	       mch_device, mch_revision, mch_type);
}

static void report_pch_info(void)
{
	int i;
	u16 lpcid = pch_type();
	const char *pch_type = "Unknown";

	for (i = 0; i < ARRAY_SIZE(pch_table); i++) {
		if (pch_table[i].lpcid == lpcid) {
			pch_type = pch_table[i].name;
			break;
		}
	}
	printk(BIOS_DEBUG, "PCH: device id %04x (rev %02x) is %s\n",
	       lpcid, pch_revision(), pch_type);
}

static void report_igd_info(void)
{
	int i;
	u16 igdid = pci_read_config16(SA_DEV_IGD, PCI_DEVICE_ID);
	const char *igd_type = "Unknown";

	for (i = 0; i < ARRAY_SIZE(igd_table); i++) {
		if (igd_table[i].igdid == igdid) {
			igd_type = igd_table[i].name;
			break;
		}
	}
	printk(BIOS_DEBUG, "IGD: device id %04x (rev %02x) is %s\n",
	       igdid, pci_read_config8(SA_DEV_IGD, PCI_REVISION_ID), igd_type);
}

void report_platform_info(void)
{
	report_cpu_info();
	report_mch_info();
	report_pch_info();
	report_igd_info();
}

/*
 * Dump in the log memory controller configuration as read from the memory
 * controller registers.
 */
void report_memory_config(void)
{
	u32 addr_decoder_common, addr_decode_ch[2];
	int i;

	addr_decoder_common = MCHBAR32(0x5000);
	addr_decode_ch[0] = MCHBAR32(0x5004);
	addr_decode_ch[1] = MCHBAR32(0x5008);

	printk(BIOS_DEBUG, "memcfg DDR3 clock %d MHz\n",
	       (MCHBAR32(0x5e04) * 13333 * 2 + 50)/100);
	printk(BIOS_DEBUG, "memcfg channel assignment: A: %d, B % d, C % d\n",
	       addr_decoder_common & 3,
	       (addr_decoder_common >> 2) & 3,
	       (addr_decoder_common >> 4) & 3);

	for (i = 0; i < ARRAY_SIZE(addr_decode_ch); i++) {
		u32 ch_conf = addr_decode_ch[i];
		printk(BIOS_DEBUG, "memcfg channel[%d] config (%8.8x):\n",
		       i, ch_conf);
		printk(BIOS_DEBUG, "   enhanced interleave mode %s\n",
		       ((ch_conf >> 22) & 1) ? "on" : "off");
		printk(BIOS_DEBUG, "   rank interleave %s\n",
		       ((ch_conf >> 21) & 1) ? "on" : "off");
		printk(BIOS_DEBUG, "   DIMMA %d MB width %s %s rank%s\n",
		       ((ch_conf >> 0) & 0xff) * 256,
		       ((ch_conf >> 19) & 1) ? "x16" : "x8 or x32",
		       ((ch_conf >> 17) & 1) ? "dual" : "single",
		       ((ch_conf >> 16) & 1) ? "" : ", selected");
		printk(BIOS_DEBUG, "   DIMMB %d MB width %s %s rank%s\n",
		       ((ch_conf >> 8) & 0xff) * 256,
		       ((ch_conf >> 19) & 1) ? "x16" : "x8 or x32",
		       ((ch_conf >> 18) & 1) ? "dual" : "single",
		       ((ch_conf >> 16) & 1) ? ", selected" : "");
	}
}
