/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2007-2009 coresystems GmbH
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
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

#include <arch/acpi.h>
#include <arch/acpigen.h>
#include <arch/cpu.h>
#include <arch/io.h>
#include <arch/smp/mpspec.h>
#include <cbfs.h>
#include <cbmem.h>
#include <compiler.h>
#include <console/console.h>
#include <cpu/cpu.h>
#include <cpu/intel/turbo.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/smm.h>
#include <cpu/x86/tsc.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <ec/google/chromeec/ec.h>
#include <drivers/intel/gma/opregion.h>
#include <rules.h>
#include <soc/acpi.h>
#include <soc/gfx.h>
#include <soc/iomap.h>
#include <soc/irq.h>
#include <soc/msr.h>
#include <soc/pattrs.h>
#include <soc/pci_devs.h>
#include <soc/pm.h>
#include <string.h>
#include <types.h>
#include <vendorcode/google/chromeos/gnvs.h>
#include <wrdd.h>

#define MWAIT_RES(state, sub_state)                         \
	{                                                   \
		.addrl = (((state) << 4) | (sub_state)),    \
		.space_id = ACPI_ADDRESS_SPACE_FIXED,       \
		.bit_width = ACPI_FFIXEDHW_VENDOR_INTEL,    \
		.bit_offset = ACPI_FFIXEDHW_CLASS_MWAIT,    \
		.access_size = ACPI_FFIXEDHW_FLAG_HW_COORD, \
	}

/* C-state map without S0ix */
static acpi_cstate_t cstate_map[] = {
	{
		/* C1 */
		.ctype = 1, /* ACPI C1 */
		.latency = 1,
		.power = 1000,
		.resource = MWAIT_RES(0, 0),
	},
	{
		/* C6NS with no L2 shrink */
		/* NOTE: this substate is above CPUID limit */
		.ctype = 2, /* ACPI C2 */
		.latency = 500,
		.power = 10,
		.resource = MWAIT_RES(5, 1),
	},
	{
		/* C6FS with full L2 shrink */
		.ctype = 3, /* ACPI C3 */
		.latency = 1500, /* 1.5ms worst case */
		.power = 1,
		.resource = MWAIT_RES(5, 2),
	}
};

void acpi_init_gnvs(global_nvs_t *gnvs)
{
	/* Set unknown wake source */
	gnvs->pm1i = -1;

	/* CPU core count */
	gnvs->pcnt = dev_count_cpu();

	/* Top of Low Memory (start of resource allocation) */
	gnvs->tolm = nc_read_top_of_low_memory();

#if IS_ENABLED(CONFIG_CONSOLE_CBMEM)
	/* Update the mem console pointer. */
	gnvs->cbmc = (u32)cbmem_find(CBMEM_ID_CONSOLE);
#endif

#if IS_ENABLED(CONFIG_CHROMEOS)
	/* Initialize Verified Boot data */
	chromeos_init_vboot(&(gnvs->chromeos));
#if IS_ENABLED(CONFIG_EC_GOOGLE_CHROMEEC)
	gnvs->chromeos.vbt2 = google_ec_running_ro() ?
		ACTIVE_ECFW_RO : ACTIVE_ECFW_RW;
#endif
#endif
}

static int acpi_sci_irq(void)
{
	u32 *actl = (u32 *)(ILB_BASE_ADDRESS + ACTL);
	int scis;
	static int sci_irq;

	if (sci_irq)
		return sci_irq;

	/* Determine how SCI is routed. */
	scis = read32(actl) & SCIS_MASK;
	switch (scis) {
	case SCIS_IRQ9:
	case SCIS_IRQ10:
	case SCIS_IRQ11:
		sci_irq = scis - SCIS_IRQ9 + 9;
		break;
	case SCIS_IRQ20:
	case SCIS_IRQ21:
	case SCIS_IRQ22:
	case SCIS_IRQ23:
		sci_irq = scis - SCIS_IRQ20 + 20;
		break;
	default:
		printk(BIOS_DEBUG, "Invalid SCI route! Defaulting to IRQ9.\n");
		sci_irq = 9;
		break;
	}

	printk(BIOS_DEBUG, "SCI is IRQ%d\n", sci_irq);
	return sci_irq;
}

