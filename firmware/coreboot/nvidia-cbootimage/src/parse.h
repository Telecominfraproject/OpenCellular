/*
 * Copyright (c) 2012-2014, NVIDIA CORPORATION.  All rights reserved.
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

/*
 * parse.h - Definitions for the cbootimage parsing code.
 */

/*
 * TODO / Notes
 * - Add doxygen commentary
 */

#ifndef INCLUDED_PARSE_H
#define INCLUDED_PARSE_H

#include "cbootimage.h"


/*
 * Enums
 */

typedef enum
{
	token_none = 0,
	token_attribute,
	token_bootloader,
	token_mts_preboot,
	token_mts,
	token_block_size,
	token_page_size,
	token_partition_size,
	token_dev_type,
	token_dev_param,
	token_redundancy,
	token_version,
	token_odm_data,
	token_bct_file,
	token_bct_copy,
	token_bct_size,
	token_nand_params,
	token_sdmmc_params,
	token_spiflash_params,
	token_data_width,
	token_clock_divider,
	token_clock_source,
	token_read_command_type_fast,
	token_max_power_class_supported,
	token_sd_controller,
	token_nand_timing2,
	token_nand_timing,
	token_block_size_log2,
	token_page_size_log2,
	token_nand_async_timing0,
	token_nand_async_timing1,
	token_nand_async_timing2,
	token_nand_async_timing3,
	token_nand_sddr_timing0,
	token_nand_sddr_timing1,
	token_nand_tddr_timing0,
	token_nand_tddr_timing1,
	token_nand_fbio_dqsib_dly_byte,
	token_nand_fbio_quse_dly_byte,
	token_nand_fbio_cfg_quse_late,
	token_async_timing0,
	token_async_timing1,
	token_async_timing2,
	token_async_timing3,
	token_sddr_timing0,
	token_sddr_timing1,
	token_tddr_timing0,
	token_tddr_timing1,
	token_fbio_dqsib_dly_byte,
	token_fbio_quse_dly_byte,
	token_fbio_cfg_quse_late,
	token_disable_sync_ddr,
	token_nand_disable_sync_ddr,
	token_sdram,
	token_crypto_hash,
	token_random_aes_blk,
	token_boot_data_version,
	token_bootloader_used,
	token_bootloaders_max,
	token_reserved,
	token_reserved_size,
	token_reserved_offset,
	token_hash_size,
	token_crypto_offset,
	token_crypto_length,
	token_max_bct_search_blks,
	token_num_param_sets,
	token_dev_type_nand,
	token_dev_type_sdmmc,
	token_dev_type_spi,
	token_num_sdram_sets,
	token_pre_bct_pad_blocks,
	token_unique_chip_id,
	token_secure_jtag_control,
	token_secure_debug_control,

	token_rsa_key_modulus,
	token_rsa_pss_sig_bl,
	token_rsa_pss_sig_bct,
	token_sign_bl,

	token_nand_clock_divider,
	token_nand_nand_timing,
	token_nand_nand_timing2,
	token_nand_block_size_log2,
	token_nand_page_size_log2,
	token_sdmmc_clock_divider,
	token_sdmmc_data_width,
	token_sdmmc_multi_page_support,
	token_sdmmc_sd_controller,
	token_sdmmc_max_power_class_supported,
	token_spiflash_read_command_type_fast,
	token_spiflash_page_size_2k_or_16k,
	token_spiflash_clock_source,
	token_spiflash_clock_divider,
	token_sdmmc_data_width_4bit,
	token_sdmmc_data_width_8bit,
	token_spi_clock_source_pllp_out0,
	token_spi_clock_source_pllc_out0,
	token_spi_clock_source_pllm_out0,
	token_spi_clock_source_clockm,
	token_memory_type_none,
	token_memory_type_ddr,
	token_memory_type_lpddr,
	token_memory_type_ddr2,
	token_memory_type_lpddr2,
	token_memory_type_ddr3,
	token_memory_type_lpddr4,

	token_bl_version,
	token_bl_start_blk,
	token_bl_start_page,
	token_bl_length,
	token_bl_load_addr,
	token_bl_entry_point,
	token_bl_attribute,
	token_bl_crypto_hash,

	token_memory_type,
	token_pllm_charge_pump_setup_ctrl,
	token_pllm_loop_filter_setup_ctrl,
	token_pllm_input_divider,
	token_pllm_feedback_divider,
	token_pllm_post_divider,
	token_pllm_stable_time,
	token_pllm_setup_control,
	token_pllm_select_div2,
	token_pllm_pdlshift_ph45,
	token_pllm_pdlshift_ph90,
	token_pllm_pdlshift_ph135,
	token_pllm_kcp,
	token_pllm_kvco,
	token_emc_bct_spare0,
	token_emc_bct_spare1,
	token_emc_bct_spare2,
	token_emc_bct_spare3,
	token_emc_bct_spare4,
	token_emc_bct_spare5,
	token_emc_bct_spare6,
	token_emc_bct_spare7,
	token_emc_bct_spare8,
	token_emc_bct_spare9,
	token_emc_bct_spare10,
	token_emc_bct_spare11,
	token_emc_bct_spare12,
	token_emc_bct_spare13,
	token_emc_clock_divider,
	token_emc_auto_cal_interval,
	token_emc_auto_cal_config,
	token_emc_auto_cal_config2,
	token_emc_auto_cal_config3,
	token_emc_auto_cal_wait,

	token_emc_xm2_comp_pad_ctrl,
	token_emc_xm2_comp_pad_ctrl2,
	token_emc_xm2_comp_pad_ctrl3,

	token_emc_pin_program_wait,
	token_emc_rc,
	token_emc_rfc,

	token_emc_rfc_pb,
	token_emc_ref_ctrl2,

	token_emc_rfc_slr,
	token_emc_ras,
	token_emc_rp,
	token_emc_r2r,
	token_emc_w2w,
	token_emc_r2w,
	token_emc_w2r,
	token_emc_r2p,
	token_emc_w2p,

	token_emc_tppd,
	token_emc_ccdmw,

	token_emc_rd_rcd,
	token_emc_wr_rcd,
	token_emc_rrd,
	token_emc_rext,
	token_emc_wdv,

	token_emc_wdv_chk,
	token_emc_wsv,
	token_emc_wev,

	token_emc_wdv_mask,

	token_emc_ws_duration,
	token_emc_we_duration,

	token_emc_quse,
	token_emc_quse_width,
	token_emc_ibdly,

	token_emc_obdly,

	token_emc_einput,
	token_emc_einput_duration,
	token_emc_puterm_extra,
	token_emc_puterm_width,
	token_emc_puterm_adj,
	token_emc_cdb_cntl1,
	token_emc_cdb_cntl2,
	token_emc_cdb_cntl3,
	token_emc_qrst,
	token_emc_qsafe,
	token_emc_rdv,
	token_emc_rdv_mask,

	token_emc_rdv_early,
	token_emc_rdv_early_mask,

	token_emc_qpop,
	token_emc_refresh,
	token_emc_burst_refresh_num,
	token_emc_pdex2wr,
	token_emc_pdex2rd,
	token_emc_pchg2pden,
	token_emc_act2pden,
	token_emc_ar2pden,
	token_emc_rw2pden,

	token_emc_cke2pden,
	token_emc_pdex2che,
	token_emc_pdex2mrr,

	token_emc_txsr,
	token_emc_tcke,
	token_emc_tckesr,
	token_emc_tpd,
	token_emc_tfaw,
	token_emc_trpab,
	token_emc_tclkstable,
	token_emc_tclkstop,
	token_emc_trefbw,
	token_emc_quse_extra,
	token_emc_fbio_cfg5,
	token_emc_fbio_cfg6,

	token_emc_fbio_cfg7,
	token_emc_fbio_cfg8,

	/* Command mapping for CMD brick 0 */
	token_emc_cmd_mapping_cmd0_0,
	token_emc_cmd_mapping_cmd0_1,
	token_emc_cmd_mapping_cmd0_2,
	token_emc_cmd_mapping_cmd1_0,
	token_emc_cmd_mapping_cmd1_1,
	token_emc_cmd_mapping_cmd1_2,
	token_emc_cmd_mapping_cmd2_0,
	token_emc_cmd_mapping_cmd2_1,
	token_emc_cmd_mapping_cmd2_2,
	token_emc_cmd_mapping_cmd3_0,
	token_emc_cmd_mapping_cmd3_1,
	token_emc_cmd_mapping_cmd3_2,
	token_emc_cmd_mapping_byte,

	token_emc_fbio_spare,
	token_emc_mrs,
	token_emc_emrs,
	token_emc_emrs2,
	token_emc_emrs3,
	token_emc_mrw1,
	token_emc_mrw2,
	token_emc_mrw3,
	token_emc_mrw4,

	/* Specifies the programming to LPDDR4 Mode Register 3 at cold boot */
	token_emc_mrw6,
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	token_emc_mrw8,
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	token_emc_mrw9,
	/* Specifies the programming to LPDDR4 Mode Register 12 at cold boot */
	token_emc_mrw10,
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	token_emc_mrw12,
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	token_emc_mrw13,
	/* Specifies the programming to LPDDR4 Mode Register 22 at cold boot */
	token_emc_mrw14,

	token_emc_mrw_reset_command,
	token_emc_mrw_reset_ninit_wait,
	token_emc_adr_cfg,
	token_mc_emem_cfg,
	token_emc_cfg,
	token_emc_cfg2,
	token_emc_cfg_pipe,

	token_emc_cfg_pipe_clk,
	token_emc_fdpd_ctrl_cmd_no_ramp,
	token_emc_cfg_update,

	token_emc_dbg,

	token_emc_dbg_write_mux,

	token_emc_cfg_dig_dll,
	token_emc_cfg_dig_dll_1,
	token_emc_cfg_dig_dll_period,
	token_warm_boot_wait,
	token_emc_ctt_term_ctrl,
	token_emc_odt_write,
	token_emc_odt_read,
	token_emc_zcal_ref_cnt,
	token_emc_zcal_wait_cnt,
	token_emc_zcal_mrw_cmd,
	token_emc_mrs_reset_dll,
	token_emc_mrs_reset_dll_wait,
	token_emc_emrs_emr2,
	token_emc_emrs_emr3,
	token_emc_emrs_ddr2_dll_enable,
	token_emc_mrs_ddr2_dll_reset,
	token_emc_emrs_ddr2_ocd_calib,
	token_emc_ddr2_wait,
	token_pmc_ddr_pwr,

	token_emc_fbio_cfg1,
	token_emc_fbio_dqsib_dly,
	token_emc_fbio_dqsib_dly_msb,
	token_emc_fbio_quse_dly,
	token_emc_fbio_quse_dly_msb,
	token_emc_adr_cfg1,
	token_mc_lowlatency_config,
	token_emc_cfg_clktrim0,
	token_emc_cfg_clktrim1,
	token_emc_cfg_clktrim2,
	token_ahb_arbitration_xbar_ctrl,
	token_emc_dll_xform_dqs,
	token_emc_dll_xform_quse,
	token_emc_mrw_zq_init_dev0,
	token_emc_mrw_zq_init_dev1,
	token_emc_mrw_zq_init_wait,
	token_apb_misc_gp_xm2cfga_pad_ctrl,
	token_apb_misc_gp_xm2cfgc_pad_ctrl,
	token_apb_misc_gp_xm2cfgc_pad_ctrl2,
	token_apb_misc_gp_xm2cfgd_pad_ctrl,
	token_apb_misc_gp_xm2cfgd_pad_ctrl2,
	token_apb_misc_gp_xm2clkcfg_Pad_ctrl,
	token_apb_misc_gp_xm2comp_pad_ctrl,
	token_apb_misc_gp_xm2vttgen_pad_ctrl,

	token_emc_clock_source,

	token_emc_clock_source_dll,
	token_clk_rst_pllm_misc20_override,
	token_clk_rst_pllm_misc20_override_enable,
	token_clear_clock2_mc1,

	token_emc_clock_use_pll_mud,
	token_emc_pin_extra_wait,

	token_emc_pin_gpio_enable,
	token_emc_pin_gpio,

	token_emc_timing_control_wait,
	token_emc_wext,
	token_emc_ctt,
	token_emc_ctt_duration,
	token_emc_prerefresh_req_cnt,
	token_emc_txsr_dll,
	token_emc_cfg_rsv,
	token_emc_mrw_extra,
	token_emc_warm_boot_mrw1,
	token_emc_warm_boot_mrw2,
	token_emc_warm_boot_mrw3,
	token_emc_warm_boot_mrw_extra,
	token_emc_warm_boot_extramode_reg_write_enable,
	token_emc_extramode_reg_write_enable,
	token_emc_mrs_wait_cnt,
	token_emc_mrs_wait_cnt2,
	token_emc_cmd_q,
	token_emc_mc2emc_q,
	token_emc_dyn_self_ref_control,
	token_ahb_arbitration_xbar_ctrl_meminit_done,
	token_emc_dev_select,
	token_emc_sel_dpd_ctrl,

	/* Pads trimmer delays */
	token_emc_fdpd_ctrl_dq,
	token_emc_fdpd_ctrl_cmd,
	token_emc_pmacro_ib_vref_dq_0,
	token_emc_pmacro_ib_vref_dq_1,
	token_emc_pmacro_ib_vref_dqs_0,
	token_emc_pmacro_ib_vref_dqs_1,
	token_emc_pmacro_ib_rxrt,
	token_emc_cfg_pipe1,
	token_emc_cfg_pipe2,

	/* Specifies the value for EMC_PMACRO_QUSE_DDLL_RANK0_0 */
	token_emc_pmacro_quse_ddll_rank0_0,
	token_emc_pmacro_quse_ddll_rank0_1,
	token_emc_pmacro_quse_ddll_rank0_2,
	token_emc_pmacro_quse_ddll_rank0_3,
	token_emc_pmacro_quse_ddll_rank0_4,
	token_emc_pmacro_quse_ddll_rank0_5,
	token_emc_pmacro_quse_ddll_rank1_0,
	token_emc_pmacro_quse_ddll_rank1_1,
	token_emc_pmacro_quse_ddll_rank1_2,
	token_emc_pmacro_quse_ddll_rank1_3,
	token_emc_pmacro_quse_ddll_rank1_4,
	token_emc_pmacro_quse_ddll_rank1_5,

	token_emc_pmacro_ob_ddll_long_dq_rank0_0,
	token_emc_pmacro_ob_ddll_long_dq_rank0_1,
	token_emc_pmacro_ob_ddll_long_dq_rank0_2,
	token_emc_pmacro_ob_ddll_long_dq_rank0_3,
	token_emc_pmacro_ob_ddll_long_dq_rank0_4,
	token_emc_pmacro_ob_ddll_long_dq_rank0_5,
	token_emc_pmacro_ob_ddll_long_dq_rank1_0,
	token_emc_pmacro_ob_ddll_long_dq_rank1_1,
	token_emc_pmacro_ob_ddll_long_dq_rank1_2,
	token_emc_pmacro_ob_ddll_long_dq_rank1_3,
	token_emc_pmacro_ob_ddll_long_dq_rank1_4,
	token_emc_pmacro_ob_ddll_long_dq_rank1_5,

	token_emc_pmacro_ob_ddll_long_dqs_rank0_0,
	token_emc_pmacro_ob_ddll_long_dqs_rank0_1,
	token_emc_pmacro_ob_ddll_long_dqs_rank0_2,
	token_emc_pmacro_ob_ddll_long_dqs_rank0_3,
	token_emc_pmacro_ob_ddll_long_dqs_rank0_4,
	token_emc_pmacro_ob_ddll_long_dqs_rank0_5,
	token_emc_pmacro_ob_ddll_long_dqs_rank1_0,
	token_emc_pmacro_ob_ddll_long_dqs_rank1_1,
	token_emc_pmacro_ob_ddll_long_dqs_rank1_2,
	token_emc_pmacro_ob_ddll_long_dqs_rank1_3,
	token_emc_pmacro_ob_ddll_long_dqs_rank1_4,
	token_emc_pmacro_ob_ddll_long_dqs_rank1_5,

	token_emc_pmacro_ib_ddll_long_dqs_rank0_0,
	token_emc_pmacro_ib_ddll_long_dqs_rank0_1,
	token_emc_pmacro_ib_ddll_long_dqs_rank0_2,
	token_emc_pmacro_ib_ddll_long_dqs_rank0_3,
	token_emc_pmacro_ib_ddll_long_dqs_rank1_0,
	token_emc_pmacro_ib_ddll_long_dqs_rank1_1,
	token_emc_pmacro_ib_ddll_long_dqs_rank1_2,
	token_emc_pmacro_ib_ddll_long_dqs_rank1_3,

	token_emc_pmacro_ddll_long_cmd_0,
	token_emc_pmacro_ddll_long_cmd_1,
	token_emc_pmacro_ddll_long_cmd_2,
	token_emc_pmacro_ddll_long_cmd_3,
	token_emc_pmacro_ddll_long_cmd_4,
	token_emc_pmacro_ddll_short_cmd_0,
	token_emc_pmacro_ddll_short_cmd_1,
	token_emc_pmacro_ddll_short_cmd_2,

	token_emc_dll_xform_dqs0,
	token_emc_dll_xform_dqs1,
	token_emc_dll_xform_dqs2,
	token_emc_dll_xform_dqs3,
	token_emc_dll_xform_dqs4,
	token_emc_dll_xform_dqs5,
	token_emc_dll_xform_dqs6,
	token_emc_dll_xform_dqs7,
	token_emc_dll_xform_dqs8,
	token_emc_dll_xform_dqs9,
	token_emc_dll_xform_dqs10,
	token_emc_dll_xform_dqs11,
	token_emc_dll_xform_dqs12,
	token_emc_dll_xform_dqs13,
	token_emc_dll_xform_dqs14,
	token_emc_dll_xform_dqs15,
	token_emc_dll_xform_quse0,
	token_emc_dll_xform_quse1,
	token_emc_dll_xform_quse2,
	token_emc_dll_xform_quse3,
	token_emc_dll_xform_quse4,
	token_emc_dll_xform_quse5,
	token_emc_dll_xform_quse6,
	token_emc_dll_xform_quse7,
	token_emc_dll_xform_addr0,
	token_emc_dll_xform_addr1,
	token_emc_dll_xform_addr2,
	token_emc_dll_xform_addr3,
	token_emc_dll_xform_addr4,
	token_emc_dll_xform_addr5,
	token_emc_dll_xform_quse8,
	token_emc_dll_xform_quse9,
	token_emc_dll_xform_quse10,
	token_emc_dll_xform_quse11,
	token_emc_dll_xform_quse12,
	token_emc_dll_xform_quse13,
	token_emc_dll_xform_quse14,
	token_emc_dll_xform_quse15,
	token_emc_dli_trim_tx_dqs0,
	token_emc_dli_trim_tx_dqs1,
	token_emc_dli_trim_tx_dqs2,
	token_emc_dli_trim_tx_dqs3,
	token_emc_dli_trim_tx_dqs4,
	token_emc_dli_trim_tx_dqs5,
	token_emc_dli_trim_tx_dqs6,
	token_emc_dli_trim_tx_dqs7,
	token_emc_dli_trim_tx_dqs8,
	token_emc_dli_trim_tx_dqs9,
	token_emc_dli_trim_tx_dqs10,
	token_emc_dli_trim_tx_dqs11,
	token_emc_dli_trim_tx_dqs12,
	token_emc_dli_trim_tx_dqs13,
	token_emc_dli_trim_tx_dqs14,
	token_emc_dli_trim_tx_dqs15,
	token_emc_dll_xform_dq0,
	token_emc_dll_xform_dq1,
	token_emc_dll_xform_dq2,
	token_emc_dll_xform_dq3,
	token_emc_dll_xform_dq4,
	token_emc_dll_xform_dq5,
	token_emc_dll_xform_dq6,
	token_emc_dll_xform_dq7,
	token_emc_zcal_interval,
	token_emc_zcal_init_dev0,
	token_emc_zcal_init_dev1,
	token_emc_zcal_init_wait,
	token_emc_zcal_cold_boot_enable,
	token_emc_zcal_warm_boot_enable,
	token_emc_zcal_warm_cold_boot_enables,
	token_emc_mrw_lpddr2zcal_warm_boot,
	token_emc_zqcal_ddr3_warm_boot,
	token_emc_zqcal_lpddr4_warm_boot,
	token_emc_zcal_warm_boot_wait,
	token_emc_mrs_warm_boot_enable,
	token_emc_mrs_extra,
	token_emc_warm_boot_mrs,
	token_emc_warm_boot_emrs,
	token_emc_warm_boot_emr2,
	token_emc_warm_boot_emr3,
	token_emc_warm_boot_mrs_extra,
	token_emc_clken_override,
	token_mc_dis_extra_snap_levels,
	token_emc_extra_refresh_num,
	token_emc_clken_override_allwarm_boot,
	token_mc_clken_override_allwarm_boot,
	token_emc_cfg_dig_dll_period_warm_boot,
	token_pmc_vddp_sel,
	token_pmc_vddp_sel_wait,
	token_pmc_ddr_cfg,
	token_pmc_io_dpd_req,
	token_pmc_io_dpd2_req,
	token_pmc_io_dpd3_req,
	token_pmc_io_dpd3_req_wait,
	token_pmc_io_dpd4_req_wait,
	token_pmc_reg_short,
	token_pmc_eno_vtt_gen,
	token_pmc_no_io_power,

	token_pmc_ddr_ctrl_wait,
	token_pmc_ddr_ctrl,

	token_pmc_por_dpd_ctrl_wait,
	token_emc_xm2cmd_pad_ctrl,
	token_emc_xm2cmd_pad_ctrl2,
	token_emc_xm2cmd_pad_ctrl3,
	token_emc_xm2cmd_pad_ctrl4,
	token_emc_xm2cmd_pad_ctrl5,
	token_emc_xm2dqs_pad_ctrl,
	token_emc_xm2dqs_pad_ctrl2,
	token_emc_xm2dqs_pad_ctrl3,
	token_emc_xm2dqs_pad_ctrl4,
	token_emc_xm2dqs_pad_ctrl5,
	token_emc_xm2dqs_pad_ctrl6,
	token_emc_xm2dq_pad_ctrl,
	token_emc_xm2dq_pad_ctrl2,
	token_emc_xm2dq_pad_ctrl3,
	token_emc_xm2clk_pad_ctrl,
	token_emc_xm2clk_pad_ctrl2,
	token_emc_xm2comp_pad_ctrl,
	token_emc_xm2vttgen_pad_ctrl,
	token_emc_xm2vttgen_pad_ctrl2,
	token_emc_xm2vttgen_pad_ctrl3,
	token_emc_xm2quse_pad_ctrl,
	token_emc_acpd_control,
	token_emc_swizzle_rank0_byte_cfg,
	token_emc_swizzle_rank0_byte0,
	token_emc_swizzle_rank0_byte1,
	token_emc_swizzle_rank0_byte2,
	token_emc_swizzle_rank0_byte3,
	token_emc_swizzle_rank1_byte_cfg,
	token_emc_swizzle_rank1_byte0,
	token_emc_swizzle_rank1_byte1,
	token_emc_swizzle_rank1_byte2,
	token_emc_swizzle_rank1_byte3,
	token_emc_addr_swizzle_stack1a,
	token_emc_addr_swizzle_stack1b,
	token_emc_addr_swizzle_stack2a,
	token_emc_addr_swizzle_stack2b,
	token_emc_addr_swizzle_stack3,
	token_emc_dsr_vttgen_drv,
	token_emc_txdsrvttgen,

	/* Specifies the value for EMC_DATA_BRLSHFT_0 */
	token_emc_data_brlshft0,
	token_emc_data_brlshft1,

	token_emc_dqs_brlshft0,
	token_emc_dqs_brlshft1,

	token_emc_cmd_brlshft0,
	token_emc_cmd_brlshft1,
	token_emc_cmd_brlshft2,
	token_emc_cmd_brlshft3,

	token_emc_quse_brlshft0,
	token_emc_quse_brlshft1,
	token_emc_quse_brlshft2,
	token_emc_quse_brlshft3,

	token_emc_dll_cfg0,
	token_emc_dll_cfg1,

	token_emc_pmc_scratch1,
	token_emc_pmc_scratch2,
	token_emc_pmc_scratch3,

	token_emc_pmacro_pad_cfg_ctrl,

	token_emc_pmacro_vttgen_ctrl0,
	token_emc_pmacro_vttgen_ctrl1,
	token_emc_pmacro_vttgen_ctrl2,

	token_emc_pmacro_brick_ctrl_rfu1,
	token_emc_pmacro_cmd_brick_ctrl_fdpd,
	token_emc_pmacro_brick_ctrl_rfu2,
	token_emc_pmacro_data_brick_ctrl_fdpd,
	token_emc_pmacro_bg_bias_ctrl0,
	token_emc_pmacro_data_pad_rx_ctrl,
	token_emc_pmacro_cmd_pad_rx_ctrl,
	token_emc_pmacro_data_rx_term_mode,
	token_emc_pmacro_cmd_rx_term_mode,
	token_emc_pmacro_data_pad_tx_ctrl,
	token_emc_pmacro_common_pad_tx_ctrl,
	token_emc_pmacro_cmd_pad_tx_ctrl,
	token_emc_cfg3,

	token_emc_pmacro_tx_pwrd0,
	token_emc_pmacro_tx_pwrd1,
	token_emc_pmacro_tx_pwrd2,
	token_emc_pmacro_tx_pwrd3,
	token_emc_pmacro_tx_pwrd4,
	token_emc_pmacro_tx_pwrd5,

	token_emc_config_sample_delay,

	token_emc_pmacro_brick_mapping0,
	token_emc_pmacro_brick_mapping1,
	token_emc_pmacro_brick_mapping2,

	token_emc_pmacro_tx_sel_clk_src0,
	token_emc_pmacro_tx_sel_clk_src1,
	token_emc_pmacro_tx_sel_clk_src2,
	token_emc_pmacro_tx_sel_clk_src3,
	token_emc_pmacro_tx_sel_clk_src4,
	token_emc_pmacro_tx_sel_clk_src5,

	token_emc_pmacro_ddll_bypass,

	token_emc_pmacro_ddll_pwrd0,
	token_emc_pmacro_ddll_pwrd1,
	token_emc_pmacro_ddll_pwrd2,

	token_emc_pmacro_cmd_ctrl0,
	token_emc_pmacro_cmd_ctrl1,
	token_emc_pmacro_cmd_ctrl2,

	token_emc_bgbias_ctl0,
	token_mc_emem_adr_cfg,
	token_mc_emem_adr_cfg_dev0,
	token_mc_emem_adr_cfg_dev1,
	token_mc_emem_adr_cfg_channel_mask,
	token_mc_emem_adr_cfg_channel_mask_propagation_count,
	token_mc_emem_adr_cfg_bank_mask0,
	token_mc_emem_adr_cfg_bank_mask1,
	token_mc_emem_adr_cfg_bank_mask2,
	token_mc_emem_adr_cfg_bank_swizzle3,
	token_mc_emem_arb_cfg,
	token_mc_emem_arb_outstanding_req,

	token_emc_emem_arb_refpb_hp_ctrl,
	token_emc_emem_arb_refpb_bank_ctrl,

	token_mc_emem_arb_timing_rcd,
	token_mc_emem_arb_timing_rp,
	token_mc_emem_arb_timing_rc,
	token_mc_emem_arb_timing_ras,
	token_mc_emem_arb_timing_faw,
	token_mc_emem_arb_timing_rrd,
	token_mc_emem_arb_timing_rap2pre,
	token_mc_emem_arb_timing_wap2pre,
	token_mc_emem_arb_timing_r2r,
	token_mc_emem_arb_timing_w2w,
	token_mc_emem_arb_timing_r2w,
	token_mc_emem_arb_timing_w2r,

	token_mc_emem_arb_timing_rfcpb,

	token_mc_emem_arb_da_turns,
	token_mc_emem_arb_da_covers,
	token_mc_emem_arb_misc0,
	token_mc_emem_arb_misc1,
	token_mc_emem_arb_misc2,

	token_mc_emem_arb_ring1_throttle,
	token_mc_emem_arb_override,
	token_mc_emem_arb_override1,
	token_mc_emem_arb_rsv,

	token_mc_da_cfg0,
	token_mc_emem_arb_timing_ccdmw,

	token_mc_clken_override,
	token_mc_emc_reg_mode,
	token_mc_stat_control,
	token_mc_display_snap_ring,
	token_mc_video_protect_bom,
	token_mc_video_protect_bom_adr_hi,
	token_mc_video_protect_size_mb,
	token_mc_video_protect_vpr_override,
	token_mc_video_protect_vpr_override1,
	token_mc_video_protect_gpu_override0,
	token_mc_video_protect_gpu_override1,
	token_mc_sec_carveout_bom,
	token_mc_sec_carveout_adr_hi,
	token_mc_sec_carveout_size_mb,
	token_mc_video_protect_write_access,
	token_mc_sec_carveout_protect_write_access,

	token_mc_generalized_carveout1_bom,
	token_mc_generalized_carveout1_bom_hi,
	token_mc_generalized_carveout1_size_128kb,
	token_mc_generalized_carveout1_access0,
	token_mc_generalized_carveout1_access1,
	token_mc_generalized_carveout1_access2,
	token_mc_generalized_carveout1_access3,
	token_mc_generalized_carveout1_access4,
	token_mc_generalized_carveout1_force_internal_access0,
	token_mc_generalized_carveout1_force_internal_access1,
	token_mc_generalized_carveout1_force_internal_access2,
	token_mc_generalized_carveout1_force_internal_access3,
	token_mc_generalized_carveout1_force_internal_access4,
	token_mc_generalized_carveout1_cfg0,

	token_mc_generalized_carveout2_bom,
	token_mc_generalized_carveout2_bom_hi,
	token_mc_generalized_carveout2_size_128kb,
	token_mc_generalized_carveout2_access0,
	token_mc_generalized_carveout2_access1,
	token_mc_generalized_carveout2_access2,
	token_mc_generalized_carveout2_access3,
	token_mc_generalized_carveout2_access4,
	token_mc_generalized_carveout2_force_internal_access0,
	token_mc_generalized_carveout2_force_internal_access1,
	token_mc_generalized_carveout2_force_internal_access2,
	token_mc_generalized_carveout2_force_internal_access3,
	token_mc_generalized_carveout2_force_internal_access4,
	token_mc_generalized_carveout2_cfg0,

	token_mc_generalized_carveout3_bom,
	token_mc_generalized_carveout3_bom_hi,
	token_mc_generalized_carveout3_size_128kb,
	token_mc_generalized_carveout3_access0,
	token_mc_generalized_carveout3_access1,
	token_mc_generalized_carveout3_access2,
	token_mc_generalized_carveout3_access3,
	token_mc_generalized_carveout3_access4,
	token_mc_generalized_carveout3_force_internal_access0,
	token_mc_generalized_carveout3_force_internal_access1,
	token_mc_generalized_carveout3_force_internal_access2,
	token_mc_generalized_carveout3_force_internal_access3,
	token_mc_generalized_carveout3_force_internal_access4,
	token_mc_generalized_carveout3_cfg0,

	token_mc_generalized_carveout4_bom,
	token_mc_generalized_carveout4_bom_hi,
	token_mc_generalized_carveout4_size_128kb,
	token_mc_generalized_carveout4_access0,
	token_mc_generalized_carveout4_access1,
	token_mc_generalized_carveout4_access2,
	token_mc_generalized_carveout4_access3,
	token_mc_generalized_carveout4_access4,
	token_mc_generalized_carveout4_force_internal_access0,
	token_mc_generalized_carveout4_force_internal_access1,
	token_mc_generalized_carveout4_force_internal_access2,
	token_mc_generalized_carveout4_force_internal_access3,
	token_mc_generalized_carveout4_force_internal_access4,
	token_mc_generalized_carveout4_cfg0,

	token_mc_generalized_carveout5_bom,
	token_mc_generalized_carveout5_bom_hi,
	token_mc_generalized_carveout5_size_128kb,
	token_mc_generalized_carveout5_access0,
	token_mc_generalized_carveout5_access1,
	token_mc_generalized_carveout5_access2,
	token_mc_generalized_carveout5_access3,
	token_mc_generalized_carveout5_access4,
	token_mc_generalized_carveout5_force_internal_access0,
	token_mc_generalized_carveout5_force_internal_access1,
	token_mc_generalized_carveout5_force_internal_access2,
	token_mc_generalized_carveout5_force_internal_access3,
	token_mc_generalized_carveout5_force_internal_access4,
	token_mc_generalized_carveout5_cfg0,

	token_emc_ca_training_enable,
	token_emc_ca_training_timing_cntl1,
	token_emc_ca_training_timing_cntl2,
	token_swizzle_rank_byte_encode,
	token_boot_rom_patch_control,
	token_boot_rom_patch_data,
	token_ch1_emc_dll_xform_dqs0,
	token_ch1_emc_dll_xform_dqs1,
	token_ch1_emc_dll_xform_dqs2,
	token_ch1_emc_dll_xform_dqs3,
	token_ch1_emc_dll_xform_dqs4,
	token_ch1_emc_dll_xform_dqs5,
	token_ch1_emc_dll_xform_dqs6,
	token_ch1_emc_dll_xform_dqs7,
	token_ch1_emc_dll_xform_quse0,
	token_ch1_emc_dll_xform_quse1,
	token_ch1_emc_dll_xform_quse2,
	token_ch1_emc_dll_xform_quse3,
	token_ch1_emc_dll_xform_quse4,
	token_ch1_emc_dll_xform_quse5,
	token_ch1_emc_dll_xform_quse6,
	token_ch1_emc_dll_xform_quse7,
	token_ch1_emc_dli_trim_tx_dqs0,
	token_ch1_emc_dli_trim_tx_dqs1,
	token_ch1_emc_dli_trim_tx_dqs2,
	token_ch1_emc_dli_trim_tx_dqs3,
	token_ch1_emc_dli_trim_tx_dqs4,
	token_ch1_emc_dli_trim_tx_dqs5,
	token_ch1_emc_dli_trim_tx_dqs6,
	token_ch1_emc_dli_trim_tx_dqs7,
	token_ch1_emc_dll_xform_dq0,
	token_ch1_emc_dll_xform_dq1,
	token_ch1_emc_dll_xform_dq2,
	token_ch1_emc_dll_xform_dq3,
	token_ch1_emc_swizzle_rank0_byte_cfg,
	token_ch1_emc_swizzle_rank0_byte0,
	token_ch1_emc_swizzle_rank0_byte1,
	token_ch1_emc_swizzle_rank0_byte2,
	token_ch1_emc_swizzle_rank0_byte3,
	token_ch1_emc_swizzle_rank1_byte_cfg,
	token_ch1_emc_swizzle_rank1_byte0,
	token_ch1_emc_swizzle_rank1_byte1,
	token_ch1_emc_swizzle_rank1_byte2,
	token_ch1_emc_swizzle_rank1_byte3,
	token_ch1_emc_addr_swizzle_stack1a,
	token_ch1_emc_addr_swizzle_stack1b,
	token_ch1_emc_addr_swizzle_stack2a,
	token_ch1_emc_addr_swizzle_stack2b,
	token_ch1_emc_addr_swizzle_stack3,
	token_ch1_emc_auto_cal_config,
	token_ch1_emc_auto_cal_config2,
	token_ch1_emc_auto_cal_config3,

	token_emc_auto_cal_config4,
	token_emc_auto_cal_config5,
	token_emc_auto_cal_config6,
	token_emc_auto_cal_config7,
	token_emc_auto_cal_config8,
	/* Specifies the value for EMC_AUTO_CAL_VREF_SEL_0 */
	token_emc_auto_cal_vref_sel0,
	token_emc_auto_cal_vref_sel1,

	/* Specifies the value for EMC_AUTO_CAL_CHANNEL */
	token_emc_auto_cal_channel,

	/* Specifies the value for EMC_PMACRO_AUTOCAL_CFG_0 */
	token_emc_pmacro_auto_cal_cfg0,
	token_emc_pmacro_auto_cal_cfg1,
	token_emc_pmacro_auto_cal_cfg2,

	token_emc_pmacro_rx_term,
	token_emc_pmacro_dq_tx_drive,
	token_emc_pmacro_ca_tx_drive,
	token_emc_pmacro_cmd_tx_drive,
	token_emc_pmacro_auto_cal_common,
	token_emc_pmacro_zcrtl,

	token_ch1_emc_cdb_cntl1,
	token_ch1_emc_dll_xform_addr0,
	token_ch1_emc_dll_xform_addr1,
	token_ch1_emc_dll_xform_addr2,
	token_ch1_emc_fbio_spare,
	token_ch1_emc_xm2_clk_pad_ctrl,
	token_ch1_emc_xm2_clk_pad_ctrl2,
	token_ch1_emc_xm2_cmd_pad_ctrl2,
	token_ch1_emc_xm2_cmd_pad_ctrl3,
	token_ch1_emc_xm2_cmd_pad_ctrl4,
	token_ch1_emc_xm2_dq_pad_ctrl,
	token_ch1_emc_xm2_dq_pad_ctrl2,
	token_ch1_emc_xm2_dqs_pad_ctrl,
	token_ch1_emc_xm2_dqs_pad_ctrl3,
	token_ch1_emc_xm2_dqs_pad_ctrl4,
	token_mc_mts_carveout_bom,
	token_mc_mts_carveout_adr_hi,
	token_mc_mts_carveout_size_mb,
	token_mc_mts_carveout_reg_ctrl,

	token_mts_info_version,
	token_mts_info_start_blk,
	token_mts_info_start_page,
	token_mts_info_length,
	token_mts_info_load_addr,
	token_mts_info_entry_point,
	token_mts_info_attribute,

	token_mts_used,
	token_mts_max,

	token_force32 = 0x7fffffff
} parse_token;

