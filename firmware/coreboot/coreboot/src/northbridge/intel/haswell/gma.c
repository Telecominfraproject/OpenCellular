/*
 * This file is part of the coreboot project.
 *
 * Copyright 2012 Google Inc.
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

#include <arch/io.h>
#include <cbmem.h>
#include <console/console.h>
#include <bootmode.h>
#include <delay.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <drivers/intel/gma/i915_reg.h>
#include <drivers/intel/gma/i915.h>
#include <drivers/intel/gma/libgfxinit.h>
#include <cpu/intel/haswell/haswell.h>
#include <drivers/intel/gma/opregion.h>
#include <southbridge/intel/lynxpoint/nvs.h>
#include <stdlib.h>
#include <string.h>

#include "chip.h"
#include "haswell.h"

#if IS_ENABLED(CONFIG_CHROMEOS)
#include <vendorcode/google/chromeos/chromeos.h>
#endif

struct gt_reg {
	u32 reg;
	u32 andmask;
	u32 ormask;
};

static const struct gt_reg haswell_gt_setup[] = {
	/* Enable Counters */
	{ 0x0a248, 0x00000000, 0x00000016 },
	{ 0x0a000, 0x00000000, 0x00070020 },
	{ 0x0a180, 0xff3fffff, 0x15000000 },
	/* Enable DOP Clock Gating */
	{ 0x09424, 0x00000000, 0x000003fd },
	/* Enable Unit Level Clock Gating */
	{ 0x09400, 0x00000000, 0x00000080 },
	{ 0x09404, 0x00000000, 0x40401000 },
	{ 0x09408, 0x00000000, 0x00000000 },
	{ 0x0940c, 0x00000000, 0x02000001 },
	{ 0x0a008, 0x00000000, 0x08000000 },
	/* Wake Rate Limits */
	{ 0x0a090, 0xffffffff, 0x00000000 },
	{ 0x0a098, 0xffffffff, 0x03e80000 },
	{ 0x0a09c, 0xffffffff, 0x00280000 },
	{ 0x0a0a8, 0xffffffff, 0x0001e848 },
	{ 0x0a0ac, 0xffffffff, 0x00000019 },
	/* Render/Video/Blitter Idle Max Count */
	{ 0x02054, 0x00000000, 0x0000000a },
	{ 0x12054, 0x00000000, 0x0000000a },
	{ 0x22054, 0x00000000, 0x0000000a },
	/* RC Sleep / RCx Thresholds */
	{ 0x0a0b0, 0xffffffff, 0x00000000 },
	{ 0x0a0b4, 0xffffffff, 0x000003e8 },
	{ 0x0a0b8, 0xffffffff, 0x0000c350 },
	/* RP Settings */
	{ 0x0a010, 0xffffffff, 0x000f4240 },
	{ 0x0a014, 0xffffffff, 0x12060000 },
	{ 0x0a02c, 0xffffffff, 0x0000e808 },
	{ 0x0a030, 0xffffffff, 0x0003bd08 },
	{ 0x0a068, 0xffffffff, 0x000101d0 },
	{ 0x0a06c, 0xffffffff, 0x00055730 },
	{ 0x0a070, 0xffffffff, 0x0000000a },
	/* RP Control */
	{ 0x0a024, 0x00000000, 0x00000b92 },
	/* HW RC6 Control */
	{ 0x0a090, 0x00000000, 0x88040000 },
	/* Video Frequency Request */
	{ 0x0a00c, 0x00000000, 0x08000000 },
	{ 0 },
};

static const struct gt_reg haswell_gt_lock[] = {
	{ 0x0a248, 0xffffffff, 0x80000000 },
	{ 0x0a004, 0xffffffff, 0x00000010 },
	{ 0x0a080, 0xffffffff, 0x00000004 },
	{ 0x0a180, 0xffffffff, 0x80000000 },
	{ 0 },
};

/* some vga option roms are used for several chipsets but they only have one
 * PCI ID in their header. If we encounter such an option rom, we need to do
 * the mapping ourselves
 */

