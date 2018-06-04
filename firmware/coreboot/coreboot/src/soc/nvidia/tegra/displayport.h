/*
 * This file is part of the coreboot project.
 *
 * drivers/video/tegra/dc/dpaux_regs.h
 *
 * Copyright (c) 2014, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SOC_NVIDIA_TEGRA_DISPLAYPORT_H__
#define __SOC_NVIDIA_TEGRA_DISPLAYPORT_H__

#include <soc/sor.h>

/* things we can't get rid of just yet. */
#define DPAUX_INTR_EN_AUX				(0x1)
#define DPAUX_INTR_AUX					(0x5)
#define DPAUX_DP_AUXDATA_WRITE_W(i)		     (0x9 + 4*(i))
#define DPAUX_DP_AUXDATA_READ_W(i)		     (0x19 + 4*(i))
#define DPAUX_DP_AUXADDR				(0x29)
#define DPAUX_DP_AUXCTL					(0x2d)
#define DPAUX_DP_AUXCTL_CMDLEN_SHIFT			(0)
#define DPAUX_DP_AUXCTL_CMDLEN_FIELD			(0xff)
#define DPAUX_DP_AUXCTL_CMD_SHIFT			(12)
#define DPAUX_DP_AUXCTL_CMD_MASK			(0xf << 12)
#define DPAUX_DP_AUXCTL_CMD_I2CWR			(0 << 12)
#define DPAUX_DP_AUXCTL_CMD_I2CRD			(1 << 12)
#define DPAUX_DP_AUXCTL_CMD_I2CREQWSTAT			(2 << 12)
#define DPAUX_DP_AUXCTL_CMD_MOTWR			(4 << 12)
#define DPAUX_DP_AUXCTL_CMD_MOTRD			(5 << 12)
#define DPAUX_DP_AUXCTL_CMD_MOTREQWSTAT			(6 << 12)
#define DPAUX_DP_AUXCTL_CMD_AUXWR			(8 << 12)
#define DPAUX_DP_AUXCTL_CMD_AUXRD			(9 << 12)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_SHIFT		(16)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_MASK		(0x1 << 16)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_DONE		(0 << 16)
#define DPAUX_DP_AUXCTL_TRANSACTREQ_PENDING		(1 << 16)
#define DPAUX_DP_AUXCTL_RST_SHIFT			(31)
#define DPAUX_DP_AUXCTL_RST_DEASSERT			(0 << 31)
#define DPAUX_DP_AUXCTL_RST_ASSERT			(1 << 31)
#define DPAUX_DP_AUXSTAT				(0x31)
#define DPAUX_DP_AUXSTAT_HPD_STATUS_SHIFT		(28)
#define DPAUX_DP_AUXSTAT_HPD_STATUS_UNPLUG		(0 << 28)
#define DPAUX_DP_AUXSTAT_HPD_STATUS_PLUGGED		(1 << 28)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_SHIFT		(20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_MASK		(0xf << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_IDLE		(0 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_SYNC		(1 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_START1		(2 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_COMMAND		(3 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_ADDRESS		(4 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_LENGTH		(5 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_WRITE1		(6 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_READ1		(7 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_GET_M		(8 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_STOP1		(9 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_STOP2		(10 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_REPLY		(11 << 20)
#define DPAUX_DP_AUXSTAT_AUXCTL_STATE_CLEANUP		(12 << 20)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_SHIFT		(16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_MASK			(0xf << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_ACK			(0 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_NACK			(1 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_DEFER		(2 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_I2CNACK		(4 << 16)
#define DPAUX_DP_AUXSTAT_REPLYTYPE_I2CDEFER		(8 << 16)
#define DPAUX_DP_AUXSTAT_NO_STOP_ERROR_SHIFT		(11)
#define DPAUX_DP_AUXSTAT_NO_STOP_ERROR_NOT_PENDING	(0 << 11)
#define DPAUX_DP_AUXSTAT_NO_STOP_ERROR_PENDING		(1 << 11)
#define DPAUX_DP_AUXSTAT_SINKSTAT_ERROR_SHIFT		(10)
#define DPAUX_DP_AUXSTAT_SINKSTAT_ERROR_NOT_PENDING	(0 << 10)
#define DPAUX_DP_AUXSTAT_SINKSTAT_ERROR_PENDING		(1 << 10)
#define DPAUX_DP_AUXSTAT_RX_ERROR_SHIFT			(9)
#define DPAUX_DP_AUXSTAT_RX_ERROR_NOT_PENDING		(0 << 9)
#define DPAUX_DP_AUXSTAT_RX_ERROR_PENDING		(1 << 9)
#define DPAUX_DP_AUXSTAT_TIMEOUT_ERROR_SHIFT		(8)
#define DPAUX_DP_AUXSTAT_TIMEOUT_ERROR_NOT_PENDING	(0 << 8)
#define DPAUX_DP_AUXSTAT_TIMEOUT_ERROR_PENDING		(1 << 8)
#define DPAUX_DP_AUXSTAT_REPLY_M_SHIFT			(0)
#define DPAUX_DP_AUXSTAT_REPLY_M_MASK			(0xff << 0)
#define DPAUX_HPD_CONFIG				(0x3d)
#define DPAUX_HPD_IRQ_CONFIG				(0x41)
#define DPAUX_DP_AUX_CONFIG				(0x45)
#define DPAUX_HYBRID_PADCTL				(0x49)
#define DPAUX_HYBRID_PADCTL_I2C_SDA_INPUT_RCV_SHIFT	(15)
#define DPAUX_HYBRID_PADCTL_I2C_SDA_INPUT_RCV_DISABLE	(0 << 15)
#define DPAUX_HYBRID_PADCTL_I2C_SDA_INPUT_RCV_ENABLE	(1 << 15)
#define DPAUX_HYBRID_PADCTL_I2C_SCL_INPUT_RCV_SHIFT	(14)
#define DPAUX_HYBRID_PADCTL_I2C_SCL_INPUT_RCV_DISABLE	(0 << 14)
#define DPAUX_HYBRID_PADCTL_I2C_SCL_INPUT_RCV_ENABLE	(1 << 14)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_SHIFT		(12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_DEFAULT_MASK	(0x3 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_60		(0 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_64		(1 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_70		(2 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_CMH_V0_56		(3 << 12)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_SHIFT		(8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_DEFAULT_MASK	(0x7 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_78		(0 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_60		(1 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_54		(2 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_45		(3 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_50		(4 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_42		(5 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_39		(6 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVZ_OHM_34		(7 << 8)
#define DPAUX_HYBRID_PADCTL_AUX_DRVI_SHIFT		(2)
#define DPAUX_HYBRID_PADCTL_AUX_DRVI_DEFAULT_MASK	(0x3f << 2)
#define DPAUX_HYBRID_PADCTL_AUX_INPUT_RCV_SHIFT		(1)
#define DPAUX_HYBRID_PADCTL_AUX_INPUT_RCV_DISABLE	(0 << 1)
#define DPAUX_HYBRID_PADCTL_AUX_INPUT_RCV_ENABLE	(1 << 1)
#define DPAUX_HYBRID_PADCTL_MODE_SHIFT			(0)
#define DPAUX_HYBRID_PADCTL_MODE_AUX			(0)
#define DPAUX_HYBRID_PADCTL_MODE_I2C			(1)
#define DPAUX_HYBRID_SPARE				(0x4d)
#define DPAUX_HYBRID_SPARE_PAD_PWR_POWERUP		(0)
#define DPAUX_HYBRID_SPARE_PAD_PWR_POWERDOWN		(1)

