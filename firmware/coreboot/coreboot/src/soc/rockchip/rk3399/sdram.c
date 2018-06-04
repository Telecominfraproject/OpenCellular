/*
 * This file is part of the coreboot project.
 *
 * Copyright 2016 Rockchip Inc.
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
#include <reset.h>
#include <soc/addressmap.h>
#include <soc/clock.h>
#include <soc/sdram.h>
#include <soc/grf.h>
#include <soc/soc.h>
#include <string.h>
#include <timer.h>
#include <types.h>

#define DDR_PI_OFFSET			0x800
#define DDR_PHY_OFFSET			0x2000
#define DDRC0_PI_BASE_ADDR		(DDRC0_BASE_ADDR + DDR_PI_OFFSET)
#define DDRC0_PHY_BASE_ADDR		(DDRC0_BASE_ADDR + DDR_PHY_OFFSET)
#define DDRC1_PI_BASE_ADDR		(DDRC1_BASE_ADDR + DDR_PI_OFFSET)
#define DDRC1_PHY_BASE_ADDR		(DDRC1_BASE_ADDR + DDR_PHY_OFFSET)

static struct rk3399_ddr_pctl_regs * const rk3399_ddr_pctl[2] = {
	(void *)DDRC0_BASE_ADDR, (void *)DDRC1_BASE_ADDR };
static struct rk3399_ddr_pi_regs * const rk3399_ddr_pi[2] = {
	(void *)DDRC0_PI_BASE_ADDR, (void *)DDRC1_PI_BASE_ADDR };
static struct rk3399_ddr_publ_regs * const rk3399_ddr_publ[2] = {
	(void *)DDRC0_PHY_BASE_ADDR, (void *)DDRC1_PHY_BASE_ADDR };
static struct rk3399_msch_regs * const rk3399_msch[2] = {
	(void *)SERVER_MSCH0_BASE_ADDR, (void *)SERVER_MSCH1_BASE_ADDR };
static struct rk3399_ddr_cic_regs *const rk3399_ddr_cic = (void *)CIC_BASE_ADDR;

/*
 * sys_reg bitfield struct
 * [31]		row_3_4_ch1
 * [30]		row_3_4_ch0
 * [29:28]	chinfo
 * [27]		rank_ch1
 * [26:25]	col_ch1
 * [24]		bk_ch1
 * [23:22]	cs0_row_ch1
 * [21:20]	cs1_row_ch1
 * [19:18]	bw_ch1
 * [17:16]	dbw_ch1;
 * [15:13]	ddrtype
 * [12]		channelnum
 * [11]		rank_ch0
 * [10:9]	col_ch0
 * [8]		bk_ch0
 * [7:6]	cs0_row_ch0
 * [5:4]	cs1_row_ch0
 * [3:2]	bw_ch0
 * [1:0]	dbw_ch0
*/
#define SYS_REG_ENC_ROW_3_4(n, ch)	((n) << (30 + (ch)))
#define SYS_REG_DEC_ROW_3_4(n, ch)	((n >> (30 + ch)) & 0x1)
#define SYS_REG_ENC_CHINFO(ch)		(1 << (28 + (ch)))
#define SYS_REG_ENC_DDRTYPE(n)		((n) << 13)
#define SYS_REG_ENC_NUM_CH(n)		(((n) - 1) << 12)
#define SYS_REG_DEC_NUM_CH(n)		(1 + ((n >> 12) & 0x1))
#define SYS_REG_ENC_RANK(n, ch)		(((n) - 1) << (11 + ((ch) * 16)))
#define SYS_REG_DEC_RANK(n, ch)		(1 + ((n >> (11 + 16 * ch)) & 0x1))
#define SYS_REG_ENC_COL(n, ch)		(((n) - 9) << (9 + ((ch) * 16)))
#define SYS_REG_DEC_COL(n, ch)		(9 + ((n >> (9 + 16 * ch)) & 0x3))
#define SYS_REG_ENC_BK(n, ch)		(((n) == 3 ? 0 : 1) \
						<< (8 + ((ch) * 16)))
#define SYS_REG_DEC_BK(n, ch)		(3 - ((n >> (8 + 16 * ch)) & 0x1))
#define SYS_REG_ENC_CS0_ROW(n, ch)	(((n) - 13) << (6 + ((ch) * 16)))
#define SYS_REG_DEC_CS0_ROW(n, ch)	(13 + ((n >> (6 + 16 * ch)) & 0x3))
#define SYS_REG_ENC_CS1_ROW(n, ch)	(((n) - 13) << (4 + ((ch) * 16)))
#define SYS_REG_DEC_CS1_ROW(n, ch)	(13 + ((n >> (4 + 16 * ch)) & 0x3))
#define SYS_REG_ENC_BW(n, ch)		((2 >> (n)) << (2 + ((ch) * 16)))
#define SYS_REG_DEC_BW(n, ch)		(2 >> ((n >> (2 + 16 * ch)) & 0x3))
#define SYS_REG_ENC_DBW(n, ch)		((2 >> (n)) << (0 + ((ch) * 16)))
#define SYS_REG_DEC_DBW(n, ch)		(2 >> ((n >> (0 + 16 * ch)) & 0x3))

#define DDR_STRIDE(n)		write32(&rk3399_pmusgrf->soc_con4,\
					(0x1F << (10 + 16)) | (n << 10))

#define PRESET_SGRF_HOLD(n)	((0x1 << (6+16)) | ((n) << 6))
#define PRESET_GPIO0_HOLD(n)	((0x1 << (7+16)) | ((n) << 7))
#define PRESET_GPIO1_HOLD(n)	((0x1 << (8+16)) | ((n) << 8))

#define PHY_DRV_ODT_Hi_Z	(0x0)
#define PHY_DRV_ODT_240		(0x1)
#define PHY_DRV_ODT_120		(0x8)
#define PHY_DRV_ODT_80		(0x9)
#define PHY_DRV_ODT_60		(0xc)
#define PHY_DRV_ODT_48		(0xd)
#define PHY_DRV_ODT_40		(0xe)
#define PHY_DRV_ODT_34_3	(0xf)

static void copy_to_reg(u32 *dest, const u32 *src, u32 n)
{
	int i;

	for (i = 0; i < n / sizeof(u32); i++) {
		write32(dest, *src);
		src++;
		dest++;
	}
}

static void phy_dll_bypass_set(u32 channel,
	struct rk3399_ddr_publ_regs *ddr_publ_regs, u32 freq)
{
	u32 *denali_phy = ddr_publ_regs->denali_phy;