u32 map_oprom_vendev(u32 vendev)
{
	u32 new_vendev = vendev;

	switch (vendev) {
	case 0x80860402:		/* GT1 Desktop */
	case 0x80860406:		/* GT1 Mobile */
	case 0x8086040a:		/* GT1 Server */
	case 0x80860a06:		/* GT1 ULT */

	case 0x80860412:		/* GT2 Desktop */
	case 0x80860416:		/* GT2 Mobile */
	case 0x8086041a:		/* GT2 Server */
	case 0x80860a16:		/* GT2 ULT */

	case 0x80860422:		/* GT3 Desktop */
	case 0x80860426:		/* GT3 Mobile */
	case 0x8086042a:		/* GT3 Server */
	case 0x80860a26:		/* GT3 ULT */

		new_vendev = 0x80860406;	/* GT1 Mobile */
		break;
	}

	return new_vendev;
}

/* GTT is the Global Translation Table for the graphics pipeline.
 * It is used to translate graphics addresses to physical
 * memory addresses. As in the CPU, GTTs map 4K pages.
 * The setgtt function adds a further bit of flexibility:
 * it allows you to set a range (the first two parameters) to point
 * to a physical address (third parameter);the physical address is
 * incremented by a count (fourth parameter) for each GTT in the
 * range.
 * Why do it this way? For ultrafast startup,
 * we can point all the GTT entries to point to one page,
 * and set that page to 0s:
 * memset(physbase, 0, 4096);
 * setgtt(0, 4250, physbase, 0);
 * this takes about 2 ms, and is a win because zeroing
 * the page takes a up to 200 ms.
 * This call sets the GTT to point to a linear range of pages
 * starting at physbase.
 */

#define GTT_PTE_BASE (2 << 20)

void
set_translation_table(int start, int end, u64 base, int inc)
{
	int i;

	for (i = start; i < end; i++){
		u64 physical_address = base + i*inc;
		/* swizzle the 32:39 bits to 4:11 */
		u32 word = physical_address | ((physical_address >> 28) & 0xff0) | 1;
		/* note: we've confirmed by checking
		 * the values that mrc does no
		 * useful setup before we run this.
		 */
		gtt_write(GTT_PTE_BASE + i * 4, word);
		gtt_read(GTT_PTE_BASE + i * 4);
	}
}

static struct resource *gtt_res = NULL;

u32 gtt_read(u32 reg)
{
	u32 val;
	val = read32(res2mmio(gtt_res, reg, 0));
	return val;

}

void gtt_write(u32 reg, u32 data)
{
	write32(res2mmio(gtt_res, reg, 0), data);
}

static inline void gtt_rmw(u32 reg, u32 andmask, u32 ormask)
{
	u32 val = gtt_read(reg);
	val &= andmask;
	val |= ormask;
	gtt_write(reg, val);
}

static inline void gtt_write_regs(const struct gt_reg *gt)
{
	for (; gt && gt->reg; gt++) {
		if (gt->andmask)
			gtt_rmw(gt->reg, gt->andmask, gt->ormask);
		else
			gtt_write(gt->reg, gt->ormask);
	}
}

#define GTT_RETRY 1000
int gtt_poll(u32 reg, u32 mask, u32 value)
{
	unsigned try = GTT_RETRY;
	u32 data;

	while (try--) {
		data = gtt_read(reg);
		if ((data & mask) == value)
			return 1;
		udelay(10);
	}

	printk(BIOS_ERR, "GT init timeout\n");
	return 0;
}

uintptr_t gma_get_gnvs_aslb(const void *gnvs)
{
	const global_nvs_t *gnvs_ptr = gnvs;
	return (uintptr_t)(gnvs_ptr ? gnvs_ptr->aslb : 0);
}

void gma_set_gnvs_aslb(void *gnvs, uintptr_t aslb)
{
	global_nvs_t *gnvs_ptr = gnvs;
	if (gnvs_ptr)
		gnvs_ptr->aslb = aslb;
}

static void power_well_enable(void)
{
	gtt_write(HSW_PWR_WELL_CTL1, HSW_PWR_WELL_ENABLE);
	gtt_poll(HSW_PWR_WELL_CTL1, HSW_PWR_WELL_STATE, HSW_PWR_WELL_STATE);

	/* In the native graphics case, we've got about 20 ms.
	 * after we power up the the AUX channel until we can talk to it.
	 * So get that going right now. We can't turn on the panel, yet, just VDD.
	 */
	if (IS_ENABLED(CONFIG_MAINBOARD_DO_NATIVE_VGA_INIT)) {
		gtt_write(PCH_PP_CONTROL, PCH_PP_UNLOCK| EDP_FORCE_VDD | PANEL_POWER_RESET);
	}
}

