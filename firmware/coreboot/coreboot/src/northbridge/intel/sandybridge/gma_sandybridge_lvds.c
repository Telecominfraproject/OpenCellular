/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013, 2014 Vladimir Serbinenko
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 or (at your option)
 *  any later version of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/io.h>
#include <console/console.h>
#include <delay.h>
#include <device/device.h>
#include <string.h>

#include <drivers/intel/gma/edid.h>
#include <drivers/intel/gma/i915.h>
#include "gma.h"
#include "chip.h"
#include "sandybridge.h"
#include <pc80/vga.h>
#include <pc80/vga_io.h>
#include <device/pci_def.h>
#include <device/pci_rom.h>

static void train_link(u8 *mmio)
{
	/* Clear interrupts. */
	write32(mmio + DEIIR, 0xffffffff);

	write32(mmio + 0xf000c, 0x2040);
	write32(mmio + 0xf000c, 0x2050);
	write32(mmio + 0x60100, 0x44000);
	write32(mmio + 0xf000c, 0x22050);

	mdelay(1);

	write32(mmio + 0x000f0018, 0x0000008ff);
	write32(mmio + 0x000f1018, 0x0000008ff);

	write32(mmio + 0x000f000c, 0x001a2050);
	write32(mmio + 0x00060100, 0x001c4000);

	write32(mmio + 0x00060100, 0x801c4000);
	write32(mmio + 0x000f000c, 0x801a2050);

	write32(mmio + 0x00060100, 0x801c4000);
	write32(mmio + 0x000f000c, 0x801a2050);
	mdelay(1);

	read32(mmio + 0x000f0014); // = 0x00000100
	write32(mmio + 0x000f0014, 0x00000100);
	write32(mmio + 0x00060100, 0x901c4000);
	write32(mmio + 0x000f000c, 0x801a2150);
	mdelay(1);
	read32(mmio + 0x000f0014); // = 0x00000600
}

static void power_port(u8 *mmio)
{
	read32(mmio + 0x000e1100); // = 0x00000000
	write32(mmio + 0x000e1100, 0x00000000);
	write32(mmio + 0x000e1100, 0x00010000);
	read32(mmio + 0x000e1100); // = 0x00010000
	read32(mmio + 0x000e1100); // = 0x00010000
	read32(mmio + 0x000e1100); // = 0x00000000
	write32(mmio + 0x000e1100, 0x00000000);
	read32(mmio + 0x000e1100); // = 0x00000000
	read32(mmio + 0x000e4200); // = 0x0000001c
	write32(mmio + 0x000e4210, 0x8004003e);
	write32(mmio + 0x000e4214, 0x80060002);
	write32(mmio + 0x000e4218, 0x01000000);
	read32(mmio + 0x000e4210); // = 0x5144003e
	write32(mmio + 0x000e4210, 0x5344003e);
	read32(mmio + 0x000e4210); // = 0x0144003e
	write32(mmio + 0x000e4210, 0x8074003e);
	read32(mmio + 0x000e4210); // = 0x5144003e
	read32(mmio + 0x000e4210); // = 0x5144003e
	write32(mmio + 0x000e4210, 0x5344003e);
	read32(mmio + 0x000e4210); // = 0x0144003e
	write32(mmio + 0x000e4210, 0x8074003e);
	read32(mmio + 0x000e4210); // = 0x5144003e
	read32(mmio + 0x000e4210); // = 0x5144003e
	write32(mmio + 0x000e4210, 0x5344003e);
	read32(mmio + 0x000e4210); // = 0x0144003e
	write32(mmio + 0x000e4210, 0x8074003e);
	read32(mmio + 0x000e4210); // = 0x5144003e
	read32(mmio + 0x000e4210); // = 0x5144003e
	write32(mmio + 0x000e4210, 0x5344003e);
	write32(mmio + 0x000c4030, 0x00001000);
	read32(mmio + 0x000c4000); // = 0x00000000
	write32(mmio + 0x000c4030, 0x00001000);
	read32(mmio + 0x000e1150); // = 0x0000001c
	write32(mmio + 0x000e1150, 0x0000089c);
	write32(mmio + 0x000fcc00, 0x01986f00);
	write32(mmio + 0x000fcc0c, 0x01986f00);
	write32(mmio + 0x000fcc18, 0x01986f00);
	write32(mmio + 0x000fcc24, 0x01986f00);
	read32(mmio + 0x000c4000); // = 0x00000000
	read32(mmio + 0x000e1180); // = 0x40000002
}