#define	DPAUX_HPD_CONFIG_UNPLUG_MIN_TIME_SHIFT		(16)

/* TODO: figure out which of the NV_ constants are the same as all the other
 * display port standard constants.
 */

#define DP_AUX_DEFER_MAX_TRIES		7
#define DP_AUX_TIMEOUT_MAX_TRIES	2
#define DP_POWER_ON_MAX_TRIES		3
#define DP_CLOCK_RECOVERY_MAX_TRIES	7
#define DP_CLOCK_RECOVERY_TOT_TRIES	15

#define DP_AUX_MAX_BYTES		16

#define DP_AUX_TIMEOUT_MS		40
#define DP_DPCP_RETRY_SLEEP_NS		400

static const u32 tegra_dp_vs_regs[][4][4] = {
	/* postcursor2 L0 */
	{
		/* pre-emphasis: L0, L1, L2, L3 */
		{0x13, 0x19, 0x1e, 0x28}, /* voltage swing: L0 */
		{0x1e, 0x25, 0x2d}, /* L1 */
		{0x28, 0x32}, /* L2 */
		{0x3c}, /* L3 */
	},

	/* postcursor2 L1 */
	{
		{0x12, 0x17, 0x1b, 0x25},
		{0x1c, 0x23, 0x2a},
		{0x25, 0x2f},
		{0x39},
	},

	/* postcursor2 L2 */
	{
		{0x12, 0x16, 0x1a, 0x22},
		{0x1b, 0x20, 0x27},
		{0x24, 0x2d},
		{0x36},
	},

	/* postcursor2 L3 */
	{
		{0x11, 0x14, 0x17, 0x1f},
		{0x19, 0x1e, 0x24},
		{0x22, 0x2a},
		{0x32},
	},
};