static void gma_pm_init_pre_vbios(struct device *dev)
{
	printk(BIOS_DEBUG, "GT Power Management Init\n");

	gtt_res = find_resource(dev, PCI_BASE_ADDRESS_0);
	if (!gtt_res || !gtt_res->base)
		return;

	power_well_enable();

	/*
	 * Enable RC6
	 */

	/* Enable Force Wake */
	gtt_write(0x0a180, 1 << 5);
	gtt_write(0x0a188, 0x00010001);
	gtt_poll(FORCEWAKE_ACK_HSW, 1 << 0, 1 << 0);

	/* GT Settings */
	gtt_write_regs(haswell_gt_setup);

	/* Wait for Mailbox Ready */
	gtt_poll(0x138124, (1UL << 31), (0UL << 31));
	/* Mailbox Data - RC6 VIDS */
	gtt_write(0x138128, 0x00000000);
	/* Mailbox Command */
	gtt_write(0x138124, 0x80000004);
	/* Wait for Mailbox Ready */
	gtt_poll(0x138124, (1UL << 31), (0UL << 31));

	/* Enable PM Interrupts */
	gtt_write(GEN6_PMIER, GEN6_PM_MBOX_EVENT | GEN6_PM_THERMAL_EVENT |
		  GEN6_PM_RP_DOWN_TIMEOUT | GEN6_PM_RP_UP_THRESHOLD |
		  GEN6_PM_RP_DOWN_THRESHOLD | GEN6_PM_RP_UP_EI_EXPIRED |
		  GEN6_PM_RP_DOWN_EI_EXPIRED);

	/* Enable RC6 in idle */
	gtt_write(0x0a094, 0x00040000);

	/* PM Lock Settings */
	gtt_write_regs(haswell_gt_lock);
}

static void init_display_planes(void)
{
	int pipe, plane;

	/* Disable cursor mode */
	for (pipe = PIPE_A; pipe <= PIPE_C; pipe++) {
		gtt_write(CURCNTR_IVB(pipe), CURSOR_MODE_DISABLE);
		gtt_write(CURBASE_IVB(pipe), 0x00000000);
	}

	/* Disable primary plane and set surface base address*/
	for (plane = PLANE_A; plane <= PLANE_C; plane++) {
		gtt_write(DSPCNTR(plane), DISPLAY_PLANE_DISABLE);
		gtt_write(DSPSURF(plane), 0x00000000);
	}

	/* Disable VGA display */
	gtt_write(CPU_VGACNTRL, CPU_VGA_DISABLE);
}