unsigned long acpi_fill_mcfg(unsigned long current)
{
	current += acpi_create_mcfg_mmconfig((acpi_mcfg_mmconfig_t *)current,
					     MCFG_BASE_ADDRESS, 0, 0, 255);
	return current;
}

void acpi_fill_in_fadt(acpi_fadt_t *fadt)
{
	const uint16_t pmbase = ACPI_BASE_ADDRESS;

	fadt->sci_int = acpi_sci_irq();
	fadt->smi_cmd = APM_CNT;
	fadt->acpi_enable = APM_CNT_ACPI_ENABLE;
	fadt->acpi_disable = APM_CNT_ACPI_DISABLE;
	fadt->s4bios_req = 0x0;
	fadt->pstate_cnt = 0;

	fadt->pm1a_evt_blk = pmbase + PM1_STS;
	fadt->pm1b_evt_blk = 0x0;
	fadt->pm1a_cnt_blk = pmbase + PM1_CNT;
	fadt->pm1b_cnt_blk = 0x0;
	fadt->pm2_cnt_blk = pmbase + PM2A_CNT_BLK;
	fadt->pm_tmr_blk = pmbase + PM1_TMR;
	fadt->gpe0_blk = pmbase + GPE0_STS;
	fadt->gpe1_blk = 0;

	fadt->pm1_evt_len = 4;
	fadt->pm1_cnt_len = 2;
	fadt->pm2_cnt_len = 1;
	fadt->pm_tmr_len = 4;
	fadt->gpe0_blk_len = 2 * (GPE0_EN - GPE0_STS);
	fadt->gpe1_blk_len = 0;
	fadt->gpe1_base = 0;
	fadt->cst_cnt = 0;
	fadt->p_lvl2_lat = 1;
	fadt->p_lvl3_lat = 87;
	fadt->flush_size = 1024;
	fadt->flush_stride = 16;
	fadt->duty_offset = 1;
	fadt->duty_width = 0;
	fadt->day_alrm = 0xd;
	fadt->mon_alrm = 0x00;
	fadt->century = 0x00;
	fadt->iapc_boot_arch = ACPI_FADT_LEGACY_DEVICES | ACPI_FADT_8042;

	fadt->flags = ACPI_FADT_WBINVD | ACPI_FADT_C1_SUPPORTED |
			ACPI_FADT_C2_MP_SUPPORTED | ACPI_FADT_SLEEP_BUTTON |
			ACPI_FADT_RESET_REGISTER | ACPI_FADT_SEALED_CASE |
			ACPI_FADT_S4_RTC_WAKE | ACPI_FADT_PLATFORM_CLOCK;

	fadt->reset_reg.space_id = 1;
	fadt->reset_reg.bit_width = 8;
	fadt->reset_reg.bit_offset = 0;
	fadt->reset_reg.resv = 0;
	fadt->reset_reg.addrl = 0xcf9;
	fadt->reset_reg.addrh = 0;
	fadt->reset_value = 6;

	fadt->x_pm1a_evt_blk.space_id = 1;
	fadt->x_pm1a_evt_blk.bit_width = fadt->pm1_evt_len * 8;
	fadt->x_pm1a_evt_blk.bit_offset = 0;
	fadt->x_pm1a_evt_blk.resv = 0;
	fadt->x_pm1a_evt_blk.addrl = pmbase + PM1_STS;
	fadt->x_pm1a_evt_blk.addrh = 0x0;

	fadt->x_pm1b_evt_blk.space_id = 1;
	fadt->x_pm1b_evt_blk.bit_width = 0;
	fadt->x_pm1b_evt_blk.bit_offset = 0;
	fadt->x_pm1b_evt_blk.resv = 0;
	fadt->x_pm1b_evt_blk.addrl = 0x0;
	fadt->x_pm1b_evt_blk.addrh = 0x0;

	fadt->x_pm1a_cnt_blk.space_id = 1;
	fadt->x_pm1a_cnt_blk.bit_width = fadt->pm1_cnt_len * 8;
	fadt->x_pm1a_cnt_blk.bit_offset = 0;
	fadt->x_pm1a_cnt_blk.resv = 0;
	fadt->x_pm1a_cnt_blk.addrl = pmbase + PM1_CNT;
	fadt->x_pm1a_cnt_blk.addrh = 0x0;

	fadt->x_pm1b_cnt_blk.space_id = 1;
	fadt->x_pm1b_cnt_blk.bit_width = 0;
	fadt->x_pm1b_cnt_blk.bit_offset = 0;
	fadt->x_pm1b_cnt_blk.resv = 0;
	fadt->x_pm1b_cnt_blk.addrl = 0x0;
	fadt->x_pm1b_cnt_blk.addrh = 0x0;

	fadt->x_pm2_cnt_blk.space_id = 1;
	fadt->x_pm2_cnt_blk.bit_width = fadt->pm2_cnt_len * 8;
	fadt->x_pm2_cnt_blk.bit_offset = 0;
	fadt->x_pm2_cnt_blk.resv = 0;
	fadt->x_pm2_cnt_blk.addrl = pmbase + PM2A_CNT_BLK;
	fadt->x_pm2_cnt_blk.addrh = 0x0;

	fadt->x_pm_tmr_blk.space_id = 1;
	fadt->x_pm_tmr_blk.bit_width = fadt->pm_tmr_len * 8;
	fadt->x_pm_tmr_blk.bit_offset = 0;
	fadt->x_pm_tmr_blk.resv = 0;
	fadt->x_pm_tmr_blk.addrl = pmbase + PM1_TMR;
	fadt->x_pm_tmr_blk.addrh = 0x0;

	fadt->x_gpe0_blk.space_id = 1;
	fadt->x_gpe0_blk.bit_width = fadt->gpe0_blk_len * 8;
	fadt->x_gpe0_blk.bit_offset = 0;
	fadt->x_gpe0_blk.resv = 0;
	fadt->x_gpe0_blk.addrl = pmbase + GPE0_STS;
	fadt->x_gpe0_blk.addrh = 0x0;

	fadt->x_gpe1_blk.space_id = 1;
	fadt->x_gpe1_blk.bit_width = 0;
	fadt->x_gpe1_blk.bit_offset = 0;
	fadt->x_gpe1_blk.resv = 0;
	fadt->x_gpe1_blk.addrl = 0x0;
	fadt->x_gpe1_blk.addrh = 0x0;
}