	if (freq <= 125*MHz) {
		/* phy_sw_master_mode_X PHY_86/214/342/470 4bits offset_8 */
		setbits_le32(&denali_phy[86], (0x3 << 2) << 8);
		setbits_le32(&denali_phy[214], (0x3 << 2) << 8);
		setbits_le32(&denali_phy[342], (0x3 << 2) << 8);
		setbits_le32(&denali_phy[470], (0x3 << 2) << 8);

		/* phy_adrctl_sw_master_mode PHY_547/675/803 4bits offset_16 */
		setbits_le32(&denali_phy[547], (0x3 << 2) << 16);
		setbits_le32(&denali_phy[675], (0x3 << 2) << 16);
		setbits_le32(&denali_phy[803], (0x3 << 2) << 16);
	} else {
		/* phy_sw_master_mode_X PHY_86/214/342/470 4bits offset_8 */
		clrbits_le32(&denali_phy[86], (0x3 << 2) << 8);
		clrbits_le32(&denali_phy[214], (0x3 << 2) << 8);
		clrbits_le32(&denali_phy[342], (0x3 << 2) << 8);
		clrbits_le32(&denali_phy[470], (0x3 << 2) << 8);

		/* phy_adrctl_sw_master_mode PHY_547/675/803 4bits offset_16 */
		clrbits_le32(&denali_phy[547], (0x3 << 2) << 16);
		clrbits_le32(&denali_phy[675], (0x3 << 2) << 16);
		clrbits_le32(&denali_phy[803], (0x3 << 2) << 16);
	}
}

static void set_memory_map(u32 channel,
			   const struct rk3399_sdram_params *sdram_params)
{
	const struct rk3399_sdram_channel *sdram_ch =
		&sdram_params->ch[channel];
	u32 *denali_ctl = rk3399_ddr_pctl[channel]->denali_ctl;
	u32 *denali_pi = rk3399_ddr_pi[channel]->denali_pi;
	u32 cs_map;
	u32 reduc;
	u32 row;

	if ((sdram_ch->ddrconfig < 2) || (sdram_ch->ddrconfig == 4))
		row = 16;
	else if (sdram_ch->ddrconfig == 3)
		row = 14;
	else
		row = 15;

	cs_map = (sdram_ch->rank > 1) ? 3 : 1;
	reduc = (sdram_ch->bw == 2) ? 0 : 1;

	clrsetbits_le32(&denali_ctl[191], 0xF, (12 - sdram_ch->col));
	clrsetbits_le32(&denali_ctl[190], (0x3 << 16) | (0x7 << 24),
			((3 - sdram_ch->bk) << 16) |
			((16 - row) << 24));

	clrsetbits_le32(&denali_ctl[196], 0x3 | (1 << 16),
			cs_map | (reduc << 16));

	/* PI_199 PI_COL_DIFF:RW:0:4 */
	clrsetbits_le32(&denali_pi[199], 0xF, (12 - sdram_ch->col));

	/* PI_155 PI_ROW_DIFF:RW:24:3 PI_BANK_DIFF:RW:16:2 */
	clrsetbits_le32(&denali_pi[155], (0x3 << 16) | (0x7 << 24),
			((3 - sdram_ch->bk) << 16) |
			((16 - row) << 24));
	/* PI_41 PI_CS_MAP:RW:24:4 */
	clrsetbits_le32(&denali_pi[41], 0xf << 24, cs_map << 24);
	if ((sdram_ch->rank == 1) && (sdram_params->dramtype == DDR3))
		write32(&denali_pi[34], 0x2EC7FFFF);
}