static const u32 tegra_dp_pe_regs[][4][4] = {
	/* postcursor2 L0 */
	{
		/* pre-emphasis: L0, L1, L2, L3 */
		{0x00, 0x09, 0x13, 0x25}, /* voltage swing: L0 */
		{0x00, 0x0f, 0x1e}, /* L1 */
		{0x00, 0x14}, /* L2 */
		{0x00}, /* L3 */
	},

	/* postcursor2 L1 */
	{
		{0x00, 0x0a, 0x14, 0x28},
		{0x00, 0x0f, 0x1e},
		{0x00, 0x14},
		{0x00},
	},

	/* postcursor2 L2 */
	{
		{0x00, 0x0a, 0x14, 0x28},
		{0x00, 0x0f, 0x1e},
		{0x00, 0x14},
		{0x00},
	},

	/* postcursor2 L3 */
	{
		{0x00, 0x0a, 0x14, 0x28},
		{0x00, 0x0f, 0x1e},
		{0x00, 0x14},
		{0x00},
	},
};

static const u32 tegra_dp_pc_regs[][4][4] = {
	/* postcursor2 L0 */
	{
		/* pre-emphasis: L0, L1, L2, L3 */
		{0x00, 0x00, 0x00, 0x00}, /* voltage swing: L0 */
		{0x00, 0x00, 0x00}, /* L1 */
		{0x00, 0x00}, /* L2 */
		{0x00}, /* L3 */
	},

	/* postcursor2 L1 */
	{
		{0x02, 0x02, 0x04, 0x05},
		{0x02, 0x04, 0x05},
		{0x04, 0x05},
		{0x05},
	},

	/* postcursor2 L2 */
	{
		{0x04, 0x05, 0x08, 0x0b},
		{0x05, 0x09, 0x0b},
		{0x08, 0x0a},
		{0x0b},
	},

	/* postcursor2 L3 */
	{
		{0x05, 0x09, 0x0b, 0x12},
		{0x09, 0x0d, 0x12},
		{0x0b, 0x0f},
		{0x12},
	},
};

static const u32 tegra_dp_tx_pu[][4][4] = {
	/* postcursor2 L0 */
	{
		/* pre-emphasis: L0, L1, L2, L3 */
		{0x20, 0x30, 0x40, 0x60}, /* voltage swing: L0 */
		{0x30, 0x40, 0x60}, /* L1 */
		{0x40, 0x60}, /* L2 */
		{0x60}, /* L3 */
	},

	/* postcursor2 L1 */
	{
		{0x20, 0x20, 0x30, 0x50},
		{0x30, 0x40, 0x50},
		{0x40, 0x50},
		{0x60},
	},

	/* postcursor2 L2 */
	{
		{0x20, 0x20, 0x30, 0x40},
		{0x30, 0x30, 0x40},
		{0x40, 0x50},
		{0x60},
	},

	/* postcursor2 L3 */
	{
		{0x20, 0x20, 0x20, 0x40},
		{0x30, 0x30, 0x40},
		{0x40, 0x40},
		{0x60},
	},
};