static acpi_tstate_t soc_tss_table[] = {
	{ 100, 1000, 0, 0x00, 0 },
	{ 88, 875, 0, 0x1e, 0 },
	{ 75, 750, 0, 0x1c, 0 },
	{ 63, 625, 0, 0x1a, 0 },
	{ 50, 500, 0, 0x18, 0 },
	{ 38, 375, 0, 0x16, 0 },
	{ 25, 250, 0, 0x14, 0 },
	{ 13, 125, 0, 0x12, 0 },
};

static void generate_t_state_entries(int core, int cores_per_package)
{
	/* Indicate SW_ALL coordination for T-states */
	acpigen_write_TSD_package(core, cores_per_package, SW_ALL);

	/* Indicate FFixedHW so OS will use MSR */
	acpigen_write_empty_PTC();

	/* Set NVS controlled T-state limit */
	acpigen_write_TPC("\\TLVL");

	/* Write TSS table for MSR access */
	acpigen_write_TSS_package(
		ARRAY_SIZE(soc_tss_table), soc_tss_table);
}

static int calculate_power(int tdp, int p1_ratio, int ratio)
{
	u32 m;
	u32 power;

	/*
	 * M = ((1.1 - ((p1_ratio - ratio) * 0.00625)) / 1.1) ^ 2
	 *
	 * Power = (ratio / p1_ratio) * m * tdp
	 */

	m = (110000 - ((p1_ratio - ratio) * 625)) / 11;
	m = (m * m) / 1000;

	power = ((ratio * 100000 / p1_ratio) / 100);
	power *= (m / 100) * (tdp / 1000);
	power /= 1000;

	return (int)power;
}