static void set_ds_odt(u32 channel,
		       const struct rk3399_sdram_params *sdram_params)
{
	u32 *denali_phy = rk3399_ddr_publ[channel]->denali_phy;

	u32 tsel_idle_en, tsel_wr_en, tsel_rd_en;
	u32 tsel_idle_select_p, tsel_wr_select_p, tsel_rd_select_p;
	u32 ca_tsel_wr_select_p, ca_tsel_wr_select_n;
	u32 tsel_idle_select_n, tsel_wr_select_n, tsel_rd_select_n;
	u32 reg_value;

	if (sdram_params->dramtype == LPDDR4) {
		tsel_rd_select_p = PHY_DRV_ODT_Hi_Z;
		tsel_wr_select_p = PHY_DRV_ODT_40;
		ca_tsel_wr_select_p = PHY_DRV_ODT_40;
		tsel_idle_select_p = PHY_DRV_ODT_Hi_Z;

		tsel_rd_select_n = PHY_DRV_ODT_240;
		tsel_wr_select_n = PHY_DRV_ODT_40;
		ca_tsel_wr_select_n = PHY_DRV_ODT_40;
		tsel_idle_select_n = PHY_DRV_ODT_240;
	} else if (sdram_params->dramtype == LPDDR3) {
		tsel_rd_select_p = PHY_DRV_ODT_240;
		tsel_wr_select_p = PHY_DRV_ODT_34_3;
		ca_tsel_wr_select_p = PHY_DRV_ODT_48;
		tsel_idle_select_p = PHY_DRV_ODT_240;

		tsel_rd_select_n = PHY_DRV_ODT_Hi_Z;
		tsel_wr_select_n = PHY_DRV_ODT_34_3;
		ca_tsel_wr_select_n = PHY_DRV_ODT_48;
		tsel_idle_select_n = PHY_DRV_ODT_Hi_Z;
	} else {
		tsel_rd_select_p = PHY_DRV_ODT_240;
		tsel_wr_select_p = PHY_DRV_ODT_34_3;
		ca_tsel_wr_select_p = PHY_DRV_ODT_34_3;
		tsel_idle_select_p = PHY_DRV_ODT_240;

		tsel_rd_select_n = PHY_DRV_ODT_240;
		tsel_wr_select_n = PHY_DRV_ODT_34_3;
		ca_tsel_wr_select_n = PHY_DRV_ODT_34_3;
		tsel_idle_select_n = PHY_DRV_ODT_240;
	}

	if (sdram_params->odt == 1)
		tsel_rd_en = 1;
	else
		tsel_rd_en = 0;

	tsel_wr_en = 0;
	tsel_idle_en = 0;

	/*
	 * phy_dq_tsel_select_X 24bits DENALI_PHY_6/134/262/390 offset_0
	 * sets termination values for read/idle cycles and drive strength
	 * for write cycles for DQ/DM
	 */
	reg_value = tsel_rd_select_n | (tsel_rd_select_p << 0x4) |
		    (tsel_wr_select_n << 8) | (tsel_wr_select_p << 12) |
		    (tsel_idle_select_n << 16) | (tsel_idle_select_p << 20);
	clrsetbits_le32(&denali_phy[6], 0xffffff, reg_value);
	clrsetbits_le32(&denali_phy[134], 0xffffff, reg_value);
	clrsetbits_le32(&denali_phy[262], 0xffffff, reg_value);
	clrsetbits_le32(&denali_phy[390], 0xffffff, reg_value);

	/*
	 * phy_dqs_tsel_select_X 24bits DENALI_PHY_7/135/263/391 offset_0
	 * sets termination values for read/idle cycles and drive strength
	 * for write cycles for DQS
	 */
	clrsetbits_le32(&denali_phy[7], 0xffffff, reg_value);
	clrsetbits_le32(&denali_phy[135], 0xffffff, reg_value);
	clrsetbits_le32(&denali_phy[263], 0xffffff, reg_value);
	clrsetbits_le32(&denali_phy[391], 0xffffff, reg_value);

	/* phy_adr_tsel_select_ 8bits DENALI_PHY_544/672/800 offset_0 */
	reg_value = ca_tsel_wr_select_n | (ca_tsel_wr_select_p << 0x4);
	clrsetbits_le32(&denali_phy[544], 0xff, reg_value);
	clrsetbits_le32(&denali_phy[672], 0xff, reg_value);
	clrsetbits_le32(&denali_phy[800], 0xff, reg_value);

	/* phy_pad_addr_drive 8bits DENALI_PHY_928 offset_0 */
	clrsetbits_le32(&denali_phy[928], 0xff, reg_value);

	/* phy_pad_rst_drive 8bits DENALI_PHY_937 offset_0 */
	clrsetbits_le32(&denali_phy[937], 0xff, reg_value);

	/* phy_pad_cke_drive 8bits DENALI_PHY_935 offset_0 */
	clrsetbits_le32(&denali_phy[935], 0xff, reg_value);

	/* phy_pad_cs_drive 8bits DENALI_PHY_939 offset_0 */
	clrsetbits_le32(&denali_phy[939], 0xff, reg_value);

	/* phy_pad_clk_drive 8bits DENALI_PHY_929 offset_0 */
	clrsetbits_le32(&denali_phy[929], 0xff, reg_value);

	/* phy_pad_fdbk_drive 23bit DENALI_PHY_924/925 */
	clrsetbits_le32(&denali_phy[924], 0xff,
			tsel_wr_select_n | (tsel_wr_select_p << 4));
	clrsetbits_le32(&denali_phy[925], 0xff,
			tsel_rd_select_n | (tsel_rd_select_p << 4));

	/* phy_dq_tsel_enable_X 3bits DENALI_PHY_5/133/261/389 offset_16 */
	reg_value = (tsel_rd_en | (tsel_wr_en << 1) | (tsel_idle_en << 2))
		<< 16;
	clrsetbits_le32(&denali_phy[5], 0x7 << 16, reg_value);
	clrsetbits_le32(&denali_phy[133], 0x7 << 16, reg_value);
	clrsetbits_le32(&denali_phy[261], 0x7 << 16, reg_value);
	clrsetbits_le32(&denali_phy[389], 0x7 << 16, reg_value);

	/* phy_dqs_tsel_enable_X 3bits DENALI_PHY_6/134/262/390 offset_24 */
	reg_value = (tsel_rd_en | (tsel_wr_en << 1) | (tsel_idle_en << 2))
		<< 24;
	clrsetbits_le32(&denali_phy[6], 0x7 << 24, reg_value);
	clrsetbits_le32(&denali_phy[134], 0x7 << 24, reg_value);
	clrsetbits_le32(&denali_phy[262], 0x7 << 24, reg_value);
	clrsetbits_le32(&denali_phy[390], 0x7 << 24, reg_value);

	/* phy_adr_tsel_enable_ 1bit DENALI_PHY_518/646/774 offset_8 */
	reg_value = tsel_wr_en << 8;
	clrsetbits_le32(&denali_phy[518], 0x1 << 8, reg_value);
	clrsetbits_le32(&denali_phy[646], 0x1 << 8, reg_value);
	clrsetbits_le32(&denali_phy[774], 0x1 << 8, reg_value);

	/* phy_pad_addr_term tsel 1bit DENALI_PHY_933 offset_17 */
	reg_value = tsel_wr_en << 17;
	clrsetbits_le32(&denali_phy[933], 0x1 << 17, reg_value);
	/*
	 * pad_rst/cke/cs/clk_term tsel 1bits
	 * DENALI_PHY_938/936/940/934 offset_17
	 */
	clrsetbits_le32(&denali_phy[938], 0x1 << 17, reg_value);
	clrsetbits_le32(&denali_phy[936], 0x1 << 17, reg_value);
	clrsetbits_le32(&denali_phy[940], 0x1 << 17, reg_value);
	clrsetbits_le32(&denali_phy[934], 0x1 << 17, reg_value);

	/* phy_pad_fdbk_term 1bit DENALI_PHY_930 offset_17 */
	clrsetbits_le32(&denali_phy[930], 0x1 << 17, reg_value);
}

