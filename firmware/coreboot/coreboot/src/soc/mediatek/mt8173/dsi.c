/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 MediaTek Inc.
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
#include <console/console.h>
#include <delay.h>
#include <soc/addressmap.h>
#include <soc/i2c.h>
#include <soc/gpio.h>
#include <soc/dsi.h>
#include <soc/ddp.h>
#include <timer.h>

static bool dual_dsi_mode;

static void mipi_write32(void *a, uint32_t v)
{
	void *a1 = a + (MIPI_TX1_BASE - MIPI_TX0_BASE);
	write32(a, v);
	if (dual_dsi_mode)
		write32(a1, v);
}

static void mipi_clrsetbits_le32(void *a, uint32_t m, uint32_t v)
{
	void *a1 = a + (MIPI_TX1_BASE - MIPI_TX0_BASE);
	clrsetbits_le32(a, m, v);
	if (dual_dsi_mode)
		clrsetbits_le32(a1, m, v);
}

static void mipi_clrbits_le32(void *a, uint32_t m)
{
	void *a1 = a + (MIPI_TX1_BASE - MIPI_TX0_BASE);
	clrbits_le32(a, m);
	if (dual_dsi_mode)
		clrbits_le32(a1, m);
}

static void mipi_setbits_le32(void *a, uint32_t m)
{
	void *a1 = a + (MIPI_TX1_BASE - MIPI_TX0_BASE);
	setbits_le32(a, m);
	if (dual_dsi_mode)
		setbits_le32(a1, m);
}

static void dsi_write32(void *a, uint32_t v)
{
	void *a1 = a + (DSI1_BASE - DSI0_BASE);
	write32(a, v);
	if (dual_dsi_mode)
		write32(a1, v);
}

static void dsi_clrsetbits_le32(void *a, uint32_t m, uint32_t v)
{
	void *a1 = a + (DSI1_BASE - DSI0_BASE);
	clrsetbits_le32(a, m, v);
	if (dual_dsi_mode)
		clrsetbits_le32(a1, m, v);
}

static void dsi_clrbits_le32(void *a, uint32_t m)
{
	void *a1 = a + (DSI1_BASE - DSI0_BASE);
	clrbits_le32(a, m);
	if (dual_dsi_mode)
		clrbits_le32(a1, m);
}

static void dsi_setbits_le32(void *a, uint32_t m)
{
	void *a1 = a + (DSI1_BASE - DSI0_BASE);
	setbits_le32(a, m);
	if (dual_dsi_mode)
		setbits_le32(a1, m);
}

