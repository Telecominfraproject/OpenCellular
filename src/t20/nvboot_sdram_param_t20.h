/*
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

/**
 * Defines the SDRAM parameter structure.
 *
 * Note that PLLM is used by EMC.
 */

#ifndef INCLUDED_NVBOOT_SDRAM_PARAM_T20_H
#define INCLUDED_NVBOOT_SDRAM_PARAM_T20_H

#define NVBOOT_BCT_SDRAM_ARB_CONFIG_WORDS 27

typedef enum {
	/* Specifies the memory type to be undefined */
	nvboot_memory_type_none = 0,

	/* Specifies the memory type to be DDR SDRAM */
	nvboot_memory_type_ddr,

	/* Specifies the memory type to be LPDDR SDRAM */
	nvboot_memory_type_lpddr,

	/* Specifies the memory type to be DDR2 SDRAM */
	nvboot_memory_type_ddr2,

	/* Specifies the memory type to be LPDDR2 SDRAM */
	nvboot_memory_type_lpddr2,

	nvboot_memory_type_num,
	nvboot_memory_type_force32 = 0x7FFFFFF
} nvboot_memory_type;


/**
 * Defines the SDRAM parameter structure
 */
typedef struct nvboot_sdram_params_rec {
	/* Specifies the type of memory device */
	nvboot_memory_type memory_type;

	/* Specifies the CPCON value for PllM */
	uint32_t pllm_charge_pump_setup_ctrl;
	/* Specifies the LPCON value for PllM */
	uint32_t pllm_loop_filter_setup_ctrl;
	/* Specifies the M value for PllM */
	uint32_t pllm_input_divider;
	/* Specifies the N value for PllM */
	uint32_t pllm_feedback_divider;
	/* Specifies the P value for PllM */
	uint32_t pllm_post_divider;
	/* Specifies the time to wait for PLLM to lock (in microseconds) */
	uint32_t pllm_stable_time;

	/* Specifies the divider for the EMC Clock Source */
	uint32_t emc_clock_divider;


	/* Auto-calibration of EMC pads */

	/* Specifies the value for EMC_AUTO_CAL_INTERVAL */
	uint32_t emc_auto_cal_interval;
	/**
	 * Specifies the value for EMC_AUTO_CAL_CONFIG
	 * Note: Trigger bits are set by the SDRAM code.
	 */
	uint32_t emc_auto_cal_config;
	/**
	 * Specifies the time for the calibration to
	 * stabilize (in microseconds)
	 */
	uint32_t emc_auto_cal_wait;

	/**
	 * Specifies the time to wait after pin programming (in microseconds)
	 * Dram vendors require at least 200us.
	 */
	uint32_t emc_pin_program_wait;


	/* Timing parameters required for the SDRAM */

	/* Specifies the value for EMC_RC */
	uint32_t emc_rc;
	/* Specifies the value for EMC_RFC */
	uint32_t emc_rfc;
	/* Specifies the value for EMC_RAS */
	uint32_t emc_ras;
	/* Specifies the value for EMC_RP */
	uint32_t emc_rp;
	/* Specifies the value for EMC_R2W */
	uint32_t emc_r2w;
	/* Specifies the value for EMC_R2W */
	uint32_t emc_w2r;
	/* Specifies the value for EMC_R2P */
	uint32_t emc_r2p;
	/* Specifies the value for EMC_W2P */
	uint32_t emc_w2p;
	/* Specifies the value for EMC_RD_RCD */
	uint32_t emc_rd_rcd;
	/* Specifies the value for EMC_WR_RCD */
	uint32_t emc_wr_rcd;
	/* Specifies the value for EMC_RRD */
	uint32_t emc_rrd;
	/* Specifies the value for EMC_REXT */
	uint32_t emc_rext;
	/* Specifies the value for EMC_WDV */
	uint32_t emc_wdv;
	/* Specifies the value for EMC_QUSE */
	uint32_t emc_quse;
	/* Specifies the value for EMC_QRST */
	uint32_t emc_qrst;
	/* Specifies the value for EMC_QSAFE */
	uint32_t emc_qsafe;
	/* Specifies the value for EMC_RDV */
	uint32_t emc_rdv;
	/* Specifies the value for EMC_REFRESH */
	uint32_t emc_refresh;
	/* Specifies the value for EMC_BURST_REFRESH_NUM */
	uint32_t emc_burst_refresh_num;
	/* Specifies the value for EMC_PDEX2WR */
	uint32_t emc_pdex2wr;
	/* Specifies the value for EMC_PDEX2RD */
	uint32_t emc_pdex2rd;
	/* Specifies the value for EMC_PCHG2PDEN */
	uint32_t emc_pchg2pden;
	/* Specifies the value for EMC_ACT2PDEN */
	uint32_t emc_act2pden;
	/* Specifies the value for EMC_AR2PDEN */
	uint32_t emc_ar2pden;
	/* Specifies the value for EMC_RW2PDEN */
	uint32_t emc_rw2pden;
	/* Specifies the value for EMC_TXSR */
	uint32_t emc_txsr;
	/* Specifies the value for EMC_TCKE */
	uint32_t emc_tcke;
	/* Specifies the value for EMC_TFAW */
	uint32_t emc_tfaw;
	/* Specifies the value for EMC_TRPAB */
	uint32_t emc_trpab;
	/* Specifies the value for EMC_TCLKSTABLE */
	uint32_t emc_tclkstable;
	/* Specifies the value for EMC_TCLKSTOP */
	uint32_t emc_tclkstop;
	/* Specifies the value for EMC_TREFBW */
	uint32_t emc_trefbw;
	/* Specifies the value for EMC_QUSE_EXTRA */
	uint32_t emc_quse_extra;


	/* FBIO configuration values */

	/* Specifies the value for EMC_FBIO_CFG1 */
	uint32_t emc_fbio_cfg1;
	/* Specifies the value for EMC_FBIO_DQSIB_DLY */
	uint32_t emc_fbio_dqsib_dly;
	/* Specifies the value for EMC_FBIO_DQSIB_DLY_MSB */
	uint32_t emc_fbio_dqsib_dly_msb;
	/* Specifies the value for EMC_FBIO_QUSE_DLY */
	uint32_t emc_fbio_quse_dly;
	/* Specifies the value for EMC_FBIO_QUSE_DLY_MSB */
	uint32_t emc_fbio_quse_dly_msb;
	/* Specifies the value for EMC_FBIO_CFG5 */
	uint32_t emc_fbio_cfg5;
	/* Specifies the value for EMC_FBIO_CFG6 */
	uint32_t emc_fbio_cfg6;
	/* Specifies the value for EMC_FBIO_SPARE */
	uint32_t emc_fbio_spare;


	/* MRS command values */

	/* Specifies the value for EMC_MRS */
	uint32_t emc_mrs;
	/* Specifies the value for EMC_EMRS */
	uint32_t emc_emrs;
	/* Specifies the first  of a sequence of three values for EMC_MRW */
	uint32_t emc_mrw1;
	/* Specifies the second of a sequence of three values for EMC_MRW */
	uint32_t emc_mrw2;
	/* Specifies the third  of a sequence of three values for EMC_MRW */
	uint32_t emc_mrw3;

	/* Specifies the EMC_MRW reset command value */
	uint32_t emc_mrw_reset_command;
	/* Specifies the EMC Reset wait time (in microseconds) */
	uint32_t emc_mrw_reset_ninit_wait;

	/**
	 * Specifies the value for EMC_ADR_CFG
	 * The same value is also used for MC_EMC_ADR_CFG
	 */
	uint32_t emc_adr_cfg;
	/* Specifies the value for EMC_ADR_CFG_1 */
	uint32_t emc_adr_cfg1;

	/**
	 * Specifies the value for MC_EMEM_CFG which holds the external memory
	 * size (in KBytes)
	 * EMEM_SIZE_KB must be <= (Device size in KB * Number of Devices)
	 */
	uint32_t mc_emem_cfg;

	/**
	 * Specifies the value for MC_LOWLATENCY_CONFIG
	 * Mainly for LL_DRAM_INTERLEAVE: Some DRAMs do not support interleave
	 * mode. If so, turn off this bit to get the correct low-latency path
	 * behavior. Reset is ENABLED.
	 */
	uint32_t mc_lowlatency_config;
	/* Specifies the value for EMC_CFG */
	uint32_t emc_cfg;
	/* Specifies the value for EMC_CFG_2 */
	uint32_t emc_cfg2;
	/* Specifies the value for EMC_DBG */
	uint32_t emc_dbg;

	/*
	 * Specifies the value for AHB_ARBITRATION_XBAR_CTRL.
	 * This is used to set the Memory Inid done
	 */
	uint32_t ahb_arbitration_xbar_ctrl;

	/*
	 * Specifies the value for EMC_CFG_DIG_DLL
	 * Note: Trigger bits are set by the SDRAM code.
	 */
	uint32_t emc_cfg_dig_dll;
	/* Specifies the value for EMC_DLL_XFORM_DQS */
	uint32_t emc_dll_xform_dqs;
	/* Specifies the value for EMC_DLL_XFORM_QUSE */
	uint32_t emc_dll_xform_quse;

	/*
	 * Specifies the delay after prgramming the PIN/NOP register during a
	 * WarmBoot0 sequence (in microseconds)
	 */
	uint32_t warm_boot_wait;

	/* Specifies the value for EMC_CTT_TERM_CTRL */
	uint32_t emc_ctt_term_ctrl;

	/* Specifies the value for EMC_ODT_WRITE */
	uint32_t emc_odt_write;
	/* Specifies the value for EMC_ODT_WRITE */
	uint32_t emc_odt_read;

	/*
	 * Specifies the value for EMC_ZCAL_REF_CNT
	 * Only meaningful for LPDDR2. Set to 0 for all other memory types.
	 */
	uint32_t emc_zcal_ref_cnt;
	/*
	 * Specifies the value for EMC_ZCAL_WAIT_CNT
	 * Only meaningful for LPDDR2. Set to 0 for all other memory types.
	 */
	uint32_t emc_zcal_wait_cnt;
	/*
	 * Specifies the value for EMC_ZCAL_MRW_CMD
	 * Only meaningful for LPDDR2. Set to 0 for all other memory types.
	 */
	uint32_t emc_zcal_mrw_cmd;

	/*
	 * Specifies the MRS command value for initilizing
	 * the mode register.
	 */
	uint32_t emc_mrs_reset_dll;
	/* Specifies the MRW command for ZQ initialization of device 0 */
	uint32_t emc_mrw_zq_init_dev0;
	/* Specifies the MRW command for ZQ initialization of device 1 */
	uint32_t emc_mrw_zq_init_dev1;
	/*
	 * Specifies the wait time after programming a ZQ initialization
	 * command (in microseconds)
	 */
	uint32_t emc_mrw_zq_init_wait;
	/*
	 * Specifies the wait time after sending an MRS DLL reset command
	 * (in microseconds)
	 */
	uint32_t emc_mrs_reset_dll_wait;
	/*
	 * Specifies the first of two EMRS commands to initialize mode
	 * registers
	 */
	uint32_t emc_emrs_emr2;
	/*
	 * Specifies the second of two EMRS commands to initialize mode
	 * registers
	 */
	uint32_t emc_emrs_emr3;
	/* Specifies the EMRS command to enable the DDR2 DLL */
	uint32_t emc_emrs_ddr2_dll_enable;
	/* Specifies the MRS command to reset the DDR2 DLL */
	uint32_t emc_mrs_ddr2_dll_reset;
	/* Specifies the EMRS command to set OCD calibration */
	uint32_t emc_emrs_ddr2_ocd_calib;
	/*
	 * Specifies the wait between initializing DDR and setting OCD
	 * calibration (in microseconds)
	 */
	uint32_t emc_ddr2_wait;


	/* Clock trimmers */

	/* Specifies the value for EMC_CFG_CLKTRIM_0 */
	uint32_t emc_cfg_clktrim0;
	/* Specifies the value for EMC_CFG_CLKTRIM_1 */
	uint32_t emc_cfg_clktrim1;
	/* Specifies the value for EMC_CFG_CLKTRIM_2 */
	uint32_t emc_cfg_clktrim2;


	/* Pad controls */

	/* Specifies the value for PMC_DDR_PWR */
	uint32_t pmc_ddr_pwr;
	/* Specifies the value for APB_MISC_GP_XM2CFGAPADCTRL */
	uint32_t apb_misc_gp_xm2cfga_pad_ctrl;
	/* Specifies the value for APB_MISC_GP_XM2CFGCPADCTRL */
	uint32_t apb_misc_gp_xm2cfgc_pad_ctrl;
	/* Specifies the value for APB_MISC_GP_XM2CFGCPADCTRL2 */
	uint32_t apb_misc_gp_xm2cfgc_pad_ctrl2;
	/* Specifies the value for APB_MISC_GP_XM2CFGDPADCTRL */
	uint32_t apb_misc_gp_xm2cfgd_pad_ctrl;
	/* Specifies the value for APB_MISC_GP_XM2CFGDPADCTRL2 */
	uint32_t apb_misc_gp_xm2cfgd_pad_ctrl2;
	/* Specifies the value for APB_MISC_GP_XM2CLKCFGPADCTRL */
	uint32_t apb_misc_gp_xm2clkcfg_Pad_ctrl;
	/* Specifies the value for APB_MISC_GP_XM2COMPPADCTRL */
	uint32_t apb_misc_gp_xm2comp_pad_ctrl;
	/* Specifies the value for APB_MISC_GP_XM2VTTGENPADCTRL */
	uint32_t apb_misc_gp_xm2vttgen_pad_ctrl;

	/*
	 * Specifies storage for arbitration configuration registers
	 * Data passed through to the Bootloader but not used by
	 * the Boot ROM
	 */
	uint32_t arbitration_config[NVBOOT_BCT_SDRAM_ARB_CONFIG_WORDS];
} nvboot_sdram_params;

#endif /* #ifndef INCLUDED_NVBOOT_SDRAM_PARAM_T20_H */