static void phy_io_config(u32 channel,
			  const struct rk3399_sdram_params *sdram_params)
{
	u32 *denali_phy = rk3399_ddr_publ[channel]->denali_phy;
	u32 vref_mode_dq, vref_value_dq, vref_mode_ac, vref_value_ac;
	u32 mode_sel = 0;
	u32 reg_value;
	u32 drv_value, odt_value;
	u32 speed;

	/* vref setting */
	if (sdram_params->dramtype == LPDDR4) {
		/* LPDDR4 */
		vref_mode_dq = 0x6;
		vref_value_dq = 0x1f;
		vref_mode_ac = 0x6;
		vref_value_ac = 0x1f;
	} else if (sdram_params->dramtype == LPDDR3) {
		if (sdram_params->odt == 1) {
			vref_mode_dq = 0x5;  /* LPDDR3 ODT */
			drv_value = (read32(&denali_phy[6]) >> 12) & 0xf;
			odt_value = (read32(&denali_phy[6]) >> 4) & 0xf;
			if (drv_value == PHY_DRV_ODT_48) {
				switch (odt_value) {
				case PHY_DRV_ODT_240:
					vref_value_dq = 0x16;
					break;
				case PHY_DRV_ODT_120:
					vref_value_dq = 0x26;
					break;
				case PHY_DRV_ODT_60:
					vref_value_dq = 0x36;
					break;
				default:
					die("Halting: Invalid ODT value.\n");
				}
			} else if (drv_value == PHY_DRV_ODT_40) {
				switch (odt_value) {
				case PHY_DRV_ODT_240:
					vref_value_dq = 0x19;
					break;
				case PHY_DRV_ODT_120:
					vref_value_dq = 0x23;
					break;
				case PHY_DRV_ODT_60:
					vref_value_dq = 0x31;
					break;
				default:
					die("Halting: Invalid ODT value.\n");
				}
			} else if (drv_value == PHY_DRV_ODT_34_3) {
				switch (odt_value) {
				case PHY_DRV_ODT_240:
					vref_value_dq = 0x17;
					break;
				case PHY_DRV_ODT_120:
					vref_value_dq = 0x20;
					break;
				case PHY_DRV_ODT_60:
					vref_value_dq = 0x2e;
					break;
				default:
					die("Halting: Invalid ODT value.\n");
				}
			} else {
				die("Halting: Invalid DRV value.\n");
			}
		} else {
			vref_mode_dq = 0x2;  /* LPDDR3 */
			vref_value_dq = 0x1f;
		}
		vref_mode_ac = 0x2;
		vref_value_ac = 0x1f;
	} else if (sdram_params->dramtype == DDR3) {
		/* DDR3L */
		vref_mode_dq = 0x1;
		vref_value_dq = 0x1f;
		vref_mode_ac = 0x1;
		vref_value_ac = 0x1f;
	}
	else
		die("Halting: Unknown DRAM type.\n");

	reg_value = (vref_mode_dq << 9) | (0x1 << 8) | vref_value_dq;

	/* PHY_913 PHY_PAD_VREF_CTRL_DQ_0 12bits offset_8 */
	clrsetbits_le32(&denali_phy[913], 0xfff << 8, reg_value << 8);
	/* PHY_914 PHY_PAD_VREF_CTRL_DQ_1 12bits offset_0 */
	clrsetbits_le32(&denali_phy[914], 0xfff, reg_value);
	/* PHY_914 PHY_PAD_VREF_CTRL_DQ_2 12bits offset_16 */
	clrsetbits_le32(&denali_phy[914], 0xfff << 16, reg_value << 16);
	/* PHY_915 PHY_PAD_VREF_CTRL_DQ_3 12bits offset_0 */
	clrsetbits_le32(&denali_phy[915], 0xfff, reg_value);

	reg_value = (vref_mode_ac << 9) | (0x1 << 8) | vref_value_ac;

	/* PHY_915 PHY_PAD_VREF_CTRL_AC 12bits offset_16 */
	clrsetbits_le32(&denali_phy[915], 0xfff << 16, reg_value << 16);

	if (sdram_params->dramtype == LPDDR4)
		mode_sel = 0x6;
	else if (sdram_params->dramtype == LPDDR3)
		mode_sel = 0x0;
	else if (sdram_params->dramtype == DDR3)
		mode_sel = 0x1;

	/* PHY_924 PHY_PAD_FDBK_DRIVE */
	clrsetbits_le32(&denali_phy[924], 0x7 << 15, mode_sel << 15);
	/* PHY_926 PHY_PAD_DATA_DRIVE */
	clrsetbits_le32(&denali_phy[926], 0x7 << 6, mode_sel << 6);
	/* PHY_927 PHY_PAD_DQS_DRIVE */
	clrsetbits_le32(&denali_phy[927], 0x7 << 6, mode_sel << 6);
	/* PHY_928 PHY_PAD_ADDR_DRIVE */
	clrsetbits_le32(&denali_phy[928], 0x7 << 14, mode_sel << 14);
	/* PHY_929 PHY_PAD_CLK_DRIVE */
	clrsetbits_le32(&denali_phy[929], 0x7 << 14, mode_sel << 14);
	/* PHY_935 PHY_PAD_CKE_DRIVE */
	clrsetbits_le32(&denali_phy[935], 0x7 << 14, mode_sel << 14);
	/* PHY_937 PHY_PAD_RST_DRIVE */
	clrsetbits_le32(&denali_phy[937], 0x7 << 14, mode_sel << 14);
	/* PHY_939 PHY_PAD_CS_DRIVE */
	clrsetbits_le32(&denali_phy[939], 0x7 << 14, mode_sel << 14);


	/* speed setting */
	if (sdram_params->ddr_freq < 400 * MHz)
		speed = 0x0;
	else if (sdram_params->ddr_freq < 800 * MHz)
		speed = 0x1;
	else if (sdram_params->ddr_freq < 1200 * MHz)
		speed = 0x2;
	else
		speed = 0x3;

	/* PHY_924 PHY_PAD_FDBK_DRIVE */
	clrsetbits_le32(&denali_phy[924], 0x3 << 21, speed << 21);
	/* PHY_926 PHY_PAD_DATA_DRIVE */
	clrsetbits_le32(&denali_phy[926], 0x3 << 9, speed << 9);
	/* PHY_927 PHY_PAD_DQS_DRIVE */
	clrsetbits_le32(&denali_phy[927], 0x3 << 9, speed << 9);
	/* PHY_928 PHY_PAD_ADDR_DRIVE */
	clrsetbits_le32(&denali_phy[928], 0x3 << 17, speed << 17);
	/* PHY_929 PHY_PAD_CLK_DRIVE */
	clrsetbits_le32(&denali_phy[929], 0x3 << 17, speed << 17);
	/* PHY_935 PHY_PAD_CKE_DRIVE */
	clrsetbits_le32(&denali_phy[935], 0x3 << 17, speed << 17);
	/* PHY_937 PHY_PAD_RST_DRIVE */
	clrsetbits_le32(&denali_phy[937], 0x3 << 17, speed << 17);
	/* PHY_939 PHY_PAD_CS_DRIVE */
	clrsetbits_le32(&denali_phy[939], 0x3 << 17, speed << 17);
}