typedef enum
{
	field_type_none = 0,
	field_type_enum,
	field_type_u32,
	field_type_u8,
	field_type_force32 = 0x7fffffff
} field_type;

/* Forward declarations */
typedef int (*process_function)(build_image_context *context,
				parse_token token,
				char *remainder);

typedef int (*process_subfield_function)(build_image_context *context,
					uint32_t index,
					parse_token token,
					uint32_t value);


typedef struct
{
	char *name;
	uint32_t value;
} enum_item;

typedef struct
{
	char *name;
	uint32_t token;
	field_type type;
	enum_item *enum_table;
} field_item;


typedef struct
{
	char *prefix;
	parse_token token;
	field_item *field_table;
	process_subfield_function process;
} parse_subfield_item;

typedef struct
{
	char *prefix;
	parse_token token;
	process_function process;
} parse_item;

/*
 * Set of function pointers and table pointers to be used to access the different hardware
 * interface for setting/getting bct information.
 */
typedef struct cbootimage_soc_config_rec {
	/*
	 * Set device parameters in bct according to the value listed
	 *
	 * @param context	The main context pointer
	 * @param index  	The device index in bct field
	 * @param token  	The parse token value
	 * @param value  	Value to set
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*set_dev_param)(build_image_context *context,
			uint32_t index,
			parse_token token,
			uint32_t value);
	/*
	 * Get the specified device parameters from bct data stored
	 * in context.
	 *
	 * @param context	The main context pointer
	 * @param index  	The device index in bct field
	 * @param token  	The parse token value
	 * @param value  	Return value get from bct field
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*get_dev_param)(build_image_context *context,
			uint32_t index,
			parse_token token,
			uint32_t *value);
	/*
	 * Set sdram parameters in bct according to the value listed
	 * in config file.
	 *
	 * @param context	The main context pointer
	 * @param index  	The sdram index in bct field
	 * @param token  	The parse token value
	 * @param value  	Value to set
	 * @return 0 and 1 for success and failure
	 */
	int (*set_sdram_param)(build_image_context *context,
			uint32_t index,
			parse_token token,
			uint32_t value);
	/*
	 * Get the specified sdram parameters from bct data stored
	 * in context.
	 *
	 * @param context	The main context pointer
	 * @param index  	The sdram index in bct field
	 * @param token  	The parse token value
	 * @param value  	Return value get from bct field
	 * @return 0 and 1 for success and failure
	 */
	int (*get_sdram_param)(build_image_context *context,
			uint32_t index,
			parse_token token,
			uint32_t *value);
	/*
	 * Set bootloader parameters in bct according to the value listed
	 * in config file.
	 *
	 * @param set 	Bootloader index
	 * @param id  	The parse token value
	 * @param data	Value to set
	 * @param bct 	Bct pointer
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*setbl_param)(uint32_t set,
			parse_token id,
			uint32_t *data,
			uint8_t *bct);
	/*
	 * Get the specified bootloader parameters from bct data stored
	 * in context.
	 *
	 * @param set 	Bootloader index
	 * @param id  	The parse token value
	 * @param data	Return value get from bct data
	 * @param bct 	Bct pointer
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*getbl_param)(uint32_t set,
			parse_token id,
			uint32_t *data,
			uint8_t *bct);
	/*
	 * Set the specified bct value stored in context bct data structure.
	 *
	 * @param id  	The parse token value
	 * @param data	Pointer of value to set
	 * @param bct 	Bct pointer
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*set_value)(parse_token id,
			void *data,
			uint8_t *bct);
	/*
	 * Get the specified bct value or some constant value of clocks and
	 * hw type.
	 *
	 * @param id  	The parse token value
	 * @param data	Return value get from bct data
	 * @param bct 	Bct pointer
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*get_value)(parse_token id,
			void *data,
			uint8_t *bct);
	/*
	 * Get the size of specified bct field
	 *
	 * @param id  	The parse token
	 * @return size or 0/-ENODATA for failure
	 */
	int (*get_value_size)(parse_token id);

	/*
	 * Set the bct crypto hash data.
	 *
	 * @param id    	The parse token value
	 * @param data  	Value to set
	 * @param length	Length of data to set
	 * @param bct   	Bct pointer
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*set_data)(parse_token id,
			uint8_t *data,
			uint32_t  length,
			uint8_t *bct);

	/*
	 * Get the BCT structure size
	 *
	 * @return BCT size
	 */
	int (*get_bct_size)();

	/*
	 * Set MTS infomation in bct according to the value listed
	 * in config file.
	 *
	 * @param context	The main context pointer
	 * @param index  	The mts_info index in bct field
	 * @param token  	The parse token value
	 * @param value  	Value to set
	 * @return 0 and 1 for success and failure
	 */
	int (*set_mts_info)(build_image_context *context,
			uint32_t index,
			parse_token token,
			uint32_t value);
	/*
	 * Get the specified MTS information from bct data stored
	 * in context.
	 *
	 * @param context	The main context pointer
	 * @param index  	The mts_info index in bct field
	 * @param token  	The parse token value
	 * @param value  	Return value get from bct field
	 * @return 0 and 1 for success and failure
	 */
	int (*get_mts_info)(build_image_context *context,
			uint32_t index,
			parse_token token,
			uint32_t *value);

	/*
	 * Check if the token is supported to dump
	 *
	 * @param id  	The parse token value
	 * @return 0 and 1 for unsupported and supported
	 */
	int (*token_supported)(parse_token id);

	void (*init_bad_block_table)(build_image_context *context);

	enum_item *devtype_table;
	enum_item *sdmmc_data_width_table;
	enum_item *spi_clock_source_table;
	enum_item *nvboot_memory_type_table;
	field_item *sdram_field_table;
	field_item *nand_table;
	field_item *sdmmc_table;
	field_item *spiflash_table;
	parse_subfield_item *device_type_table;
} cbootimage_soc_config;