static void gma_setup_panel(struct device *dev)
{
	struct northbridge_intel_haswell_config *conf = dev->chip_info;
	u32 reg32;

	printk(BIOS_DEBUG, "GT Power Management Init (post VBIOS)\n");

	/* Setup Digital Port Hotplug */
	reg32 = gtt_read(PCH_PORT_HOTPLUG);
	if (!reg32) {
		reg32 = (conf->gpu_dp_b_hotplug & 0x7) << 2;
		reg32 |= (conf->gpu_dp_c_hotplug & 0x7) << 10;
		reg32 |= (conf->gpu_dp_d_hotplug & 0x7) << 18;
		gtt_write(PCH_PORT_HOTPLUG, reg32);
	}

	/* Setup Panel Power On Delays */
	reg32 = gtt_read(PCH_PP_ON_DELAYS);
	if (!reg32) {
		reg32 = (conf->gpu_panel_port_select & 0x3) << 30;
		reg32 |= (conf->gpu_panel_power_up_delay & 0x1fff) << 16;
		reg32 |= (conf->gpu_panel_power_backlight_on_delay & 0x1fff);
		gtt_write(PCH_PP_ON_DELAYS, reg32);
	}

	/* Setup Panel Power Off Delays */
	reg32 = gtt_read(PCH_PP_OFF_DELAYS);
	if (!reg32) {
		reg32 = (conf->gpu_panel_power_down_delay & 0x1fff) << 16;
		reg32 |= (conf->gpu_panel_power_backlight_off_delay & 0x1fff);
		gtt_write(PCH_PP_OFF_DELAYS, reg32);
	}

	/* Setup Panel Power Cycle Delay */
	if (conf->gpu_panel_power_cycle_delay) {
		reg32 = gtt_read(PCH_PP_DIVISOR);
		reg32 &= ~0xff;
		reg32 |= conf->gpu_panel_power_cycle_delay & 0xff;
		gtt_write(PCH_PP_DIVISOR, reg32);
	}

	/* Enable Backlight if needed */
	if (conf->gpu_cpu_backlight) {
		gtt_write(BLC_PWM_CPU_CTL2, BLC_PWM2_ENABLE);
		gtt_write(BLC_PWM_CPU_CTL, conf->gpu_cpu_backlight);
	}
	if (conf->gpu_pch_backlight) {
		gtt_write(BLC_PWM_PCH_CTL1, BLM_PCH_PWM_ENABLE);
		gtt_write(BLC_PWM_PCH_CTL2, conf->gpu_pch_backlight);
	}

	/* Get display,pipeline,and DDI registers into a basic sane state */
	power_well_enable();

	init_display_planes();

	/* DDI-A params set:
	   bit 0: Display detected (RO)
	   bit 4: DDI A supports 4 lanes and DDI E is not used
	   bit 7: DDI buffer is idle
	*/
	gtt_write(DDI_BUF_CTL_A, DDI_BUF_IS_IDLE | DDI_A_4_LANES | DDI_INIT_DISPLAY_DETECTED);

	/* Set FDI registers - is this required? */
	gtt_write(_FDI_RXA_MISC, 0x00200090);
	gtt_write(_FDI_RXA_MISC, 0x0a000000);

	/* Enable the handshake with PCH display when processing reset */
	gtt_write(NDE_RSTWRN_OPT, RST_PCH_HNDSHK_EN);

	/* undocumented */
	gtt_write(0x42090, 0x04000000);
	gtt_write(0x9840, 0x00000000);
	gtt_write(0x42090, 0xa4000000);

	gtt_write(SOUTH_DSPCLK_GATE_D, PCH_LP_PARTITION_LEVEL_DISABLE);

	/* undocumented */
	gtt_write(0x42080, 0x00004000);

	/* Prepare DDI buffers for DP and FDI */
	intel_prepare_ddi();

	/* Hot plug detect buffer enabled for port A */
	gtt_write(DIGITAL_PORT_HOTPLUG_CNTRL, DIGITAL_PORTA_HOTPLUG_ENABLE);

	/* Enable HPD buffer for digital port D and B */
	gtt_write(PCH_PORT_HOTPLUG, PORTD_HOTPLUG_ENABLE | PORTB_HOTPLUG_ENABLE);

	/* Bits 4:0 - Power cycle delay (default 0x6 --> 500ms)
	   Bits 31:8 - Reference divider (0x0004af ----> 24MHz)
	*/
	gtt_write(PCH_PP_DIVISOR, 0x0004af06);
}

static void gma_pm_init_post_vbios(struct device *dev)
{
	int cdclk = 0;
	int devid = pci_read_config16(dev, PCI_DEVICE_ID);
	int gpu_is_ulx = 0;

	if (devid == 0x0a0e || devid == 0x0a1e)
		gpu_is_ulx = 1;

	/* CD Frequency */
	if ((gtt_read(0x42014) & 0x1000000) || gpu_is_ulx || haswell_is_ult())
		cdclk = 0; /* fixed frequency */
	else
		cdclk = 2; /* variable frequency */

	if (gpu_is_ulx || cdclk != 0)
		gtt_rmw(0x130040, 0xf7ffffff, 0x04000000);
	else
		gtt_rmw(0x130040, 0xf3ffffff, 0x00000000);

	/* More magic */
	if (haswell_is_ult() || gpu_is_ulx) {
		if (!gpu_is_ulx)
			gtt_write(0x138128, 0x00000000);
		else
			gtt_write(0x138128, 0x00000001);
		gtt_write(0x13812c, 0x00000000);
		gtt_write(0x138124, 0x80000017);
	}

	/* Disable Force Wake */
	gtt_write(0x0a188, 0x00010000);
	gtt_poll(FORCEWAKE_ACK_HSW, 1 << 0, 0 << 0);
	gtt_write(0x0a188, 0x00000001);
}

/* Enable SCI to ACPI _GPE._L06 */
static void gma_enable_swsci(void)
{
	u16 reg16;

	/* clear DMISCI status */
	reg16 = inw(get_pmbase() + TCO1_STS);
	reg16 &= DMISCI_STS;
	outw(get_pmbase() + TCO1_STS, reg16);

	/* clear and enable ACPI TCO SCI */
	enable_tco_sci();
}