static int pctl_cfg(u32 channel,
		    const struct rk3399_sdram_params *sdram_params)
{
	u32 *denali_ctl = rk3399_ddr_pctl[channel]->denali_ctl;
	u32 *denali_pi = rk3399_ddr_pi[channel]->denali_pi;
	u32 *denali_phy = rk3399_ddr_publ[channel]->denali_phy;
	const u32 *params_ctl = sdram_params->pctl_regs.denali_ctl;
	const u32 *params_phy = sdram_params->phy_regs.denali_phy;
	u32 tmp, tmp1, tmp2;
	u32 pwrup_srefresh_exit;
	struct stopwatch sw;

	/*
	 * work around controller bug:
	 * Do not program DRAM_CLASS until NO_PHY_IND_TRAIN_INT is programmed
	 */
	copy_to_reg(&denali_ctl[1], &params_ctl[1],
		    sizeof(struct rk3399_ddr_pctl_regs) - 4);
	write32(&denali_ctl[0], params_ctl[0]);
	copy_to_reg(denali_pi, &sdram_params->pi_regs.denali_pi[0],
		    sizeof(struct rk3399_ddr_pi_regs));
	/* rank count need to set for init */
	set_memory_map(channel, sdram_params);

	write32(&denali_phy[910], sdram_params->phy_regs.denali_phy[910]);
	write32(&denali_phy[911], sdram_params->phy_regs.denali_phy[911]);
	write32(&denali_phy[912], sdram_params->phy_regs.denali_phy[912]);

	pwrup_srefresh_exit = read32(&denali_ctl[68]) & PWRUP_SREFRESH_EXIT;
	clrbits_le32(&denali_ctl[68], PWRUP_SREFRESH_EXIT);

	/* PHY_DLL_RST_EN */
	clrsetbits_le32(&denali_phy[957], 0x3 << 24, 1 << 24);

	setbits_le32(&denali_pi[0], START);
	setbits_le32(&denali_ctl[0], START);

	while (1) {
		tmp = read32(&denali_phy[920]);
		tmp1 = read32(&denali_phy[921]);
		tmp2 = read32(&denali_phy[922]);
		if ((((tmp >> 16) & 0x1) == 0x1) &&
		    (((tmp1 >> 16) & 0x1) == 0x1) &&
		    (((tmp1 >> 0) & 0x1) == 0x1) &&
		    (((tmp2 >> 0) & 0x1) == 0x1))
			break;
	}

	copy_to_reg(&denali_phy[896], &params_phy[896], (958 - 895) * 4);
	copy_to_reg(&denali_phy[0], &params_phy[0], (90 - 0 + 1) * 4);
	copy_to_reg(&denali_phy[128], &params_phy[128], (218 - 128 + 1) * 4);
	copy_to_reg(&denali_phy[256], &params_phy[256], (346 - 256 + 1) * 4);
	copy_to_reg(&denali_phy[384], &params_phy[384], (474 - 384 + 1) * 4);
	copy_to_reg(&denali_phy[512], &params_phy[512], (549 - 512 + 1) * 4);
	copy_to_reg(&denali_phy[640], &params_phy[640], (677 - 640 + 1) * 4);
	copy_to_reg(&denali_phy[768], &params_phy[768], (805 - 768 + 1) * 4);
	set_ds_odt(channel, sdram_params);

	/*
	 * phy_dqs_tsel_wr_timing_X 8bits DENALI_PHY_84/212/340/468 offset_8
	 * dqs_tsel_wr_end[7:4] add Half cycle
	 */
	tmp = (read32(&denali_phy[84]) >> 8) & 0xff;
	clrsetbits_le32(&denali_phy[84], 0xff << 8, (tmp + 0x10) << 8);
	tmp = (read32(&denali_phy[212]) >> 8) & 0xff;
	clrsetbits_le32(&denali_phy[212], 0xff << 8, (tmp + 0x10) << 8);
	tmp = (read32(&denali_phy[340]) >> 8) & 0xff;
	clrsetbits_le32(&denali_phy[340], 0xff << 8, (tmp + 0x10) << 8);
	tmp = (read32(&denali_phy[468]) >> 8) & 0xff;
	clrsetbits_le32(&denali_phy[468], 0xff << 8, (tmp + 0x10) << 8);

	/*
	 * phy_dqs_tsel_wr_timing_X 8bits DENALI_PHY_83/211/339/467 offset_8
	 * dq_tsel_wr_end[7:4] add Half cycle
	 */
	tmp = (read32(&denali_phy[83]) >> 16) & 0xff;
	clrsetbits_le32(&denali_phy[83], 0xff << 16, (tmp + 0x10) << 16);
	tmp = (read32(&denali_phy[211]) >> 16) & 0xff;
	clrsetbits_le32(&denali_phy[211], 0xff << 16, (tmp + 0x10) << 16);
	tmp = (read32(&denali_phy[339]) >> 16) & 0xff;
	clrsetbits_le32(&denali_phy[339], 0xff << 16, (tmp + 0x10) << 16);
	tmp = (read32(&denali_phy[467]) >> 16) & 0xff;
	clrsetbits_le32(&denali_phy[467], 0xff << 16, (tmp + 0x10) << 16);

	phy_io_config(channel, sdram_params);

	/* PHY_DLL_RST_EN */
	clrsetbits_le32(&denali_phy[957], 0x3 << 24, 0x2 << 24);

	/* FIXME: need to care ERROR bit */
	stopwatch_init_msecs_expire(&sw, 100);
	while (!(read32(&denali_ctl[203]) & (1 << 3))) {
		if (stopwatch_expired(&sw))
			return -1;
	}

	clrsetbits_le32(&denali_ctl[68], PWRUP_SREFRESH_EXIT,
			pwrup_srefresh_exit);
	return 0;
}

static void select_per_cs_training_index(u32 channel, u32 rank)
{
	u32 *denali_phy = rk3399_ddr_publ[channel]->denali_phy;

	/* PHY_84 PHY_PER_CS_TRAINING_EN_0 1bit offset_16 */
	if ((read32(&denali_phy[84])>>16) & 1) {
		/*
		 * PHY_8/136/264/392
		 * phy_per_cs_training_index_X 1bit offset_24
		 */
		clrsetbits_le32(&denali_phy[8], 0x1 << 24, rank << 24);
		clrsetbits_le32(&denali_phy[136], 0x1 << 24, rank << 24);
		clrsetbits_le32(&denali_phy[264], 0x1 << 24, rank << 24);
		clrsetbits_le32(&denali_phy[392], 0x1 << 24, rank << 24);
	}
}

static void override_write_leveling_value(u32 channel)
{
	u32 *denali_ctl = rk3399_ddr_pctl[channel]->denali_ctl;
	u32 *denali_phy = rk3399_ddr_publ[channel]->denali_phy;
	u32 byte;

	/* PHY_896 PHY_FREQ_SEL_MULTICAST_EN 1bit offset_0 */
	setbits_le32(&denali_phy[896], 1);

	/*
	 * PHY_8/136/264/392
	 * phy_per_cs_training_multicast_en_X 1bit offset_16
	 */
	clrsetbits_le32(&denali_phy[8], 0x1 << 16, 1 << 16);
	clrsetbits_le32(&denali_phy[136], 0x1 << 16, 1 << 16);
	clrsetbits_le32(&denali_phy[264], 0x1 << 16, 1 << 16);
	clrsetbits_le32(&denali_phy[392], 0x1 << 16, 1 << 16);

	for (byte = 0; byte < 4; byte++)
		clrsetbits_le32(&denali_phy[63 + (128 * byte)], 0xffff << 16,
			0x200 << 16);

	/* PHY_896 PHY_FREQ_SEL_MULTICAST_EN 1bit offset_0 */
	clrbits_le32(&denali_phy[896], 1);

	/* CTL_200 ctrlupd_req 1bit offset_8 */
	clrsetbits_le32(&denali_ctl[200], 0x1 << 8, 0x1 << 8);
}