void process_config_file(build_image_context *context, uint8_t simple_parse);

void t210_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
void t132_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
void t124_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
void t114_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
void t30_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
void t20_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);

int if_bct_is_t210_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
int if_bct_is_t132_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
int if_bct_is_t124_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
int if_bct_is_t114_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
int if_bct_is_t30_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
int if_bct_is_t20_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);

int
t132_get_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t132_set_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);
int
t132_get_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t132_set_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);

int
t210_get_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t210_set_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);
int
t210_get_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t210_set_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);

int
t124_get_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t124_set_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);
int
t124_get_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t124_set_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);
int
t114_get_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t114_set_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);
int
t114_get_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t114_set_sdram_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);
int
t30_get_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t30_set_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);
int
t30_get_sdram_param(build_image_context *context,
		uint32_t index,
		parse_token token,
		uint32_t *value);
int
t30_set_sdram_param(build_image_context *context,
		uint32_t index,
		parse_token token,
		uint32_t value);
int
t20_get_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t *value);
int
t20_set_dev_param(build_image_context *context,
	uint32_t index,
	parse_token token,
	uint32_t value);

int
t20_get_sdram_param(build_image_context *context,
		uint32_t index,
		parse_token token,
		uint32_t *value);
int
t20_set_sdram_param(build_image_context *context,
		uint32_t index,
		parse_token token,
		uint32_t value);