enum {
	driveCurrent_Level0 = 0,
	driveCurrent_Level1 = 1,
	driveCurrent_Level2 = 2,
	driveCurrent_Level3 = 3,
};

enum {
	preEmphasis_Disabled = 0,
	preEmphasis_Level1   = 1,
	preEmphasis_Level2   = 2,
	preEmphasis_Level3   = 3,
};

enum {
	postCursor2_Level0 = 0,
	postCursor2_Level1 = 1,
	postCursor2_Level2 = 2,
	postCursor2_Level3 = 3,
	postCursor2_Supported
};

static inline int tegra_dp_is_max_vs(u32 pe, u32 vs)
{
	return (vs < (driveCurrent_Level3 - pe)) ? 0 : 1;
}

static inline int tegra_dp_is_max_pe(u32 pe, u32 vs)
{
	return (pe < (preEmphasis_Level3 - vs)) ? 0 : 1;
}

static inline int tegra_dp_is_max_pc(u32 pc)
{
	return (pc < postCursor2_Level3) ? 0 : 1;
}

/* the +10ms is the time for power rail going up from 10-90% or
   90%-10% on powerdown */
/* Time from power-rail is turned on and aux/12c-over-aux is available */
#define EDP_PWR_ON_TO_AUX_TIME_MS	    (200+10)
/* Time from power-rail is turned on and MainLink is available for LT */
#define EDP_PWR_ON_TO_ML_TIME_MS	    (200+10)
/* Time from turning off power to turn-it on again (does not include post
   poweron time) */
#define EDP_PWR_OFF_TO_ON_TIME_MS	    (500+10)

struct tegra_dc_dp_data {
	struct tegra_dc			*dc;
	struct tegra_dc_sor_data	sor;
	void				*aux_base;
	struct tegra_dc_dp_link_config	link_cfg;
	u8				revision;
	int				enabled;
};


/* DPCD definitions */
/* you know, all the vendors pick their own set of defines.
 * All of them.
 * FIXME so we can use the ones in include/device/drm_dp_helper.h
 */