static int data_training(u32 channel,
			 const struct rk3399_sdram_params *sdram_params,
			 u32 training_flag)
{
	u32 *denali_pi = rk3399_ddr_pi[channel]->denali_pi;
	u32 *denali_phy = rk3399_ddr_publ[channel]->denali_phy;
	u32 i, tmp;
	u32 obs_0, obs_1, obs_2, obs_3, obs_err = 0;
	u32 rank = sdram_params->ch[channel].rank;
	u32 reg_value = 0;

	/* PHY_927 PHY_PAD_DQS_DRIVE  RPULL offset_22 */
	setbits_le32(&denali_phy[927], (1 << 22));

	if (training_flag == PI_FULL_TRAINING) {
		if (sdram_params->dramtype == LPDDR4) {
			training_flag = PI_CA_TRAINING | PI_WRITE_LEVELING |
					PI_READ_GATE_TRAINING |
					PI_READ_LEVELING | PI_WDQ_LEVELING;
		} else if (sdram_params->dramtype == LPDDR3) {
			training_flag = PI_CA_TRAINING | PI_WRITE_LEVELING |
					PI_READ_GATE_TRAINING;
		} else if (sdram_params->dramtype == DDR3) {
			training_flag = PI_WRITE_LEVELING |
					PI_READ_GATE_TRAINING |
					PI_READ_LEVELING;
		}
	}

	/* ca training(LPDDR4,LPDDR3 support) */
	if ((training_flag & PI_CA_TRAINING) == PI_CA_TRAINING) {
		for (i = 0; i < rank; i++) {
			select_per_cs_training_index(channel, i);
			/* PI_100 PI_CALVL_EN:RW:8:2 */
			clrsetbits_le32(&denali_pi[100], 0x3 << 8, 0x2 << 8);
			/* PI_92 PI_CALVL_REQ:WR:16:1,PI_CALVL_CS:RW:24:2 */
			clrsetbits_le32(&denali_pi[92],
					(0x1 << 16) | (0x3 << 24),
					(0x1 << 16) | (i << 24));

			while (1) {
				/* PI_174 PI_INT_STATUS:RD:8:18 */
				tmp = read32(&denali_pi[174]) >> 8;

				/*
				 * check status obs
				 * PHY_532/660/789 phy_adr_calvl_obs1_:0:32
				 */
				obs_0 = read32(&denali_phy[532]);
				obs_1 = read32(&denali_phy[660]);
				obs_2 = read32(&denali_phy[788]);
				if (((obs_0 >> 30) & 0x3) ||
				    ((obs_1 >> 30) & 0x3) ||
				    ((obs_2 >> 30) & 0x3))
					obs_err = 1;
				if ((((tmp >> 11) & 0x1) == 0x1) &&
				    (((tmp >> 13) & 0x1) == 0x1) &&
				    (((tmp >> 5) & 0x1) == 0x0) &&
				    (obs_err == 0))
					break;
				else if ((((tmp >> 5) & 0x1) == 0x1) ||
					 (obs_err == 1))
					return -1;
			}
			/* clear interrupt,PI_175 PI_INT_ACK:WR:0:17 */
			write32((&denali_pi[175]), 0x00003f7c);
		}
		clrbits_le32(&denali_pi[100], 0x3 << 8);
	}

	/* write leveling(LPDDR4,LPDDR3,DDR3 support) */
	if ((training_flag & PI_WRITE_LEVELING) == PI_WRITE_LEVELING) {
		for (i = 0; i < rank; i++) {
			select_per_cs_training_index(channel, i);
			/* PI_60 PI_WRLVL_EN:RW:8:2 */
			clrsetbits_le32(&denali_pi[60], 0x3 << 8, 0x2 << 8);
			/* PI_59 PI_WRLVL_REQ:WR:8:1,PI_WRLVL_CS:RW:16:2 */
			clrsetbits_le32(&denali_pi[59],
					(0x1 << 8) | (0x3 << 16),
					(0x1 << 8) | (i << 16));

			while (1) {
				/* PI_174 PI_INT_STATUS:RD:8:18 */
				tmp = read32(&denali_pi[174]) >> 8;

				/*
				 * check status obs, if error maybe can not
				 * get leveling done PHY_40/168/296/424
				 * phy_wrlvl_status_obs_X:0:13
				 */
				obs_0 = read32(&denali_phy[40]);
				obs_1 = read32(&denali_phy[168]);
				obs_2 = read32(&denali_phy[296]);
				obs_3 = read32(&denali_phy[424]);
				if (((obs_0 >> 12) & 0x1) ||
				    ((obs_1 >> 12) & 0x1) ||
				    ((obs_2 >> 12) & 0x1) ||
				    ((obs_3 >> 12) & 0x1))
					obs_err = 1;
				if ((((tmp >> 10) & 0x1) == 0x1) &&
				    (((tmp >> 13) & 0x1) == 0x1) &&
				    (((tmp >> 4) & 0x1) == 0x0) &&
				    (obs_err == 0))
					break;
				else if ((((tmp >> 4) & 0x1) == 0x1) ||
					 (obs_err == 1))
					return -1;
			}
			/* clear interrupt,PI_175 PI_INT_ACK:WR:0:17 */
			write32((&denali_pi[175]), 0x00003f7c);
		}

		override_write_leveling_value(channel);
		clrbits_le32(&denali_pi[60], 0x3 << 8);
	}

	/* read gate training(LPDDR4,LPDDR3,DDR3 support) */
	if ((training_flag & PI_READ_GATE_TRAINING) == PI_READ_GATE_TRAINING) {

		/*
		 * The differential signal of DQS needs to keep low level
		 * before gate training. RPULL will connect 4Kn from PADP
		 * to VSS and a 4Kn from PADN to VDDQ to ensure it.
		 * But if it has PHY side ODT connect at this time,
		 * it will change the DQS signal level. So disable PHY
		 * side ODT before gate training and restore ODT state
		 * after gate training.
		 */
		if (sdram_params->dramtype != LPDDR4) {
			reg_value = (read32(&denali_phy[6]) >> 24) & 0x7;

			/*
			 * phy_dqs_tsel_enable_X 3bits
			 * DENALI_PHY_6/134/262/390 offset_24
			 */
			clrbits_le32(&denali_phy[6], 0x7 << 24);
			clrbits_le32(&denali_phy[134], 0x7 << 24);
			clrbits_le32(&denali_phy[262], 0x7 << 24);
			clrbits_le32(&denali_phy[390], 0x7 << 24);
		}
		for (i = 0; i < rank; i++) {
			select_per_cs_training_index(channel, i);
			/* PI_80 PI_RDLVL_GATE_EN:RW:24:2 */
			clrsetbits_le32(&denali_pi[80], 0x3 << 24, 0x2 << 24);
			/*
			 * PI_74 PI_RDLVL_GATE_REQ:WR:16:1
			 * PI_RDLVL_CS:RW:24:2
			 */
			clrsetbits_le32(&denali_pi[74],
					(0x1 << 16) | (0x3 << 24),
					(0x1 << 16) | (i << 24));

			while (1) {
				/* PI_174 PI_INT_STATUS:RD:8:18 */
				tmp = read32(&denali_pi[174]) >> 8;

				/*
				 * check status obs
				 * PHY_43/171/299/427
				 *     PHY_GTLVL_STATUS_OBS_x:16:8
				 */
				obs_0 = read32(&denali_phy[43]);
				obs_1 = read32(&denali_phy[171]);
				obs_2 = read32(&denali_phy[299]);
				obs_3 = read32(&denali_phy[427]);
				if (((obs_0 >> (16 + 6)) & 0x3) ||
				    ((obs_1 >> (16 + 6)) & 0x3) ||
				    ((obs_2 >> (16 + 6)) & 0x3) ||
				    ((obs_3 >> (16 + 6)) & 0x3))
					obs_err = 1;
				if ((((tmp >> 9) & 0x1) == 0x1) &&
				    (((tmp >> 13) & 0x1) == 0x1) &&
				    (((tmp >> 3) & 0x1) == 0x0) &&
				    (obs_err == 0))
					break;
				else if ((((tmp >> 3) & 0x1) == 0x1) ||
					 (obs_err == 1))
					return -1;
			}
			/* clear interrupt,PI_175 PI_INT_ACK:WR:0:17 */
			write32((&denali_pi[175]), 0x00003f7c);
		}
		clrbits_le32(&denali_pi[80], 0x3 << 24);

		if (sdram_params->dramtype != LPDDR4) {
			/*
			 * phy_dqs_tsel_enable_X 3bits
			 * DENALI_PHY_6/134/262/390 offset_24
			 */
			tmp = reg_value << 24;
			clrsetbits_le32(&denali_phy[6], 0x7 << 24, tmp);
			clrsetbits_le32(&denali_phy[134], 0x7 << 24, tmp);
			clrsetbits_le32(&denali_phy[262], 0x7 << 24, tmp);
			clrsetbits_le32(&denali_phy[390], 0x7 << 24, tmp);
		}
	}

	/* read leveling(LPDDR4,LPDDR3,DDR3 support) */
	if ((training_flag & PI_READ_LEVELING) == PI_READ_LEVELING) {
		for (i = 0; i < rank; i++) {
			select_per_cs_training_index(channel, i);
			/* PI_80 PI_RDLVL_EN:RW:16:2 */
			clrsetbits_le32(&denali_pi[80], 0x3 << 16, 0x2 << 16);
			/* PI_74 PI_RDLVL_REQ:WR:8:1,PI_RDLVL_CS:RW:24:2 */
			clrsetbits_le32(&denali_pi[74],
					(0x1 << 8) | (0x3 << 24),
					(0x1 << 8) | (i << 24));

			while (1) {
				/* PI_174 PI_INT_STATUS:RD:8:18 */
				tmp = read32(&denali_pi[174]) >> 8;

				/*
				 * make sure status obs not report error bit
				 * PHY_46/174/302/430
				 *     phy_rdlvl_status_obs_X:16:8
				 */
				if ((((tmp >> 8) & 0x1) == 0x1) &&
				    (((tmp >> 13) & 0x1) == 0x1) &&
				    (((tmp >> 2) & 0x1) == 0x0))
					break;
				else if (((tmp >> 2) & 0x1) == 0x1)
					return -1;
			}
			/* clear interrupt,PI_175 PI_INT_ACK:WR:0:17 */
			write32((&denali_pi[175]), 0x00003f7c);
		}
		clrbits_le32(&denali_pi[80], 0x3 << 16);
	}

	/* wdq leveling(LPDDR4 support) */
	if ((training_flag & PI_WDQ_LEVELING) == PI_WDQ_LEVELING) {
		for (i = 0; i < rank; i++) {
			select_per_cs_training_index(channel, i);
			/*
			 * disable PI_WDQLVL_VREF_EN before wdq leveling?
			 * PI_181 PI_WDQLVL_VREF_EN:RW:8:1
			 */
			clrbits_le32(&denali_pi[181], 0x1 << 8);
			/* PI_124 PI_WDQLVL_EN:RW:16:2 */
			clrsetbits_le32(&denali_pi[124], 0x3 << 16, 0x2 << 16);
			/* PI_121 PI_WDQLVL_REQ:WR:8:1,PI_WDQLVL_CS:RW:16:2 */
			clrsetbits_le32(&denali_pi[121],
					(0x1 << 8) | (0x3 << 16),
					(0x1 << 8) | (i << 16));

			while (1) {
				/* PI_174 PI_INT_STATUS:RD:8:18 */
				tmp = read32(&denali_pi[174]) >> 8;
				if ((((tmp >> 12) & 0x1) == 0x1) &&
				    (((tmp >> 13) & 0x1) == 0x1) &&
				    (((tmp >> 6) & 0x1) == 0x0))
					break;
				else if (((tmp >> 6) & 0x1) == 0x1)
					return -1;
			}
			/* clear interrupt,PI_175 PI_INT_ACK:WR:0:17 */
			write32((&denali_pi[175]), 0x00003f7c);
		}
		clrbits_le32(&denali_pi[124], 0x3 << 16);
	}

	/* PHY_927 PHY_PAD_DQS_DRIVE  RPULL offset_22 */
	clrbits_le32(&denali_phy[927], (1 << 22));

	return 0;
}