uint32_t iceil_log2(uint32_t a, uint32_t b);

/* Returns the smallest power of 2 >= a */
uint32_t ceil_log2(uint32_t a);

extern cbootimage_soc_config *g_soc_config;

/*
 * Dummy function for unsupported token
 */
int bct_get_unsupported(parse_token id);

/*
 * Provide access to enum and field tables.  These tables are useful when
 * pretty printing a BCT file using bct_dump.
 */

extern enum_item s_devtype_table_t20[];
extern enum_item s_devtype_table_t30[];
extern enum_item s_devtype_table_t114[];
extern enum_item s_devtype_table_t124[];
extern enum_item s_devtype_table_t132[];
extern enum_item s_devtype_table_t210[];

extern enum_item s_sdmmc_data_width_table_t20[];
extern enum_item s_sdmmc_data_width_table_t30[];
extern enum_item s_sdmmc_data_width_table_t114[];
extern enum_item s_sdmmc_data_width_table_t124[];
extern enum_item s_sdmmc_data_width_table_t132[];
extern enum_item s_sdmmc_data_width_table_t210[];

extern enum_item s_spi_clock_source_table_t20[];
extern enum_item s_spi_clock_source_table_t30[];
extern enum_item s_spi_clock_source_table_t114[];
extern enum_item s_spi_clock_source_table_t124[];
extern enum_item s_spi_clock_source_table_t132[];
extern enum_item s_spi_clock_source_table_t210[];