static int mtk_dsi_phy_clk_setting(u32 format, u32 lanes,
				   const struct edid *edid)
{
	u32 txdiv0, txdiv1;
	u64 pcw;
	u32 reg;
	u32 bit_per_pixel;
	int i, data_rate, mipi_tx_rate;

	reg = read32(&mipi_tx0->dsi_bg_con);

	reg = (reg & (~RG_DSI_V02_SEL)) | (4 << 20);
	reg = (reg & (~RG_DSI_V032_SEL)) | (4 << 17);
	reg = (reg & (~RG_DSI_V04_SEL)) | (4 << 14);
	reg = (reg & (~RG_DSI_V072_SEL)) | (4 << 11);
	reg = (reg & (~RG_DSI_V10_SEL)) | (4 << 8);
	reg = (reg & (~RG_DSI_V12_SEL)) | (4 << 5);
	reg |= RG_DSI_BG_CKEN;
	reg |= RG_DSI_BG_CORE_EN;
	mipi_write32(&mipi_tx0->dsi_bg_con, reg);
	udelay(30);

	mipi_clrsetbits_le32(&mipi_tx0->dsi_top_con, RG_DSI_LNT_IMP_CAL_CODE,
			8 << 4 | RG_DSI_LNT_HS_BIAS_EN);

	mipi_setbits_le32(&mipi_tx0->dsi_con,
		     RG_DSI0_CKG_LDOOUT_EN | RG_DSI0_LDOCORE_EN);

	mipi_clrsetbits_le32(&mipi_tx0->dsi_pll_pwr, RG_DSI_MPPLL_SDM_ISO_EN,
			RG_DSI_MPPLL_SDM_PWR_ON);

	mipi_clrbits_le32(&mipi_tx0->dsi_pll_con0, RG_DSI0_MPPLL_PLL_EN);

	switch (format) {
	case MIPI_DSI_FMT_RGB565:
		bit_per_pixel = 16;
		break;
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
		bit_per_pixel = 18;
		break;
	case MIPI_DSI_FMT_RGB888:
	default:
		bit_per_pixel = 24;
		break;
	}

	/**
	 * data_rate = (pixel_clock / 1000) * bit_per_pixel * mipi_ratio / lane_num
	 * pixel_clock unit is Khz, data_rata unit is MHz, so need divide 1000.
	 * mipi_ratio is mipi clk coefficient for balance the pixel clk in mipi.
	 * we set mipi_ratio is 1.02.
	 * lane_num
	 */
	data_rate = edid->mode.pixel_clock * 102 * bit_per_pixel /
			 (lanes * 1000 * 100);
	mipi_tx_rate = data_rate;
	if (dual_dsi_mode)
		data_rate /= 2;

	if (data_rate > 500) {
		txdiv0 = 0;
		txdiv1 = 0;
	} else if (data_rate >= 250) {
		txdiv0 = 1;
		txdiv1 = 0;
	} else if (data_rate >= 125) {
		txdiv0 = 2;
		txdiv1 = 0;
	} else if (data_rate >= 62) {
		txdiv0 = 2;
		txdiv1 = 1;
	} else if (data_rate >= 50) {
		txdiv0 = 2;
		txdiv1 = 2;
	} else {
		printk(BIOS_ERR, "data rate (%u) must be >=50. Please check "
		       "pixel clock (%u), bpp (%u), and number of lanes (%u)\n",
		       data_rate, edid->mode.pixel_clock, bit_per_pixel, lanes);
		return -1;
	}

	mipi_clrsetbits_le32(&mipi_tx0->dsi_pll_con0,
			RG_DSI0_MPPLL_TXDIV1 | RG_DSI0_MPPLL_TXDIV0 |
			RG_DSI0_MPPLL_PREDIV, txdiv1 << 5 | txdiv0 << 3);

	/**
	 * PLL PCW config
	 * PCW bit 24~30 = integer part of pcw
	 * PCW bit 0~23 = fractional part of pcw
	 * pcw = data_Rate*4*txdiv/(Ref_clk*2);
	 * Post DIV =4, so need data_Rate*4
	 * Ref_clk is 26MHz
	 */
	pcw = (u64)(data_rate * (1 << txdiv0) * (1 << txdiv1)) << 24;
	pcw /= 13;
	mipi_write32(&mipi_tx0->dsi_pll_con2, pcw);

	mipi_setbits_le32(&mipi_tx0->dsi_pll_con1, RG_DSI0_MPPLL_SDM_FRA_EN);

	mipi_setbits_le32(&mipi_tx0->dsi_clock_lane, LDOOUT_EN);

	for (i = 0; i < lanes; i++)
		mipi_setbits_le32(&mipi_tx0->dsi_data_lane[i], LDOOUT_EN);

	mipi_setbits_le32(&mipi_tx0->dsi_pll_con0, RG_DSI0_MPPLL_PLL_EN);

	udelay(40);

	mipi_clrbits_le32(&mipi_tx0->dsi_pll_con1, RG_DSI0_MPPLL_SDM_SSC_EN);
	mipi_clrbits_le32(&mipi_tx0->dsi_top_con, RG_DSI_PAD_TIE_LOW_EN);

	return mipi_tx_rate;
}

static void mtk_dsi_phy_timconfig(u32 data_rate)
{
	u32 timcon0, timcon1, timcon2, timcon3;
	u32 cycle_time, ui, lpx;

	ui = 1000 / data_rate + 0x01;
	cycle_time = 8000 / data_rate + 0x01;
	lpx = 5;

	timcon0 = (8 << 24) | (0xa << 16) | (0x6 << 8) | lpx;
	timcon1 = (7 << 24) | (5 * lpx << 16) | ((3 * lpx) / 2) << 8 |
		  (4 * lpx);
	timcon2 = ((div_round_up(0x64, cycle_time) + 0xa) << 24) |
		  (div_round_up(0x150, cycle_time) << 16);
	timcon3 = (2 * lpx) << 16 |
		  div_round_up(80 + 52 * ui, cycle_time) << 8 |
		  div_round_up(0x40, cycle_time);

	dsi_write32(&dsi0->dsi_phy_timecon0, timcon0);
	dsi_write32(&dsi0->dsi_phy_timecon1, timcon1);
	dsi_write32(&dsi0->dsi_phy_timecon2, timcon2);
	dsi_write32(&dsi0->dsi_phy_timecon3, timcon3);
}