static void set_ddrconfig(const struct rk3399_sdram_params *sdram_params,
			  unsigned char channel, u32 ddrconfig)
{
	/* only need to set ddrconfig */
	struct rk3399_msch_regs *ddr_msch_regs = rk3399_msch[channel];
	unsigned int cs0_cap = 0;
	unsigned int cs1_cap = 0;

	cs0_cap = (1 << (sdram_params->ch[channel].cs0_row
			+ sdram_params->ch[channel].col
			+ sdram_params->ch[channel].bk
			+ sdram_params->ch[channel].bw - 20));
	if (sdram_params->ch[channel].rank > 1)
		cs1_cap = cs0_cap >> (sdram_params->ch[channel].cs0_row
				- sdram_params->ch[channel].cs1_row);
	if (sdram_params->ch[channel].row_3_4) {
		cs0_cap = cs0_cap * 3 / 4;
		cs1_cap = cs1_cap * 3 / 4;
	}

	write32(&ddr_msch_regs->ddrconf, ddrconfig | (ddrconfig << 8));
	write32(&ddr_msch_regs->ddrsize, ((cs0_cap / 32) & 0xff) |
					 (((cs1_cap / 32) & 0xff) << 8));
}

static void dram_all_config(const struct rk3399_sdram_params *sdram_params)
{
	u32 sys_reg = 0;
	unsigned int channel;
	unsigned int use;

	sys_reg |= SYS_REG_ENC_DDRTYPE(sdram_params->dramtype);
	sys_reg |= SYS_REG_ENC_NUM_CH(sdram_params->num_channels);
	for (channel = 0, use = 0;
	     (use < sdram_params->num_channels) && (channel < 2); channel++) {
		const struct rk3399_sdram_channel *info =
			&sdram_params->ch[channel];
		struct rk3399_msch_regs *ddr_msch_regs;
		const struct rk3399_msch_timings *noc_timing;

		if (sdram_params->ch[channel].col == 0)
			continue;
		use++;
		sys_reg |= SYS_REG_ENC_ROW_3_4(info->row_3_4, channel);
		sys_reg |= SYS_REG_ENC_CHINFO(channel);
		sys_reg |= SYS_REG_ENC_RANK(info->rank, channel);
		sys_reg |= SYS_REG_ENC_COL(info->col, channel);
		sys_reg |= SYS_REG_ENC_BK(info->bk, channel);
		sys_reg |= SYS_REG_ENC_CS0_ROW(info->cs0_row, channel);
		if (sdram_params->ch[channel].rank > 1)
			sys_reg |= SYS_REG_ENC_CS1_ROW(info->cs1_row, channel);
		sys_reg |= SYS_REG_ENC_BW(info->bw, channel);
		sys_reg |= SYS_REG_ENC_DBW(info->dbw, channel);

		ddr_msch_regs = rk3399_msch[channel];
		noc_timing = &sdram_params->ch[channel].noc_timings;
		write32(&ddr_msch_regs->ddrtiminga0.d32,
			noc_timing->ddrtiminga0.d32);
		write32(&ddr_msch_regs->ddrtimingb0.d32,
			noc_timing->ddrtimingb0.d32);
		write32(&ddr_msch_regs->ddrtimingc0.d32,
			noc_timing->ddrtimingc0.d32);
		write32(&ddr_msch_regs->devtodev0.d32,
			noc_timing->devtodev0.d32);
		write32(&ddr_msch_regs->ddrmode.d32,
			noc_timing->ddrmode.d32);

		/* rank 1 memory clock disable (dfi_dram_clk_disable = 1) */
		if (sdram_params->ch[channel].rank == 1)
			setbits_le32(&rk3399_ddr_pctl[channel]->denali_ctl[276],
				     1 << 17);
	}

	write32(&rk3399_pmugrf->os_reg2, sys_reg);
	DDR_STRIDE(sdram_params->stride);

	/* reboot hold register set */
	write32(&pmucru_ptr->pmucru_rstnhold_con[1],
		PRESET_SGRF_HOLD(0) | PRESET_GPIO0_HOLD(1) |
		PRESET_GPIO1_HOLD(1));
	clrsetbits_le32(&cru_ptr->glb_rst_con, 0x3, 0x3);
}