static void gma_func0_init(struct device *dev)
{
	int lightup_ok = 0;
	u32 reg32;

	/* IGD needs to be Bus Master */
	reg32 = pci_read_config32(dev, PCI_COMMAND);
	reg32 |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY | PCI_COMMAND_IO;
	pci_write_config32(dev, PCI_COMMAND, reg32);

	/* Init graphics power management */
	gma_pm_init_pre_vbios(dev);

	/* Pre panel init */
	gma_setup_panel(dev);

	if (IS_ENABLED(CONFIG_MAINBOARD_USE_LIBGFXINIT)) {
		printk(BIOS_SPEW, "NATIVE graphics, run native enable\n");
		gma_gfxinit(&lightup_ok);
		gfx_set_init_done(1);
	}

	if (! lightup_ok) {
		printk(BIOS_SPEW, "FUI did not run; using VBIOS\n");
		mdelay(CONFIG_PRE_GRAPHICS_DELAY);
		pci_dev_init(dev);
	}

	/* Post panel init */
	gma_pm_init_post_vbios(dev);

	gma_enable_swsci();
	intel_gma_restore_opregion();
}

static void gma_set_subsystem(struct device *dev, unsigned vendor,
			      unsigned device)
{
	if (!vendor || !device) {
		pci_write_config32(dev, PCI_SUBSYSTEM_VENDOR_ID,
				pci_read_config32(dev, PCI_VENDOR_ID));
	} else {
		pci_write_config32(dev, PCI_SUBSYSTEM_VENDOR_ID,
				((device & 0xffff) << 16) | (vendor & 0xffff));
	}
}

const struct i915_gpu_controller_info *
intel_gma_get_controller_info(void)
{
	struct device *dev = dev_find_slot(0, PCI_DEVFN(0x2,0));
	if (!dev) {
		return NULL;
	}
	struct northbridge_intel_haswell_config *chip = dev->chip_info;
	return &chip->gfx;
}

static void gma_ssdt(struct device *device)
{
	const struct i915_gpu_controller_info *gfx = intel_gma_get_controller_info();
	if (!gfx) {
		return;
	}

	drivers_intel_gma_displays_ssdt_generate(gfx);
}

static unsigned long
gma_write_acpi_tables(struct device *const dev, unsigned long current,
		      struct acpi_rsdp *const rsdp)
{
	igd_opregion_t *opregion = (igd_opregion_t *)current;
	global_nvs_t *gnvs;

	if (intel_gma_init_igd_opregion(opregion) != CB_SUCCESS)
		return current;

	current += sizeof(igd_opregion_t);

	/* GNVS has been already set up */
	gnvs = cbmem_find(CBMEM_ID_ACPI_GNVS);
	if (gnvs) {
		/* IGD OpRegion Base Address */
		gma_set_gnvs_aslb(gnvs, (uintptr_t)opregion);
	} else {
		printk(BIOS_ERR, "Error: GNVS table not found.\n");
	}

	current = acpi_align_current(current);
	return current;
}

static struct pci_operations gma_pci_ops = {
	.set_subsystem    = gma_set_subsystem,
};

static struct device_operations gma_func0_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= gma_func0_init,
	.acpi_fill_ssdt_generator = gma_ssdt,
	.scan_bus		= 0,
	.enable			= 0,
	.ops_pci		= &gma_pci_ops,
	.write_acpi_tables	= gma_write_acpi_tables,
};

static const unsigned short pci_device_ids[] = {
	0x0402, /* Desktop GT1 */
	0x0412, /* Desktop GT2 */
	0x0422, /* Desktop GT3 */
	0x0406, /* Mobile GT1 */
	0x0416, /* Mobile GT2 */
	0x0426, /* Mobile GT3 */
	0x0d16, /* Mobile 4+3 GT1 */
	0x0d26, /* Mobile 4+3 GT2 */
	0x0d36, /* Mobile 4+3 GT3 */
	0x0a06, /* ULT GT1 */
	0x0a16, /* ULT GT2 */
	0x0a26, /* ULT GT3 */
	0,
};

static const struct pci_driver pch_lpc __pci_driver = {
	.ops	 = &gma_func0_ops,
	.vendor	 = PCI_VENDOR_ID_INTEL,
	.devices = pci_device_ids,
};