#define NV_DPCD_REV					(0x00000000)
#define NV_DPCD_REV_MAJOR_SHIFT				(4)
#define NV_DPCD_REV_MAJOR_MASK				(0xf << 4)
#define NV_DPCD_REV_MINOR_SHIFT				(0)
#define NV_DPCD_REV_MINOR_MASK				(0xf)
#define NV_DPCD_MAX_LINK_BANDWIDTH			(0x00000001)
#define NV_DPCD_MAX_LINK_BANDWIDTH_VAL_1_62_GPBS	(0x00000006)
#define NV_DPCD_MAX_LINK_BANDWIDTH_VAL_2_70_GPBS	(0x0000000a)
#define NV_DPCD_MAX_LINK_BANDWIDTH_VAL_5_40_GPBS	(0x00000014)
#define NV_DPCD_MAX_LANE_COUNT				(0x00000002)
#define NV_DPCD_MAX_LANE_COUNT_MASK			(0x1f)
#define NV_DPCD_MAX_LANE_COUNT_LANE_1			(0x00000001)
#define NV_DPCD_MAX_LANE_COUNT_LANE_2			(0x00000002)
#define NV_DPCD_MAX_LANE_COUNT_LANE_4			(0x00000004)
#define NV_DPCD_MAX_LANE_COUNT_TPS3_SUPPORTED_YES       (0x00000001 << 6)
#define NV_DPCD_MAX_LANE_COUNT_ENHANCED_FRAMING_NO	(0x00000000 << 7)
#define NV_DPCD_MAX_LANE_COUNT_ENHANCED_FRAMING_YES	(0x00000001 << 7)
#define NV_DPCD_MAX_DOWNSPREAD				(0x00000003)
#define NV_DPCD_MAX_DOWNSPREAD_VAL_NONE			(0x00000000)
#define NV_DPCD_MAX_DOWNSPREAD_VAL_0_5_PCT		(0x00000001)
#define NV_DPCD_MAX_DOWNSPREAD_NO_AUX_HANDSHAKE_LT_F	(0x00000000 << 6)
#define NV_DPCD_MAX_DOWNSPREAD_NO_AUX_HANDSHAKE_LT_T	(0x00000001 << 6)
#define NV_DPCD_EDP_CONFIG_CAP				(0x0000000D)
#define NV_DPCD_EDP_CONFIG_CAP_ASC_RESET_NO		(0x00000000)
#define NV_DPCD_EDP_CONFIG_CAP_ASC_RESET_YES		(0x00000001)
#define NV_DPCD_EDP_CONFIG_CAP_FRAMING_CHANGE_NO	(0x00000000 << 1)
#define NV_DPCD_EDP_CONFIG_CAP_FRAMING_CHANGE_YES	(0x00000001 << 1)
#define NV_DPCD_TRAINING_AUX_RD_INTERVAL				(0x0000000E)
#define NV_DPCD_LINK_BANDWIDTH_SET			(0x00000100)
#define NV_DPCD_LANE_COUNT_SET				(0x00000101)
#define NV_DPCD_LANE_COUNT_SET_ENHANCEDFRAMING_F	(0x00000000 << 7)
#define NV_DPCD_LANE_COUNT_SET_ENHANCEDFRAMING_T	(0x00000001 << 7)
#define NV_DPCD_TRAINING_PATTERN_SET			(0x00000102)
#define NV_DPCD_TRAINING_PATTERN_SET_TPS_MASK		0x3
#define NV_DPCD_TRAINING_PATTERN_SET_TPS_NONE		(0x00000000)
#define NV_DPCD_TRAINING_PATTERN_SET_TPS_TP1		(0x00000001)
#define NV_DPCD_TRAINING_PATTERN_SET_TPS_TP2		(0x00000002)
#define NV_DPCD_TRAINING_PATTERN_SET_TPS_TP3		(0x00000003)
#define NV_DPCD_TRAINING_PATTERN_SET_SC_DISABLED_F	(0x00000000 << 5)
#define NV_DPCD_TRAINING_PATTERN_SET_SC_DISABLED_T	(0x00000001 << 5)
#define NV_DPCD_TRAINING_LANE0_SET			(0x00000103)
#define NV_DPCD_TRAINING_LANE1_SET			(0x00000104)
#define NV_DPCD_TRAINING_LANE2_SET			(0x00000105)
#define NV_DPCD_TRAINING_LANE3_SET			(0x00000106)
#define NV_DPCD_TRAINING_LANEX_SET_DC_SHIFT		0
#define NV_DPCD_TRAINING_LANEX_SET_DC_MAX_REACHED_T	(0x00000001 << 2)
#define NV_DPCD_TRAINING_LANEX_SET_DC_MAX_REACHED_F     (0x00000000 << 2)
#define NV_DPCD_TRAINING_LANEX_SET_PE_SHIFT		3
#define NV_DPCD_TRAINING_LANEX_SET_PE_MAX_REACHED_T	(0x00000001 << 5)
#define NV_DPCD_TRAINING_LANEX_SET_PE_MAX_REACHED_F	(0x00000000 << 5)
#define NV_DPCD_DOWNSPREAD_CTRL				(0x00000107)
#define NV_DPCD_DOWNSPREAD_CTRL_SPREAD_AMP_NONE		(0x00000000 << 4)
#define NV_DPCD_DOWNSPREAD_CTRL_SPREAD_AMP_LT_0_5	(0x00000001 << 4)
#define NV_DPCD_MAIN_LINK_CHANNEL_CODING_SET		(0x00000108)
#define NV_DPCD_MAIN_LINK_CHANNEL_CODING_SET_ANSI_8B10B	1
#define NV_DPCD_EDP_CONFIG_SET				(0x0000010A)
#define NV_DPCD_EDP_CONFIG_SET_ASC_RESET_DISABLE	(0x00000000)
#define NV_DPCD_EDP_CONFIG_SET_ASC_RESET_ENABLE		(0x00000001)
#define NV_DPCD_EDP_CONFIG_SET_FRAMING_CHANGE_DISABLE	(0x00000000 << 1)
#define NV_DPCD_EDP_CONFIG_SET_FRAMING_CHANGE_ENABLE	(0x00000001 << 1)
#define NV_DPCD_TRAINING_LANE0_1_SET2			(0x0000010F)
#define NV_DPCD_TRAINING_LANE2_3_SET2			(0x00000110)
#define NV_DPCD_LANEX_SET2_PC2_SHIFT			0
#define NV_DPCD_LANEX_SET2_PC2_MAX_REACHED_T		(0x00000001 << 2)
#define NV_DPCD_LANEX_SET2_PC2_MAX_REACHED_F		(0x00000000 << 2)
#define NV_DPCD_LANEXPLUS1_SET2_PC2_SHIFT		4
#define NV_DPCD_LANEXPLUS1_SET2_PC2_MAX_REACHED_T	(0x00000001 << 6)
#define NV_DPCD_LANEXPLUS1_SET2_PC2_MAX_REACHED_F	(0x00000000 << 6)
#define NV_DPCD_SINK_COUNT				(0x00000200)
#define NV_DPCD_DEVICE_SERVICE_IRQ_VECTOR		(0x00000201)
#define NV_DPCD_DEVICE_SERVICE_IRQ_VECTOR_AUTO_TEST_NO	(0x00000000 << 1)
#define NV_DPCD_DEVICE_SERVICE_IRQ_VECTOR_AUTO_TEST_YES	(0x00000001 << 1)
#define NV_DPCD_DEVICE_SERVICE_IRQ_VECTOR_CP_NO		(0x00000000 << 2)
#define NV_DPCD_DEVICE_SERVICE_IRQ_VECTOR_CP_YES	(0x00000001 << 2)
#define NV_DPCD_LANE0_1_STATUS				(0x00000202)
#define NV_DPCD_LANE2_3_STATUS				(0x00000203)
#define NV_DPCD_STATUS_LANEX_CR_DONE_SHIFT		0
#define NV_DPCD_STATUS_LANEX_CR_DONE_NO			(0x00000000)
#define NV_DPCD_STATUS_LANEX_CR_DONE_YES		(0x00000001)
#define NV_DPCD_STATUS_LANEX_CHN_EQ_DONE_SHIFT		1
#define NV_DPCD_STATUS_LANEX_CHN_EQ_DONE_NO		(0x00000000 << 1)
#define NV_DPCD_STATUS_LANEX_CHN_EQ_DONE_YES		(0x00000001 << 1)
#define NV_DPCD_STATUS_LANEX_SYMBOL_LOCKED_SHFIT	2
#define NV_DPCD_STATUS_LANEX_SYMBOL_LOCKED_NO		(0x00000000 << 2)
#define NV_DPCD_STATUS_LANEX_SYMBOL_LOCKED_YES		(0x00000001 << 2)
#define NV_DPCD_STATUS_LANEXPLUS1_CR_DONE_SHIFT		4
#define NV_DPCD_STATUS_LANEXPLUS1_CR_DONE_NO		(0x00000000 << 4)
#define NV_DPCD_STATUS_LANEXPLUS1_CR_DONE_YES		(0x00000001 << 4)
#define NV_DPCD_STATUS_LANEXPLUS1_CHN_EQ_DONE_SHIFT	5
#define NV_DPCD_STATUS_LANEXPLUS1_CHN_EQ_DONE_NO	(0x00000000 << 5)
#define NV_DPCD_STATUS_LANEXPLUS1_CHN_EQ_DONE_YES	(0x00000001 << 5)
#define NV_DPCD_STATUS_LANEXPLUS1_SYMBOL_LOCKED_SHIFT	6
#define NV_DPCD_STATUS_LANEXPLUS1_SYMBOL_LOCKED_NO	(0x00000000 << 6)
#define NV_DPCD_STATUS_LANEXPLUS1_SYMBOL_LOCKED_YES	(0x00000001 << 6)
#define NV_DPCD_LANE_ALIGN_STATUS_UPDATED		(0x00000204)
#define NV_DPCD_LANE_ALIGN_STATUS_UPDATED_DONE_NO	(0x00000000)
#define NV_DPCD_LANE_ALIGN_STATUS_UPDATED_DONE_YES	(0x00000001)
#define NV_DPCD_SINK_STATUS				(0x00000205)
#define NV_DPCD_SINK_STATUS_PORT0_IN_SYNC		(0x1 << 0)
#define NV_DPCD_LANE0_1_ADJUST_REQ			(0x00000206)
#define NV_DPCD_LANE2_3_ADJUST_REQ			(0x00000207)
#define NV_DPCD_ADJUST_REQ_LANEX_DC_SHIFT		0
#define NV_DPCD_ADJUST_REQ_LANEX_DC_MASK		0x3
#define NV_DPCD_ADJUST_REQ_LANEX_PE_SHIFT		2
#define NV_DPCD_ADJUST_REQ_LANEX_PE_MASK		(0x3 << 2)
#define NV_DPCD_ADJUST_REQ_LANEXPLUS1_DC_SHIFT		4
#define NV_DPCD_ADJUST_REQ_LANEXPLUS1_DC_MASK		(0x3 << 4)
#define NV_DPCD_ADJUST_REQ_LANEXPLUS1_PE_SHIFT		6
#define NV_DPCD_ADJUST_REQ_LANEXPLUS1_PE_MASK		(0x3 << 6)
#define NV_DPCD_ADJUST_REQ_POST_CURSOR2			(0x0000020C)
#define NV_DPCD_ADJUST_REQ_POST_CURSOR2_LANE_MASK	0x3
#define NV_DPCD_ADJUST_REQ_POST_CURSOR2_LANE_SHIFT(i)	(i*2)
#define NV_DPCD_TEST_REQUEST				(0x00000218)
#define NV_DPCD_SOURCE_IEEE_OUI				(0x00000300)
#define NV_DPCD_SINK_IEEE_OUI				(0x00000400)
#define NV_DPCD_BRANCH_IEEE_OUI				(0x00000500)
#define NV_DPCD_SET_POWER				(0x00000600)
#define NV_DPCD_SET_POWER_VAL_RESERVED			(0x00000000)
#define NV_DPCD_SET_POWER_VAL_D0_NORMAL			(0x00000001)
#define NV_DPCD_SET_POWER_VAL_D3_PWRDWN			(0x00000002)
#define NV_DPCD_HDCP_BKSV_OFFSET			(0x00068000)
#define NV_DPCD_HDCP_RPRIME_OFFSET			(0x00068005)
#define NV_DPCD_HDCP_AKSV_OFFSET			(0x00068007)
#define NV_DPCD_HDCP_AN_OFFSET				(0x0006800C)
#define NV_DPCD_HDCP_VPRIME_OFFSET			(0x00068014)
#define NV_DPCD_HDCP_BCAPS_OFFSET			(0x00068028)
#define NV_DPCD_HDCP_BSTATUS_OFFSET			(0x00068029)
#define NV_DPCD_HDCP_BINFO_OFFSET			(0x0006802A)
#define NV_DPCD_HDCP_KSV_FIFO_OFFSET			(0x0006802C)
#define NV_DPCD_HDCP_AINFO_OFFSET			(0x0006803B)
#endif /* __SOC_NVIDIA_TEGRA_DISPLAYPORT_H__ */