static void switch_to_phy_index1(const struct rk3399_sdram_params *sdram_params)
{
	u32 channel;
	u32 *denali_phy;
	struct stopwatch sw;
	u32 ch_count = sdram_params->num_channels;

	stopwatch_init_msecs_expire(&sw, 100);
	write32(&rk3399_ddr_cic->cic_ctrl0,
		RK_CLRSETBITS(0x03 << 4 | 1 << 2 | 1,
			      1 << 4 | 1 << 2 | 1));
	while (!(read32(&rk3399_ddr_cic->cic_status0) & (1 << 2))) {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR,
			       "index1 frequency change overtime, reset\n");
			hard_reset();
		}
	}

	stopwatch_init_msecs_expire(&sw, 100);
	write32(&rk3399_ddr_cic->cic_ctrl0, RK_CLRSETBITS(1 << 1, 1 << 1));
	while (!(read32(&rk3399_ddr_cic->cic_status0) & (1 << 0))) {
		if (stopwatch_expired(&sw)) {
			printk(BIOS_ERR,
			       "index1 frequency done overtime, reset\n");
			hard_reset();
		}
	}

	for (channel = 0; channel < ch_count; channel++) {
		denali_phy = rk3399_ddr_publ[channel]->denali_phy;
		clrsetbits_le32(&denali_phy[896], (0x3 << 8) | 1, 1 << 8);
		if (data_training(channel, sdram_params, PI_FULL_TRAINING)) {
			printk(BIOS_ERR, "index1 training failed, reset\n");
			hard_reset();
		}
	}
}

void sdram_init(const struct rk3399_sdram_params *sdram_params)
{
	unsigned char dramtype = sdram_params->dramtype;
	unsigned int ddr_freq = sdram_params->ddr_freq;
	int channel;

	printk(BIOS_INFO, "Starting SDRAM initialization...\n");

	if ((dramtype == DDR3 && ddr_freq > 800*MHz) ||
	    (dramtype == LPDDR3 && ddr_freq > 933*MHz) ||
	    (dramtype == LPDDR4 && ddr_freq > 800*MHz))
		die("SDRAM frequency is to high!");

	rkclk_configure_ddr(ddr_freq);

	for (channel = 0; channel < 2; channel++) {
		phy_dll_bypass_set(channel, rk3399_ddr_publ[channel], ddr_freq);

		if (channel >= sdram_params->num_channels)
			continue;

		/*
		 * TODO: we need to find the root cause why this
		 * step may fail, before that, we just reset the
		 * system, and start again.
		 */
		if (pctl_cfg(channel, sdram_params) != 0) {
			printk(BIOS_ERR, "pctl_cfg fail, reset\n");
			hard_reset();
		}

		/* LPDDR2/LPDDR3 need to wait DAI complete, max 10us */
		if (dramtype == LPDDR3)
			udelay(10);

		if (data_training(channel, sdram_params, PI_FULL_TRAINING)) {
			printk(BIOS_ERR,
			       "SDRAM initialization failed, reset\n");
			hard_reset();
		}

		set_ddrconfig(sdram_params, channel,
			      sdram_params->ch[channel].ddrconfig);
	}
	dram_all_config(sdram_params);
	switch_to_phy_index1(sdram_params);

	printk(BIOS_INFO, "Finish SDRAM initialization...\n");
}

size_t sdram_size_mb(void)
{
	u32 rank, col, bk, cs0_row, cs1_row, bw, row_3_4;
	size_t chipsize_mb = 0;
	static size_t size_mb = 0;
	u32 ch;

	if (!size_mb) {
		u32 sys_reg = read32(&rk3399_pmugrf->os_reg2);
		u32 ch_num = SYS_REG_DEC_NUM_CH(sys_reg);

		for (ch = 0; ch < ch_num; ch++) {
			rank = SYS_REG_DEC_RANK(sys_reg, ch);
			col = SYS_REG_DEC_COL(sys_reg, ch);
			bk = SYS_REG_DEC_BK(sys_reg, ch);
			cs0_row = SYS_REG_DEC_CS0_ROW(sys_reg, ch);
			cs1_row = SYS_REG_DEC_CS1_ROW(sys_reg, ch);
			bw = SYS_REG_DEC_BW(sys_reg, ch);
			row_3_4 = SYS_REG_DEC_ROW_3_4(sys_reg, ch);

			chipsize_mb = (1 << (cs0_row + col + bk + bw - 20));

			if (rank > 1)
				chipsize_mb += chipsize_mb >>
					(cs0_row - cs1_row);
			if (row_3_4)
				chipsize_mb = chipsize_mb * 3 / 4;
			size_mb += chipsize_mb;
		}

		/*
		 * we use the 0x00000000~0xf7ffffff space
		 * since 0xf8000000~0xffffffff is soc register space
		 * so we reserve it
		 */
		size_mb = MIN(size_mb, 0xf8000000/MiB);
	}

	return size_mb;
}