extern enum_item s_nvboot_memory_type_table_t20[];
extern enum_item s_nvboot_memory_type_table_t30[];
extern enum_item s_nvboot_memory_type_table_t114[];
extern enum_item s_nvboot_memory_type_table_t124[];
extern enum_item s_nvboot_memory_type_table_t132[];
extern enum_item s_nvboot_memory_type_table_t210[];

extern field_item s_sdram_field_table_t20[];
extern field_item s_sdram_field_table_t30[];
extern field_item s_sdram_field_table_t114[];
extern field_item s_sdram_field_table_t124[];
extern field_item s_sdram_field_table_t132[];
extern field_item s_sdram_field_table_t210[];

extern field_item s_nand_table_t20[];
extern field_item s_nand_table_t30[];

extern field_item s_sdmmc_table_t20[];
extern field_item s_sdmmc_table_t30[];
extern field_item s_sdmmc_table_t114[];
extern field_item s_sdmmc_table_t124[];
extern field_item s_sdmmc_table_t132[];
extern field_item s_sdmmc_table_t210[];

extern field_item s_spiflash_table_t20[];
extern field_item s_spiflash_table_t30[];
extern field_item s_spiflash_table_t114[];
extern field_item s_spiflash_table_t124[];
extern field_item s_spiflash_table_t132[];
extern field_item s_spiflash_table_t210[];

extern parse_subfield_item s_device_type_table_t20[];
extern parse_subfield_item s_device_type_table_t30[];
extern parse_subfield_item s_device_type_table_t114[];
extern parse_subfield_item s_device_type_table_t124[];
extern parse_subfield_item s_device_type_table_t132[];
extern parse_subfield_item s_device_type_table_t210[];

#endif /* #ifndef INCLUDED_PARSE_H */