static void generate_p_state_entries(int core, int cores_per_package)
{
	int ratio_min, ratio_max, ratio_turbo, ratio_step, ratio_range_2;
	int coord_type, power_max, power_unit, num_entries;
	int ratio, power, clock, clock_max;
	int vid, vid_turbo, vid_min, vid_max, vid_range_2;
	u32 control_status;
	const struct pattrs *pattrs = pattrs_get();
	msr_t msr;

	/* Inputs from CPU attributes */
	ratio_max = pattrs->iacore_ratios[IACORE_MAX];
	ratio_min = pattrs->iacore_ratios[IACORE_LFM];
	vid_max = pattrs->iacore_vids[IACORE_MAX];
	vid_min = pattrs->iacore_vids[IACORE_LFM];

	/* Set P-states coordination type based on MSR disable bit */
	coord_type = (pattrs->num_cpus > 2) ? SW_ALL : HW_ALL;

	/* Max Non-Turbo Frequency */
	clock_max = (ratio_max * pattrs->bclk_khz) / 1000;

	/* Calculate CPU TDP in mW */
	msr = rdmsr(MSR_PKG_POWER_SKU_UNIT);
	power_unit = 1 << (msr.lo & 0xf);
	msr = rdmsr(MSR_PKG_POWER_LIMIT);
	power_max = ((msr.lo & 0x7fff) / power_unit) * 1000;

	/* Write _PCT indicating use of FFixedHW */
	acpigen_write_empty_PCT();

	/* Write _PPC with NVS specified limit on supported P-state */
	acpigen_write_PPC_NVS();

	/* Write PSD indicating configured coordination type */
	acpigen_write_PSD_package(core, 1, coord_type);

	/* Add P-state entries in _PSS table */
	acpigen_write_name("_PSS");

	/* Determine ratio points */
	ratio_step = 1;
	num_entries = (ratio_max - ratio_min) / ratio_step;
	while (num_entries > 15) { /* ACPI max is 15 ratios */
		ratio_step <<= 1;
		num_entries >>= 1;
	}

	/* P[T] is Turbo state if enabled */
	if (get_turbo_state() == TURBO_ENABLED) {
		/* _PSS package count including Turbo */
		acpigen_write_package(num_entries + 2);

		ratio_turbo = pattrs->iacore_ratios[IACORE_TURBO];
		vid_turbo = pattrs->iacore_vids[IACORE_TURBO];
		control_status = (ratio_turbo << 8) | vid_turbo;

		/* Add entry for Turbo ratio */
		acpigen_write_PSS_package(
			clock_max + 1,		/* MHz */
			power_max,		/* mW */
			10,			/* lat1 */
			10,			/* lat2 */
			control_status,		/* control */
			control_status);	/* status */
	} else {
		/* _PSS package count without Turbo */
		acpigen_write_package(num_entries + 1);
		ratio_turbo = ratio_max;
		vid_turbo = vid_max;
	}

	/* First regular entry is max non-turbo ratio */
	control_status = (ratio_max << 8) | vid_max;
	acpigen_write_PSS_package(
		clock_max,		/* MHz */
		power_max,		/* mW */
		10,			/* lat1 */
		10,			/* lat2 */
		control_status,		/* control */
		control_status);	/* status */

	/* Set up ratio and vid ranges for VID calculation */
	ratio_range_2 = (ratio_turbo - ratio_min) * 2;
	vid_range_2 = (vid_turbo - vid_min) * 2;

	/* Generate the remaining entries */
	for (ratio = ratio_min + ((num_entries - 1) * ratio_step);
	     ratio >= ratio_min; ratio -= ratio_step) {

		/* Calculate VID for this ratio */
		vid = ((ratio - ratio_min) * vid_range_2) /
			ratio_range_2 + vid_min;
		/* Round up if remainder */
		if (((ratio - ratio_min) * vid_range_2) % ratio_range_2)
			vid++;

		/* Calculate power at this ratio */
		power = calculate_power(power_max, ratio_max, ratio);
		clock = (ratio * pattrs->bclk_khz) / 1000;
		control_status = (ratio << 8) | (vid & 0xff);

		acpigen_write_PSS_package(
			clock,			/* MHz */
			power,			/* mW */
			10,			/* lat1 */
			10,			/* lat2 */
			control_status,		/* control */
			control_status);	/* status */
	}

	/* Fix package length */
	acpigen_pop_len();
}