int i915lightup_sandy(const struct i915_gpu_controller_info *info,
		u32 physbase, u16 piobase, u8 *mmio, u32 lfb)
{
	int i;
	u8 edid_data[128];
	struct edid edid;
	struct edid_mode *mode;
	u32 hactive, vactive, right_border, bottom_border;
	int hpolarity, vpolarity;
	u32 vsync, hsync, vblank, hblank, hfront_porch, vfront_porch;
	u32 candp1, candn;
	u32 best_delta = 0xffffffff;
	u32 target_frequency;
	u32 pixel_p1 = 1;
	u32 pixel_n = 1;
	u32 pixel_m1 = 1;
	u32 pixel_m2 = 1;
	u32 link_frequency = info->link_frequency_270_mhz ? 270000 : 162000;
	u32 data_m1;
	u32 data_n1 = 0x00800000;
	u32 link_m1;
	u32 link_n1 = 0x00080000;

	if (!IS_ENABLED(CONFIG_MAINBOARD_DO_NATIVE_VGA_INIT))
		return 0;

	if ((bridge_silicon_revision() & BASE_REV_MASK) == BASE_REV_IVB) {
		return i915lightup_ivy(info, physbase, piobase, mmio, lfb);
	}

	write32(mmio + 0x00070080, 0x00000000);
	write32(mmio + DSPCNTR(0), 0x00000000);
	write32(mmio + 0x00071180, 0x00000000);
	write32(mmio + CPU_VGACNTRL, 0x0000298e | VGA_DISP_DISABLE);
	write32(mmio + 0x0007019c, 0x00000000);
	write32(mmio + 0x0007119c, 0x00000000);
	write32(mmio + 0x000ec008, 0x2c010000);
	write32(mmio + 0x000ec020, 0x2c010000);
	write32(mmio + 0x000ec038, 0x2c010000);
	write32(mmio + 0x000ec050, 0x2c010000);
	write32(mmio + 0x000ec408, 0x2c010000);
	write32(mmio + 0x000ec420, 0x2c010000);
	write32(mmio + 0x000ec438, 0x2c010000);
	write32(mmio + 0x000ec450, 0x2c010000);
	vga_gr_write(0x18, 0);
	write32(mmio + 0x00042004, 0x02000000);
	write32(mmio + 0x000fd034, 0x8421ffe0);

	/* Setup GTT.  */
	for (i = 0; i < 0x2000; i++)
	{
		outl((i << 2) | 1, piobase);
		outl(physbase + (i << 12) + 1, piobase + 4);
	}

	vga_misc_write(0x67);

	const u8 cr[] = { 0x5f, 0x4f, 0x50, 0x82, 0x55, 0x81, 0xbf, 0x1f,
		    0x00, 0x4f, 0x0d, 0x0e, 0x00, 0x00, 0x00, 0x00,
		    0x9c, 0x8e, 0x8f, 0x28, 0x1f, 0x96, 0xb9, 0xa3,
		    0xff
	};
	vga_cr_write(0x11, 0);

	for (i = 0; i <= 0x18; i++)
		vga_cr_write(i, cr[i]);

	power_port(mmio);

	intel_gmbus_read_edid(mmio + PCH_GMBUS0, GMBUS_PORT_PANEL, 0x50,
			edid_data, sizeof(edid_data));
	decode_edid(edid_data, sizeof(edid_data), &edid);
	mode = &edid.mode;

	/* Disable screen memory to prevent garbage from appearing.  */
	vga_sr_write(1, vga_sr_read(1) | 0x20);

	hactive = edid.x_resolution;
	vactive = edid.y_resolution;
	right_border = mode->hborder;
	bottom_border = mode->vborder;
	hpolarity = (mode->phsync == '-');
	vpolarity = (mode->pvsync == '-');
	vsync = mode->vspw;
	hsync = mode->hspw;
	vblank = mode->vbl;
	hblank = mode->hbl;
	hfront_porch = mode->hso;
	vfront_porch = mode->vso;

	target_frequency = mode->lvds_dual_channel ? mode->pixel_clock
		: (2 * mode->pixel_clock);

	if (IS_ENABLED(CONFIG_LINEAR_FRAMEBUFFER)) {
		vga_sr_write(1, 1);
		vga_sr_write(0x2, 0xf);
		vga_sr_write(0x3, 0x0);
		vga_sr_write(0x4, 0xe);
		vga_gr_write(0, 0x0);
		vga_gr_write(1, 0x0);
		vga_gr_write(2, 0x0);
		vga_gr_write(3, 0x0);
		vga_gr_write(4, 0x0);
		vga_gr_write(5, 0x0);
		vga_gr_write(6, 0x5);
		vga_gr_write(7, 0xf);
		vga_gr_write(0x10, 0x1);
		vga_gr_write(0x11, 0);

		edid.bytes_per_line = (edid.bytes_per_line + 63) & ~63;

		write32(mmio + DSPCNTR(0), DISPPLANE_BGRX888);
		write32(mmio + DSPADDR(0), 0);
		write32(mmio + DSPSTRIDE(0), edid.bytes_per_line);
		write32(mmio + DSPSURF(0), 0);
		for (i = 0; i < 0x100; i++)
			write32(mmio + LGC_PALETTE(0) + 4 * i, i * 0x010101);
	} else {
		vga_textmode_init();
	}

	/* Find suitable divisors.  */
	for (candp1 = 1; candp1 <= 8; candp1++) {
		for (candn = 5; candn <= 10; candn++) {
			u32 cur_frequency;
			u32 m; /* 77 - 131.  */
			u32 denom; /* 35 - 560.  */
			u32 current_delta;

			denom = candn * candp1 * 7;
			/* Doesn't overflow for up to
			   5000000 kHz = 5 GHz.  */
			m = (target_frequency * denom + 60000) / 120000;

			if (m < 77 || m > 131)
				continue;

			cur_frequency = (120000 * m) / denom;
			if (target_frequency > cur_frequency)
				current_delta = target_frequency - cur_frequency;
			else
				current_delta = cur_frequency - target_frequency;


			if (best_delta > current_delta) {
				best_delta = current_delta;
				pixel_n = candn;
				pixel_p1 = candp1;
				pixel_m2 = ((m + 3) % 5) + 7;
				pixel_m1 = (m - pixel_m2) / 5;
			}
		}
	}

	if (best_delta == 0xffffffff) {
		printk (BIOS_ERR, "Couldn't find GFX clock divisors\n");
		return 0;
	}

	link_m1 = ((uint64_t)link_n1 * mode->pixel_clock) / link_frequency;
	data_m1 = ((uint64_t)data_n1 * 18 * mode->pixel_clock)
		/ (link_frequency * 8 * 4);

	printk(BIOS_INFO, "bringing up panel at resolution %d x %d\n",
	       hactive, vactive);
	printk(BIOS_DEBUG, "Borders %d x %d\n",
	       right_border, bottom_border);
	printk(BIOS_DEBUG, "Blank %d x %d\n",
	       hblank, vblank);
	printk(BIOS_DEBUG, "Sync %d x %d\n",
	       hsync, vsync);
	printk(BIOS_DEBUG, "Front porch %d x %d\n",
	       hfront_porch, vfront_porch);
	printk(BIOS_DEBUG, (info->use_spread_spectrum_clock
			    ? "Spread spectrum clock\n" : "DREF clock\n"));
	printk(BIOS_DEBUG,
	       mode->lvds_dual_channel ? "Dual channel\n" : "Single channel\n");
	printk(BIOS_DEBUG, "Polarities %d, %d\n",
	       hpolarity, vpolarity);
	printk(BIOS_DEBUG, "Data M1=%d, N1=%d\n",
	       data_m1, data_n1);
	printk(BIOS_DEBUG, "Link frequency %d kHz\n",
	       link_frequency);
	printk(BIOS_DEBUG, "Link M1=%d, N1=%d\n",
	       link_m1, link_n1);
	printk(BIOS_DEBUG, "Pixel N=%d, M1=%d, M2=%d, P1=%d\n",
	       pixel_n, pixel_m1, pixel_m2, pixel_p1);
	printk(BIOS_DEBUG, "Pixel clock %d kHz\n",
	       120000 * (5 * pixel_m1 + pixel_m2) / pixel_n
	       / (pixel_p1 * 7));

	write32(mmio + PCH_LVDS,
		(hpolarity << 20) | (vpolarity << 21)
		| (mode->lvds_dual_channel ? LVDS_CLOCK_B_POWERUP_ALL
		   | LVDS_CLOCK_BOTH_POWERUP_ALL : 0)
		| LVDS_BORDER_ENABLE | LVDS_CLOCK_A_POWERUP_ALL
		| LVDS_DETECTED);
	write32(mmio + BLC_PWM_CPU_CTL2, (1 << 31));
	write32(mmio + PCH_DREF_CONTROL, (info->use_spread_spectrum_clock
					  ? 0x1002 : 0x400));
	mdelay(1);
	write32(mmio + PCH_PP_CONTROL, PANEL_UNLOCK_REGS
		| (read32(mmio + PCH_PP_CONTROL) & ~PANEL_UNLOCK_MASK));
	write32(mmio + _PCH_FP0(0),
		((pixel_n - 2) << 16)
		| ((pixel_m1 - 2) << 8) | pixel_m2);
	write32(mmio + PCH_DPLL_SEL, 8);
	write32(mmio + _PCH_DPLL(0),
		DPLL_VCO_ENABLE | DPLLB_MODE_LVDS
		| (mode->lvds_dual_channel ? DPLLB_LVDS_P2_CLOCK_DIV_7
		   : DPLLB_LVDS_P2_CLOCK_DIV_14)
		| (0x10000 << (pixel_p1 - 1))
		| ((info->use_spread_spectrum_clock ? 3 : 0) << 13)
		| (0x1 << (pixel_p1 - 1)));
	mdelay(1);
	write32(mmio + _PCH_DPLL(0),
		DPLL_VCO_ENABLE | DPLLB_MODE_LVDS
		| (mode->lvds_dual_channel ? DPLLB_LVDS_P2_CLOCK_DIV_7
		   : DPLLB_LVDS_P2_CLOCK_DIV_14)
		| (0x10000 << (pixel_p1 - 1))
		| ((info->use_spread_spectrum_clock ? 3 : 0) << 13)
		| (0x1 << (pixel_p1 - 1)));
	/* Re-lock the registers.  */
	write32(mmio + PCH_PP_CONTROL,
		(read32(mmio + PCH_PP_CONTROL) & ~PANEL_UNLOCK_MASK));

	write32(mmio + PCH_LVDS,
		(hpolarity << 20) | (vpolarity << 21)
		| (mode->lvds_dual_channel ? LVDS_CLOCK_B_POWERUP_ALL
		   | LVDS_CLOCK_BOTH_POWERUP_ALL : 0)
		| LVDS_BORDER_ENABLE | LVDS_CLOCK_A_POWERUP_ALL
		| LVDS_DETECTED);

	write32(mmio + HTOTAL(0),
		((hactive + right_border + hblank - 1) << 16)
		| (hactive - 1));
	write32(mmio + HBLANK(0),
		((hactive + right_border + hblank - 1) << 16)
		| (hactive + right_border - 1));
	write32(mmio + HSYNC(0),
		((hactive + right_border + hfront_porch + hsync - 1) << 16)
		| (hactive + right_border + hfront_porch - 1));

	write32(mmio + VTOTAL(0), ((vactive + bottom_border + vblank - 1) << 16)
		| (vactive - 1));
	write32(mmio + VBLANK(0), ((vactive + bottom_border + vblank - 1) << 16)
		| (vactive + bottom_border - 1));
	write32(mmio + VSYNC(0),
		(vactive + bottom_border + vfront_porch + vsync - 1)
		| (vactive + bottom_border + vfront_porch - 1));

	write32(mmio + PIPECONF(0), PIPECONF_DISABLE);

	write32(mmio + PF_WIN_POS(0), 0);
	if (IS_ENABLED(CONFIG_LINEAR_FRAMEBUFFER)) {
		write32(mmio + PIPESRC(0), ((hactive - 1) << 16) | (vactive - 1));
		write32(mmio + PF_CTL(0),0);
		write32(mmio + PF_WIN_SZ(0), 0);
	} else {
		write32(mmio + PIPESRC(0), (639 << 16) | 399);
		write32(mmio + PF_CTL(0),PF_ENABLE | PF_FILTER_MED_3x3);
		write32(mmio + PF_WIN_SZ(0), vactive | (hactive << 16));
	}

	mdelay(1);

	write32(mmio + PIPE_DATA_M1(0), 0x7e000000 | data_m1);
	write32(mmio + PIPE_DATA_N1(0), data_n1);
	write32(mmio + PIPE_LINK_M1(0), link_m1);
	write32(mmio + PIPE_LINK_N1(0), link_n1);

	write32(mmio + 0x000f000c, 0x00002040);
	mdelay(1);
	write32(mmio + 0x000f000c, 0x00002050);
	write32(mmio + 0x00060100, 0x00044000);
	mdelay(1);
	write32(mmio + PIPECONF(0), PIPECONF_BPP_6);
	write32(mmio + 0x000f000c, 0x00022050);
	write32(mmio + PIPECONF(0), PIPECONF_BPP_6 | PIPECONF_DITHER_EN);
	write32(mmio + PIPECONF(0), PIPECONF_ENABLE | PIPECONF_BPP_6 | PIPECONF_DITHER_EN);

	if (IS_ENABLED(CONFIG_LINEAR_FRAMEBUFFER))
		write32(mmio + CPU_VGACNTRL, 0x20298e | VGA_DISP_DISABLE);
	else
		write32(mmio + CPU_VGACNTRL, 0x20298e);

	train_link(mmio);

	if (IS_ENABLED(CONFIG_LINEAR_FRAMEBUFFER)) {
		write32(mmio + DSPCNTR(0), DISPLAY_PLANE_ENABLE | DISPPLANE_BGRX888);
		mdelay(1);
	}

	write32(mmio + TRANS_HTOTAL(0),
		((hactive + right_border + hblank - 1) << 16)
		| (hactive - 1));
	write32(mmio + TRANS_HBLANK(0),
		((hactive + right_border + hblank - 1) << 16)
		| (hactive + right_border - 1));
	write32(mmio + TRANS_HSYNC(0),
		((hactive + right_border + hfront_porch + hsync - 1) << 16)
		| (hactive + right_border + hfront_porch - 1));

	write32(mmio + TRANS_VTOTAL(0),
		((vactive + bottom_border + vblank - 1) << 16)
		| (vactive - 1));
	write32(mmio + TRANS_VBLANK(0),
		((vactive + bottom_border + vblank - 1) << 16)
		| (vactive + bottom_border - 1));
	write32(mmio + TRANS_VSYNC(0),
		(vactive + bottom_border + vfront_porch + vsync - 1)
		| (vactive + bottom_border + vfront_porch - 1));

	write32(mmio + 0x00060100, 0xb01c4000);
	write32(mmio + 0x000f000c, 0x801a2350);
	mdelay(1);

	if (IS_ENABLED(CONFIG_LINEAR_FRAMEBUFFER))
		write32(mmio + TRANSCONF(0), TRANS_ENABLE | TRANS_6BPC
					   | TRANS_STATE_MASK);
	else
		write32(mmio + TRANSCONF(0), TRANS_ENABLE | TRANS_6BPC);

	write32(mmio + PCH_LVDS,
		LVDS_PORT_ENABLE
		| (hpolarity << 20) | (vpolarity << 21)
		| (mode->lvds_dual_channel ? LVDS_CLOCK_B_POWERUP_ALL
		   | LVDS_CLOCK_BOTH_POWERUP_ALL : 0)
		| LVDS_BORDER_ENABLE | LVDS_CLOCK_A_POWERUP_ALL
		| LVDS_DETECTED);

	write32(mmio + PCH_PP_CONTROL, PANEL_UNLOCK_REGS | PANEL_POWER_OFF);
	write32(mmio + PCH_PP_CONTROL, PANEL_UNLOCK_REGS | PANEL_POWER_RESET);
	mdelay(1);
	write32(mmio + PCH_PP_CONTROL, PANEL_UNLOCK_REGS
		| PANEL_POWER_ON | PANEL_POWER_RESET);

	printk (BIOS_DEBUG, "waiting for panel powerup\n");
	while (1) {
		u32 reg32;
		reg32 = read32(mmio + PCH_PP_STATUS);
		if (((reg32 >> 28) & 3) == 0)
			break;
	}
	printk (BIOS_DEBUG, "panel powered up\n");

	write32(mmio + PCH_PP_CONTROL, PANEL_POWER_ON | PANEL_POWER_RESET);

	/* Enable screen memory.  */
	vga_sr_write(1, vga_sr_read(1) & ~0x20);

	/* Clear interrupts. */
	write32(mmio + DEIIR, 0xffffffff);
	write32(mmio + SDEIIR, 0xffffffff);

	if (IS_ENABLED(CONFIG_LINEAR_FRAMEBUFFER)) {
		memset ((void *) lfb, 0, edid.x_resolution
					* edid.y_resolution * 4);
		set_vbe_mode_info_valid(&edid, lfb);
	}

	/* Linux relies on VBT for panel info.  */
	generate_fake_intel_oprom(info, dev_find_slot(0, PCI_DEVFN(2, 0)),
				  "$VBT SNB/IVB-MOBILE");

	return 1;
}