static void mtk_dsi_reset(void)
{
	dsi_setbits_le32(&dsi0->dsi_con_ctrl, 3);
	dsi_clrbits_le32(&dsi0->dsi_con_ctrl, 1);
}

static void mtk_dsi_clk_hs_mode_enable(void)
{
	dsi_setbits_le32(&dsi0->dsi_phy_lccon, LC_HS_TX_EN);
}

static void mtk_dsi_clk_hs_mode_disable(void)
{
	dsi_clrbits_le32(&dsi0->dsi_phy_lccon, LC_HS_TX_EN);
}

static void mtk_dsi_set_mode(u32 mode_flags)
{
	u32 tmp_reg1 = 0;

	if (mode_flags & MIPI_DSI_MODE_VIDEO) {
		tmp_reg1 = SYNC_PULSE_MODE;

		if (mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
			tmp_reg1 = BURST_MODE;

		if (mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			tmp_reg1 = SYNC_PULSE_MODE;
	}

	dsi_write32(&dsi0->dsi_mode_ctrl, tmp_reg1);
}

static void mtk_dsi_rxtx_control(u32 mode_flags, u32 lanes)
{
	u32 tmp_reg = 0;

	switch (lanes) {
	case 1:
		tmp_reg = 1 << 2;
		break;
	case 2:
		tmp_reg = 3 << 2;
		break;
	case 3:
		tmp_reg = 7 << 2;
		break;
	case 4:
	default:
		tmp_reg = 0xf << 2;
		break;
	}

	tmp_reg |= (mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) << 6;
	tmp_reg |= (mode_flags & MIPI_DSI_MODE_EOT_PACKET) >> 3;

	dsi_write32(&dsi0->dsi_txrx_ctrl, tmp_reg);
}

static void mtk_dsi_config_vdo_timing(u32 mode_flags, u32 format,
				      const struct edid *edid)
{
	u32 hsync_active_byte;
	u32 hbp_byte;
	u32 hfp_byte;
	u32 vbp_byte;
	u32 vfp_byte;
	u32 bpp;
	u32 packet_fmt;
	u32 hactive;

	if (format == MIPI_DSI_FMT_RGB565)
		bpp = 2;
	else
		bpp = 3;

	vbp_byte = edid->mode.vbl - edid->mode.vso - edid->mode.vspw -
		   edid->mode.vborder;
	vfp_byte = edid->mode.vso - edid->mode.vborder;

	dsi_write32(&dsi0->dsi_vsa_nl, edid->mode.vspw);
	dsi_write32(&dsi0->dsi_vbp_nl, vbp_byte);
	dsi_write32(&dsi0->dsi_vfp_nl, vfp_byte);
	dsi_write32(&dsi0->dsi_vact_nl, edid->mode.va);

	if (mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		hbp_byte = (edid->mode.hbl - edid->mode.hso - edid->mode.hspw -
			    edid->mode.hborder) * bpp - 10;
	else
		hbp_byte = (edid->mode.hbl - edid->mode.hso -
			    edid->mode.hborder) * bpp - 10;

	hsync_active_byte = edid->mode.hspw * bpp - 10;
	hfp_byte = (edid->mode.hso - edid->mode.hborder) * bpp - 12;

	dsi_write32(&dsi0->dsi_hsa_wc, hsync_active_byte);
	dsi_write32(&dsi0->dsi_hbp_wc, hbp_byte);
	dsi_write32(&dsi0->dsi_hfp_wc, hfp_byte);

	switch (format) {
	case MIPI_DSI_FMT_RGB888:
		packet_fmt = PACKED_PS_24BIT_RGB888;
		break;
	case MIPI_DSI_FMT_RGB666:
		packet_fmt = LOOSELY_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		packet_fmt = PACKED_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB565:
		packet_fmt = PACKED_PS_16BIT_RGB565;
		break;
	default:
		packet_fmt = PACKED_PS_24BIT_RGB888;
		break;
	}

	hactive = edid->mode.ha;
	if (dual_dsi_mode)
		hactive /= 2;
	packet_fmt |= (hactive * bpp) & DSI_PS_WC;

	dsi_write32(&dsi0->dsi_psctrl, packet_fmt);
}

static void mtk_dsi_start(void)
{
	dsi_write32(&dsi0->dsi_start, 0);
	/* Only start master DSI */
	write32(&dsi0->dsi_start, 1);
}

static void mtk_dsi_tx_cmd_type0(u8 cmd)
{
	struct stopwatch sw;
	u32 cmdq0;
	u32 intsta_0, intsta_1;

	cmdq0 = (MIPI_DSI_DCS_SHORT_WRITE << 8) | SHORT_PACKET | (cmd << 16);

	dsi_write32(&dsi0->dsi_cmdq0, cmdq0);
	dsi_clrsetbits_le32(&dsi0->dsi_cmdq_size, CMDQ_SIZE, 1);
	dsi_write32(&dsi0->dsi_intsta, 0);

	dsi_write32(&dsi0->dsi_start, 1);

	stopwatch_init_usecs_expire(&sw, 400);
	do {
		intsta_0 = read32(&dsi0->dsi_intsta);
		intsta_1 = read32(&dsi1->dsi_intsta);
		if ((intsta_0 & CMD_DONE_INT_FLAG) &&
		    (intsta_1 & CMD_DONE_INT_FLAG))
			break;
		udelay(4);
	} while (!stopwatch_expired(&sw));

	if (!(intsta_0 & CMD_DONE_INT_FLAG))
		printk(BIOS_ERR, "DSI0 DONE INT Timeout\n");

	if (!(intsta_1 & CMD_DONE_INT_FLAG))
		printk(BIOS_ERR, "DSI1 DONE INT Timeout\n");

	dsi_write32(&dsi0->dsi_start, 0);
}

int mtk_dsi_init(u32 mode_flags, u32 format, u32 lanes, bool dual,
		 const struct edid *edid)
{
	int data_rate;

	dual_dsi_mode = dual;

	data_rate = mtk_dsi_phy_clk_setting(format, lanes, edid);

	if (data_rate < 0)
		return -1;

	mtk_dsi_reset();
	mtk_dsi_phy_timconfig(data_rate);
	mtk_dsi_rxtx_control(mode_flags, lanes);
	mtk_dsi_clk_hs_mode_disable();
	mtk_dsi_config_vdo_timing(mode_flags, format, edid);
	mtk_dsi_set_mode(mode_flags);
	mtk_dsi_clk_hs_mode_enable();

	if (dual_dsi_mode) {
		dsi_write32(&dsi0->dsi_start, 0);
		/* Disable dual_dsi when in CMD_MODE */
		dsi_write32(&dsi0->dsi_con_ctrl, DSI_EN);

		dsi_write32(&dsi0->dsi_mode_ctrl, CMD_MODE);

		mtk_dsi_tx_cmd_type0(MIPI_DCS_EXIT_SLEEP_MODE);
		mtk_dsi_tx_cmd_type0(MIPI_DCS_SET_DISPLAY_ON);

		dsi_write32(&dsi0->dsi_con_ctrl, DSI_EN | DSI_DUAL);

		dsi_write32(&dsi0->dsi_mode_ctrl, BURST_MODE);
	}

	mtk_dsi_start();

	return 0;
}

void mtk_dsi_pin_drv_ctrl(void)
{
	struct stopwatch sw;
	uint32_t pwr_ack;

	mipi_setbits_le32(&lvds_tx1->vopll_ctl3, RG_DA_LVDSTX_PWR_ON);

	stopwatch_init_usecs_expire(&sw, 1000);

	do {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR, "enable lvdstx_power failed!!!\n");
			return;
		}
		pwr_ack = read32(&lvds_tx1->vopll_ctl3) & RG_AD_LVDSTX_PWR_ACK;
		if (dual_dsi_mode)
			pwr_ack &= read32(&lvds_tx2->vopll_ctl3) &
					RG_AD_LVDSTX_PWR_ACK;
	} while (pwr_ack == 0);

	mipi_clrbits_le32(&lvds_tx1->vopll_ctl3, RG_DA_LVDS_ISO_EN);
}