void generate_cpu_entries(struct device *device)
{
	int core;
	int pcontrol_blk = get_pmbase(), plen = 6;
	const struct pattrs *pattrs = pattrs_get();

	for (core = 0; core < pattrs->num_cpus; core++) {
		if (core > 0) {
			pcontrol_blk = 0;
			plen = 0;
		}

		/* Generate processor \_PR.CPUx */
		acpigen_write_processor(
			core, pcontrol_blk, plen);

		/* Generate  P-state tables */
		generate_p_state_entries(
			core, pattrs->num_cpus);

		/* Generate C-state tables */
		acpigen_write_CST_package(
			cstate_map, ARRAY_SIZE(cstate_map));

		/* Generate T-state tables */
		generate_t_state_entries(
			core, pattrs->num_cpus);

		acpigen_pop_len();
	}
}

unsigned long acpi_madt_irq_overrides(unsigned long current)
{
	int sci_irq = acpi_sci_irq();
	acpi_madt_irqoverride_t *irqovr;
	uint16_t sci_flags = MP_IRQ_TRIGGER_LEVEL;

	/* INT_SRC_OVR */
	irqovr = (void *)current;
	current += acpi_create_madt_irqoverride(irqovr, 0, 0, 2, 0);

	if (sci_irq >= 20)
		sci_flags |= MP_IRQ_POLARITY_LOW;
	else
		sci_flags |= MP_IRQ_POLARITY_HIGH;

	irqovr = (void *)current;
	current += acpi_create_madt_irqoverride(irqovr, 0, sci_irq, sci_irq,
						sci_flags);

	return current;
}

/* Initialize IGD OpRegion, called from ACPI code */
static int update_igd_opregion(igd_opregion_t *opregion)
{
	/* FIXME: Add platform specific mailbox initialization */

	return 0;
}

unsigned long southcluster_write_acpi_tables(struct device *device,
					     unsigned long current,
					     struct acpi_rsdp *rsdp)
{
	acpi_header_t *ssdt2;
	global_nvs_t *gnvs = cbmem_find(CBMEM_ID_ACPI_GNVS);

	current = acpi_write_hpet(device, current, rsdp);
	current = acpi_align_current(current);

	if (IS_ENABLED(CONFIG_INTEL_GMA_ADD_VBT_DATA_FILE)) {
		igd_opregion_t *opregion;

		printk(BIOS_DEBUG, "ACPI:    * IGD OpRegion\n");
		opregion = (igd_opregion_t *)current;
		intel_gma_init_igd_opregion(opregion);
		if (gnvs)
			gnvs->aslb = (u32)opregion;
		update_igd_opregion(opregion);
		current += sizeof(igd_opregion_t);
		current = acpi_align_current(current);
	}

	ssdt2 = (acpi_header_t *)current;
	memset(ssdt2, 0, sizeof(acpi_header_t));
	acpi_create_serialio_ssdt(ssdt2);
	if (ssdt2->length) {
		current += ssdt2->length;
		acpi_add_table(rsdp, ssdt2);
		printk(BIOS_DEBUG, "ACPI:     * SSDT2 @ %p Length %x\n", ssdt2,
		       ssdt2->length);
		current = acpi_align_current(current);
	} else {
		ssdt2 = NULL;
		printk(BIOS_DEBUG, "ACPI:     * SSDT2 not generated.\n");
	}

	printk(BIOS_DEBUG, "current = %lx\n", current);

	return current;
}

void southcluster_inject_dsdt(struct device *device)
{
	global_nvs_t *gnvs;

	gnvs = cbmem_find(CBMEM_ID_ACPI_GNVS);
	if (!gnvs) {
		gnvs = cbmem_add(CBMEM_ID_ACPI_GNVS, sizeof(*gnvs));
		if (gnvs)
			memset(gnvs, 0, sizeof(*gnvs));
	}

	if (gnvs) {
		acpi_create_gnvs(gnvs);
		/* Fill in the Wifi Region id */
		if (IS_ENABLED(CONFIG_HAVE_REGULATORY_DOMAIN))
			gnvs->cid1 = wifi_regulatory_domain();
		else
			gnvs->cid1 = WRDD_DEFAULT_REGULATORY_DOMAIN;
		acpi_save_gnvs((unsigned long)gnvs);
		/* And tell SMI about it */
		smm_setup_structures(gnvs, NULL, NULL);

		/* Add it to DSDT.  */
		acpigen_write_scope("\\");
		acpigen_write_name_dword("NVSA", (u32) gnvs);
		acpigen_pop_len();
	}
}

__weak void acpi_create_serialio_ssdt(acpi_header_t *ssdt)
{
}
