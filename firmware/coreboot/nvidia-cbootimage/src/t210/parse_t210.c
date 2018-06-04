/*
 * Copyright (c) 2015, NVIDIA CORPORATION.  All rights reserved.
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
 * parse_t210.c - The implementation for parsing dev/sdram parameters
 */

#include "../parse.h"
#include "nvboot_bct_t210.h"

enum_item s_devtype_table_t210[] = {
	{ "NvBootDevType_Sdmmc", nvboot_dev_type_sdmmc },
	{ "NvBootDevType_Spi", nvboot_dev_type_spi },
	{ "Sdmmc", nvboot_dev_type_sdmmc },
	{ "Spi", nvboot_dev_type_spi },
	{ NULL, 0 }
};

enum_item s_sdmmc_data_width_table_t210[] = {
	{
	  "NvBootSdmmcDataWidth_4Bit",
	  nvboot_sdmmc_data_width_4bit
	},
	{
	  "NvBootSdmmcDataWidth_8Bit",
	  nvboot_sdmmc_data_width_8bit
	},
	{ "4Bit", nvboot_sdmmc_data_width_4bit },
	{ "8Bit", nvboot_sdmmc_data_width_8bit },
	{ NULL, 0 }
};

enum_item s_spi_clock_source_table_t210[] = {
	{ "NvBootSpiClockSource_PllPOut0", nvboot_spi_clock_source_pllp_out0 },
	{ "NvBootSpiClockSource_ClockM", nvboot_spi_clock_source_clockm },
	{ "ClockSource_PllPOut0", nvboot_spi_clock_source_pllp_out0 },
	{ "ClockSource_ClockM", nvboot_spi_clock_source_clockm },
	{ "PllPOut0", nvboot_spi_clock_source_pllp_out0 },
	{ "ClockM", nvboot_spi_clock_source_clockm },
	{ NULL, 0 }
};

enum_item s_nvboot_memory_type_table_t210[] = {
	{ "NvBootMemoryType_None", nvboot_memory_type_none },
	{ "NvBootMemoryType_Ddr3", nvboot_memory_type_ddr3 },
	{ "NvBootMemoryType_Ddr2", nvboot_memory_type_ddr2 },
	{ "NvBootMemoryType_Ddr", nvboot_memory_type_ddr },
	{ "NvBootMemoryType_LpDdr2", nvboot_memory_type_lpddr2 },
	{ "NvBootMemoryType_LpDdr4", nvboot_memory_type_lpddr4 },
	{ "NvBootMemoryType_LpDdr", nvboot_memory_type_lpddr },

	{ "None", nvboot_memory_type_none },
	{ "Ddr3", nvboot_memory_type_ddr3 },
	{ "Ddr2", nvboot_memory_type_ddr2 },
	{ "Ddr", nvboot_memory_type_ddr },
	{ "LpDdr2", nvboot_memory_type_lpddr2 },
	{ "LpDdr4", nvboot_memory_type_lpddr4 },
	{ "LpDdr", nvboot_memory_type_lpddr },

	{ NULL, 0 }
};

#define TOKEN(name)						\
	token_##name, field_type_u32, NULL

field_item s_sdram_field_table_t210[] = {
	{ "MemoryType", token_memory_type,
	  field_type_enum, s_nvboot_memory_type_table_t210 },

	{ "PllMInputDivider",           TOKEN(pllm_input_divider) },
	{ "PllMFeedbackDivider",        TOKEN(pllm_feedback_divider) },
	{ "PllMStableTime",             TOKEN(pllm_stable_time) },
	{ "PllMSetupControl",           TOKEN(pllm_setup_control) },
	{ "PllMPostDivider",            TOKEN(pllm_post_divider) },
	{ "PllMKCP",                    TOKEN(pllm_kcp) },
	{ "PllMKVCO",                   TOKEN(pllm_kvco) },
	{ "EmcBctSpare0",               TOKEN(emc_bct_spare0) },
	{ "EmcBctSpare1",               TOKEN(emc_bct_spare1) },
	{ "EmcBctSpare2",               TOKEN(emc_bct_spare2) },
	{ "EmcBctSpare3",               TOKEN(emc_bct_spare3) },
	{ "EmcBctSpare4",               TOKEN(emc_bct_spare4) },
	{ "EmcBctSpare5",               TOKEN(emc_bct_spare5) },
	{ "EmcBctSpare6",               TOKEN(emc_bct_spare6) },
	{ "EmcBctSpare7",               TOKEN(emc_bct_spare7) },
	{ "EmcBctSpare8",               TOKEN(emc_bct_spare8) },
	{ "EmcBctSpare9",               TOKEN(emc_bct_spare9) },
	{ "EmcBctSpare10",              TOKEN(emc_bct_spare10) },
	{ "EmcBctSpare11",              TOKEN(emc_bct_spare11) },
	{ "EmcBctSpare12",              TOKEN(emc_bct_spare12) },
	{ "EmcBctSpare13",              TOKEN(emc_bct_spare13) },
	{ "EmcClockSource",             TOKEN(emc_clock_source) },
	{ "EmcClockSourceDll",          TOKEN(emc_clock_source_dll) },

	{ "ClkRstControllerPllmMisc2Override",
			                TOKEN(clk_rst_pllm_misc20_override) },
	{ "ClkRstControllerPllmMisc2OverrideEnable",
			         TOKEN(clk_rst_pllm_misc20_override_enable) },
	{ "ClearClk2Mc1",               TOKEN(clear_clock2_mc1) },

	{ "EmcAutoCalInterval",         TOKEN(emc_auto_cal_interval) },
	{ "EmcAutoCalConfig",           TOKEN(emc_auto_cal_config) },
	{ "EmcAutoCalConfig2",          TOKEN(emc_auto_cal_config2) },
	{ "EmcAutoCalConfig3",          TOKEN(emc_auto_cal_config3) },
	{ "EmcAutoCalConfig4",          TOKEN(emc_auto_cal_config4) },
	{ "EmcAutoCalConfig5",          TOKEN(emc_auto_cal_config5) },
	{ "EmcAutoCalConfig6",          TOKEN(emc_auto_cal_config6) },
	{ "EmcAutoCalConfig7",          TOKEN(emc_auto_cal_config7) },
	{ "EmcAutoCalConfig8",          TOKEN(emc_auto_cal_config8) },

	{ "EmcAutoCalVrefSel0",         TOKEN(emc_auto_cal_vref_sel0) },
	{ "EmcAutoCalVrefSel1",         TOKEN(emc_auto_cal_vref_sel1) },

	{ "EmcAutoCalChannel",          TOKEN(emc_auto_cal_channel) },
	{ "EmcPmacroAutocalCfg0",       TOKEN(emc_pmacro_auto_cal_cfg0) },
	{ "EmcPmacroAutocalCfg1",       TOKEN(emc_pmacro_auto_cal_cfg1) },
	{ "EmcPmacroAutocalCfg2",       TOKEN(emc_pmacro_auto_cal_cfg2) },
	{ "EmcPmacroRxTerm",            TOKEN(emc_pmacro_rx_term) },
	{ "EmcPmacroDqTxDrv",           TOKEN(emc_pmacro_dq_tx_drive) },
	{ "EmcPmacroCaTxDrv",           TOKEN(emc_pmacro_ca_tx_drive) },
	{ "EmcPmacroCmdTxDrv",          TOKEN(emc_pmacro_cmd_tx_drive) },
	{ "EmcPmacroAutocalCfgCommon",  TOKEN(emc_pmacro_auto_cal_common) },
	{ "EmcPmacroZctrl",             TOKEN(emc_pmacro_zcrtl) },

	{ "EmcAutoCalWait",             TOKEN(emc_auto_cal_wait) },

	{ "EmcXm2CompPadCtrl",          TOKEN(emc_xm2_comp_pad_ctrl) },
	{ "EmcXm2CompPadCtrl2",         TOKEN(emc_xm2_comp_pad_ctrl2) },
	{ "EmcXm2CompPadCtrl3",         TOKEN(emc_xm2_comp_pad_ctrl3) },

	{ "EmcAdrCfg",                  TOKEN(emc_adr_cfg) },

	{ "EmcPinProgramWait",          TOKEN(emc_pin_program_wait) },
	{ "EmcPinExtraWait",            TOKEN(emc_pin_extra_wait) },

	{ "EmcPinGpioEn",               TOKEN(emc_pin_gpio_enable) },
	{ "EmcPinGpio",                 TOKEN(emc_pin_gpio) },

	{ "EmcTimingControlWait",       TOKEN(emc_timing_control_wait) },

	{ "EmcRc",                      TOKEN(emc_rc) },
	{ "EmcRfc",                     TOKEN(emc_rfc) },

	{ "EmcRfcPb",                   TOKEN(emc_rfc_pb) },
	{ "EmcRefctrl2",                TOKEN(emc_ref_ctrl2) },

	{ "EmcRfcSlr",                  TOKEN(emc_rfc_slr) },
	{ "EmcRas",                     TOKEN(emc_ras) },
	{ "EmcRp",                      TOKEN(emc_rp) },
	{ "EmcR2r",                     TOKEN(emc_r2r) },
	{ "EmcW2w",                     TOKEN(emc_w2w) },
	{ "EmcR2w",                     TOKEN(emc_r2w) },
	{ "EmcW2r",                     TOKEN(emc_w2r) },
	{ "EmcR2p",                     TOKEN(emc_r2p) },
	{ "EmcW2p",                     TOKEN(emc_w2p) },

	{ "EmcTppd",                    TOKEN(emc_tppd) },
	{ "EmcCcdmw",                   TOKEN(emc_ccdmw) },

	{ "EmcRdRcd",                   TOKEN(emc_rd_rcd) },
	{ "EmcWrRcd",                   TOKEN(emc_wr_rcd) },
	{ "EmcRrd",                     TOKEN(emc_rrd) },
	{ "EmcRext",                    TOKEN(emc_rext) },
	{ "EmcWext",                    TOKEN(emc_wext) },
	{ "EmcWdv",                     TOKEN(emc_wdv) },

	{ "EmcWdvChk",                  TOKEN(emc_wdv_chk) },
	{ "EmcWsv",                     TOKEN(emc_wsv) },
	{ "EmcWev",                     TOKEN(emc_wev) },

	{ "EmcWdvMask",                 TOKEN(emc_wdv_mask) },

	{ "EmcWsDuration",              TOKEN(emc_ws_duration) },
	{ "EmcWeDuration",              TOKEN(emc_we_duration) },

	{ "EmcQUse",                    TOKEN(emc_quse) },
	{ "EmcQuseWidth",               TOKEN(emc_quse_width) },
	{ "EmcIbdly",                   TOKEN(emc_ibdly) },

	{ "EmcObdly",                   TOKEN(emc_obdly) },

	{ "EmcEInput",                  TOKEN(emc_einput) },
	{ "EmcEInputDuration",          TOKEN(emc_einput_duration) },
	{ "EmcPutermExtra",             TOKEN(emc_puterm_extra) },
	{ "EmcPutermWidth",             TOKEN(emc_puterm_width) },
	{ "EmcQRst",                    TOKEN(emc_qrst) },
	{ "EmcQSafe",                   TOKEN(emc_qsafe) },
	{ "EmcRdv",                     TOKEN(emc_rdv) },
	{ "EmcRdvMask",                 TOKEN(emc_rdv_mask) },

	{ "EmcRdvEarly",                TOKEN(emc_rdv_early) },
	{ "EmcRdvEarlyMask",            TOKEN(emc_rdv_early_mask) },

	{ "EmcQpop",                    TOKEN(emc_qpop) },
	{ "EmcRefresh",                 TOKEN(emc_refresh) },
	{ "EmcBurstRefreshNum",         TOKEN(emc_burst_refresh_num) },

	{ "EmcPreRefreshReqCnt",        TOKEN(emc_prerefresh_req_cnt) },

	{ "EmcPdEx2Wr",                 TOKEN(emc_pdex2wr) },
	{ "EmcPdEx2Rd",                 TOKEN(emc_pdex2rd) },
	{ "EmcPChg2Pden",               TOKEN(emc_pchg2pden) },
	{ "EmcAct2Pden",                TOKEN(emc_act2pden) },
	{ "EmcAr2Pden",                 TOKEN(emc_ar2pden) },
	{ "EmcRw2Pden",                 TOKEN(emc_rw2pden) },

	{ "EmcCke2Pden",                TOKEN(emc_cke2pden) },
	{ "EmcPdex2Cke",                TOKEN(emc_pdex2che) },
	{ "EmcPdex2Mrr",                TOKEN(emc_pdex2mrr) },

	{ "EmcTxsr",                    TOKEN(emc_txsr) },
	{ "EmcTxsrDll",                 TOKEN(emc_txsr_dll) },
	{ "EmcTcke",                    TOKEN(emc_tcke) },
	{ "EmcTckesr",                  TOKEN(emc_tckesr) },
	{ "EmcTpd",                     TOKEN(emc_tpd) },
	{ "EmcTfaw",                    TOKEN(emc_tfaw) },
	{ "EmcTrpab",                   TOKEN(emc_trpab) },
	{ "EmcTClkStable",              TOKEN(emc_tclkstable) },
	{ "EmcTClkStop",                TOKEN(emc_tclkstop) },
	{ "EmcTRefBw",                  TOKEN(emc_trefbw) },
	{ "EmcFbioCfg5",                TOKEN(emc_fbio_cfg5) },
	{ "EmcFbioCfg7",                TOKEN(emc_fbio_cfg7) },
	{ "EmcFbioCfg8",                TOKEN(emc_fbio_cfg8) },

	{ "EmcCmdMappingCmd0_0",        TOKEN(emc_cmd_mapping_cmd0_0) },
	{ "EmcCmdMappingCmd0_1",        TOKEN(emc_cmd_mapping_cmd0_1) },
	{ "EmcCmdMappingCmd0_2",        TOKEN(emc_cmd_mapping_cmd0_2) },
	{ "EmcCmdMappingCmd1_0",        TOKEN(emc_cmd_mapping_cmd1_0) },
	{ "EmcCmdMappingCmd1_1",        TOKEN(emc_cmd_mapping_cmd1_1) },
	{ "EmcCmdMappingCmd1_2",        TOKEN(emc_cmd_mapping_cmd1_2) },
	{ "EmcCmdMappingCmd2_0",        TOKEN(emc_cmd_mapping_cmd2_0) },
	{ "EmcCmdMappingCmd2_1",        TOKEN(emc_cmd_mapping_cmd2_1) },
	{ "EmcCmdMappingCmd2_2",        TOKEN(emc_cmd_mapping_cmd2_2) },
	{ "EmcCmdMappingCmd3_0",        TOKEN(emc_cmd_mapping_cmd3_0) },
	{ "EmcCmdMappingCmd3_1",        TOKEN(emc_cmd_mapping_cmd3_1) },
	{ "EmcCmdMappingCmd3_2",        TOKEN(emc_cmd_mapping_cmd3_2) },
	{ "EmcCmdMappingByte",          TOKEN(emc_cmd_mapping_byte) },

	{ "EmcFbioSpare",               TOKEN(emc_fbio_spare) },
	{ "EmcCfgRsv",                  TOKEN(emc_cfg_rsv) },
	{ "EmcMrs",                     TOKEN(emc_mrs) },

	{ "EmcEmrs",                    TOKEN(emc_emrs) },

	{ "EmcEmrs2",                   TOKEN(emc_emrs2) },
	{ "EmcEmrs3",                   TOKEN(emc_emrs3) },
	{ "EmcMrw1",                    TOKEN(emc_mrw1) },
	{ "EmcMrw2",                    TOKEN(emc_mrw2) },
	{ "EmcMrw3",                    TOKEN(emc_mrw3) },
	{ "EmcMrw4",                    TOKEN(emc_mrw4) },

	{ "EmcMrw6",                    TOKEN(emc_mrw6) },
	{ "EmcMrw8",                    TOKEN(emc_mrw8) },
	{ "EmcMrw9",                    TOKEN(emc_mrw9) },
	{ "EmcMrw10",                   TOKEN(emc_mrw10) },
	{ "EmcMrw12",                   TOKEN(emc_mrw12) },
	{ "EmcMrw13",                   TOKEN(emc_mrw13) },
	{ "EmcMrw14",                   TOKEN(emc_mrw14) },

	{ "EmcMrwExtra",                TOKEN(emc_mrw_extra) },
	{ "EmcWarmBootMrwExtra",        TOKEN(emc_warm_boot_mrw_extra) },
	{ "EmcWarmBootExtraModeRegWriteEnable",
			TOKEN(emc_warm_boot_extramode_reg_write_enable) },
	{ "EmcExtraModeRegWriteEnable", TOKEN(emc_extramode_reg_write_enable) },
	{ "EmcMrwResetCommand",         TOKEN(emc_mrw_reset_command) },
	{ "EmcMrwResetNInitWait",       TOKEN(emc_mrw_reset_ninit_wait) },
	{ "EmcMrsWaitCnt",              TOKEN(emc_mrs_wait_cnt) },
	{ "EmcMrsWaitCnt2",             TOKEN(emc_mrs_wait_cnt2) },
	{ "EmcCfg",                     TOKEN(emc_cfg) },
	{ "EmcCfg2",                    TOKEN(emc_cfg2) },
	{ "EmcCfgPipe",                 TOKEN(emc_cfg_pipe) },

	{ "EmcCfgPipeClk",              TOKEN(emc_cfg_pipe_clk) },
	{ "EmcFdpdCtrlCmdNoRamp",       TOKEN(emc_fdpd_ctrl_cmd_no_ramp) },
	{ "EmcCfgUpdate",               TOKEN(emc_cfg_update) },

	{ "EmcDbg",                     TOKEN(emc_dbg) },
	{ "EmcDbgWriteMux",             TOKEN(emc_dbg_write_mux) },

	{ "EmcCmdQ",                    TOKEN(emc_cmd_q) },
	{ "EmcMc2EmcQ",                 TOKEN(emc_mc2emc_q) },
	{ "EmcDynSelfRefControl",       TOKEN(emc_dyn_self_ref_control) },
	{ "AhbArbitrationXbarCtrlMemInitDone",
			TOKEN(ahb_arbitration_xbar_ctrl_meminit_done) },
	{ "EmcCfgDigDll",               TOKEN(emc_cfg_dig_dll) },

	{ "EmcCfgDigDll_1",             TOKEN(emc_cfg_dig_dll_1) },

	{ "EmcCfgDigDllPeriod",         TOKEN(emc_cfg_dig_dll_period) },
	{ "EmcDevSelect",               TOKEN(emc_dev_select) },
	{ "EmcSelDpdCtrl",              TOKEN(emc_sel_dpd_ctrl) },

	{ "EmcFdpdCtrlDq",              TOKEN(emc_fdpd_ctrl_dq) },
	{ "EmcFdpdCtrlCmd",             TOKEN(emc_fdpd_ctrl_cmd) },
	{ "EmcPmacroIbVrefDq_0",        TOKEN(emc_pmacro_ib_vref_dq_0) },
	{ "EmcPmacroIbVrefDq_1",        TOKEN(emc_pmacro_ib_vref_dq_1) },
	{ "EmcPmacroIbVrefDqs_0",       TOKEN(emc_pmacro_ib_vref_dqs_0) },
	{ "EmcPmacroIbVrefDqs_1",       TOKEN(emc_pmacro_ib_vref_dqs_1) },
	{ "EmcPmacroIbRxrt",            TOKEN(emc_pmacro_ib_rxrt) },
	{ "EmcCfgPipe1",                TOKEN(emc_cfg_pipe1) },
	{ "EmcCfgPipe2",                TOKEN(emc_cfg_pipe2) },
	{ "EmcPmacroQuseDdllRank0_0",   TOKEN(emc_pmacro_quse_ddll_rank0_0) },
	{ "EmcPmacroQuseDdllRank0_1",   TOKEN(emc_pmacro_quse_ddll_rank0_1) },
	{ "EmcPmacroQuseDdllRank0_2",   TOKEN(emc_pmacro_quse_ddll_rank0_2) },
	{ "EmcPmacroQuseDdllRank0_3",   TOKEN(emc_pmacro_quse_ddll_rank0_3) },
	{ "EmcPmacroQuseDdllRank0_4",   TOKEN(emc_pmacro_quse_ddll_rank0_4) },
	{ "EmcPmacroQuseDdllRank0_5",   TOKEN(emc_pmacro_quse_ddll_rank0_5) },
	{ "EmcPmacroQuseDdllRank1_0",   TOKEN(emc_pmacro_quse_ddll_rank1_0) },
	{ "EmcPmacroQuseDdllRank1_1",   TOKEN(emc_pmacro_quse_ddll_rank1_1) },
	{ "EmcPmacroQuseDdllRank1_2",   TOKEN(emc_pmacro_quse_ddll_rank1_2) },
	{ "EmcPmacroQuseDdllRank1_3",   TOKEN(emc_pmacro_quse_ddll_rank1_3) },
	{ "EmcPmacroQuseDdllRank1_4",   TOKEN(emc_pmacro_quse_ddll_rank1_4) },
	{ "EmcPmacroQuseDdllRank1_5",   TOKEN(emc_pmacro_quse_ddll_rank1_5) },
	{ "EmcPmacroObDdllLongDqRank0_0",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank0_0) },
	{ "EmcPmacroObDdllLongDqRank0_1",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank0_1) },
	{ "EmcPmacroObDdllLongDqRank0_2",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank0_2) },
	{ "EmcPmacroObDdllLongDqRank0_3",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank0_3) },
	{ "EmcPmacroObDdllLongDqRank0_4",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank0_4) },
	{ "EmcPmacroObDdllLongDqRank0_5",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank0_5) },
	{ "EmcPmacroObDdllLongDqRank1_0",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank1_0) },
	{ "EmcPmacroObDdllLongDqRank1_1",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank1_1) },
	{ "EmcPmacroObDdllLongDqRank1_2",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank1_2) },
	{ "EmcPmacroObDdllLongDqRank1_3",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank1_3) },
	{ "EmcPmacroObDdllLongDqRank1_4",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank1_4) },
	{ "EmcPmacroObDdllLongDqRank1_5",   TOKEN(emc_pmacro_ob_ddll_long_dq_rank1_5) },
	{ "EmcPmacroObDdllLongDqsRank0_0",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank0_0) },
	{ "EmcPmacroObDdllLongDqsRank0_1",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank0_1) },
	{ "EmcPmacroObDdllLongDqsRank0_2",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank0_2) },
	{ "EmcPmacroObDdllLongDqsRank0_3",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank0_3) },
	{ "EmcPmacroObDdllLongDqsRank0_4",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank0_4) },
	{ "EmcPmacroObDdllLongDqsRank0_5",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank0_5) },
	{ "EmcPmacroObDdllLongDqsRank1_0",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank1_0) },
	{ "EmcPmacroObDdllLongDqsRank1_1",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank1_1) },
	{ "EmcPmacroObDdllLongDqsRank1_2",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank1_2) },
	{ "EmcPmacroObDdllLongDqsRank1_3",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank1_3) },
	{ "EmcPmacroObDdllLongDqsRank1_4",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank1_4) },
	{ "EmcPmacroObDdllLongDqsRank1_5",   TOKEN(emc_pmacro_ob_ddll_long_dqs_rank1_5) },

	{ "EmcPmacroIbDdllLongDqsRank0_0",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank0_0) },
	{ "EmcPmacroIbDdllLongDqsRank0_1",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank0_1) },
	{ "EmcPmacroIbDdllLongDqsRank0_2",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank0_2) },
	{ "EmcPmacroIbDdllLongDqsRank0_3",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank0_3) },
	{ "EmcPmacroIbDdllLongDqsRank1_0",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank1_0) },
	{ "EmcPmacroIbDdllLongDqsRank1_1",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank1_1) },
	{ "EmcPmacroIbDdllLongDqsRank1_2",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank1_2) },
	{ "EmcPmacroIbDdllLongDqsRank1_3",   TOKEN(emc_pmacro_ib_ddll_long_dqs_rank1_3) },

	{ "EmcPmacroDdllLongCmd_0",     TOKEN(emc_pmacro_ddll_long_cmd_0) },
	{ "EmcPmacroDdllLongCmd_1",     TOKEN(emc_pmacro_ddll_long_cmd_1) },
	{ "EmcPmacroDdllLongCmd_2",     TOKEN(emc_pmacro_ddll_long_cmd_2) },
	{ "EmcPmacroDdllLongCmd_3",     TOKEN(emc_pmacro_ddll_long_cmd_3) },
	{ "EmcPmacroDdllLongCmd_4",     TOKEN(emc_pmacro_ddll_long_cmd_4) },
	{ "EmcPmacroDdllShortCmd_0",    TOKEN(emc_pmacro_ddll_short_cmd_0) },
	{ "EmcPmacroDdllShortCmd_1",    TOKEN(emc_pmacro_ddll_short_cmd_1) },
	{ "EmcPmacroDdllShortCmd_2",    TOKEN(emc_pmacro_ddll_short_cmd_2) },

	{ "WarmBootWait",               TOKEN(warm_boot_wait) },
	{ "EmcOdtWrite",                TOKEN(emc_odt_write) },
	{ "EmcZcalInterval",            TOKEN(emc_zcal_interval) },
	{ "EmcZcalWaitCnt",             TOKEN(emc_zcal_wait_cnt) },
	{ "EmcZcalMrwCmd",              TOKEN(emc_zcal_mrw_cmd) },
	{ "EmcMrsResetDll",             TOKEN(emc_mrs_reset_dll) },


	{ "EmcZcalInitDev0",            TOKEN(emc_zcal_init_dev0) },
	{ "EmcZcalInitDev1",            TOKEN(emc_zcal_init_dev1) },
	{ "EmcZcalInitWait",            TOKEN(emc_zcal_init_wait) },
	{ "EmcZcalWarmColdBootEnables", TOKEN(emc_zcal_warm_cold_boot_enables) },
	{ "EmcMrwLpddr2ZcalWarmBoot",   TOKEN(emc_mrw_lpddr2zcal_warm_boot) },
	{ "EmcZqCalDdr3WarmBoot",       TOKEN(emc_zqcal_ddr3_warm_boot) },

	{ "EmcZqCalLpDdr4WarmBoot",     TOKEN(emc_zqcal_lpddr4_warm_boot) },

	{ "EmcZcalWarmBootWait",        TOKEN(emc_zcal_warm_boot_wait) },
	{ "EmcMrsWarmBootEnable",       TOKEN(emc_mrs_warm_boot_enable) },

	{ "EmcMrsResetDllWait",         TOKEN(emc_mrs_reset_dll_wait) },

	{ "EmcMrsExtra",                TOKEN(emc_mrs_extra) },
	{ "EmcWarmBootMrsExtra",        TOKEN(emc_warm_boot_mrs_extra) },

	{ "EmcEmrsDdr2DllEnable",       TOKEN(emc_emrs_ddr2_dll_enable) },
	{ "EmcMrsDdr2DllReset",         TOKEN(emc_mrs_ddr2_dll_reset) },
	{ "EmcEmrsDdr2OcdCalib",        TOKEN(emc_emrs_ddr2_ocd_calib) },

	{ "EmcDdr2Wait",                TOKEN(emc_ddr2_wait) },
	{ "EmcClkenOverride",           TOKEN(emc_clken_override) },
	{ "EmcExtraRefreshNum",         TOKEN(emc_extra_refresh_num) },
	{ "EmcClkenOverrideAllWarmBoot",
			TOKEN(emc_clken_override_allwarm_boot) },
	{ "McClkenOverrideAllWarmBoot", TOKEN(mc_clken_override_allwarm_boot) },
	{ "EmcCfgDigDllPeriodWarmBoot",
			TOKEN(emc_cfg_dig_dll_period_warm_boot) },
	{ "PmcVddpSel",                 TOKEN(pmc_vddp_sel) },
	{ "PmcVddpSelWait",             TOKEN(pmc_vddp_sel_wait) },
	{ "PmcDdrPwr",                  TOKEN(pmc_ddr_pwr) },
	{ "PmcDdrCfg",                  TOKEN(pmc_ddr_cfg) },
	{ "PmcIoDpd3Req",               TOKEN(pmc_io_dpd3_req) },
	{ "PmcIoDpd3ReqWait",           TOKEN(pmc_io_dpd3_req_wait) },

	{ "PmcIoDpd4ReqWait",           TOKEN(pmc_io_dpd4_req_wait) },

	{ "PmcRegShort",                TOKEN(pmc_reg_short) },
	{ "PmcNoIoPower",               TOKEN(pmc_no_io_power) },

	{ "PmcDdrCntrlWait",            TOKEN(pmc_ddr_ctrl_wait) },
	{ "PmcDdrCntrl",                TOKEN(pmc_ddr_ctrl) },

	{ "EmcAcpdControl",             TOKEN(emc_acpd_control) },
	{ "EmcSwizzleRank0Byte0",       TOKEN(emc_swizzle_rank0_byte0) },
	{ "EmcSwizzleRank0Byte1",       TOKEN(emc_swizzle_rank0_byte1) },
	{ "EmcSwizzleRank0Byte2",       TOKEN(emc_swizzle_rank0_byte2) },
	{ "EmcSwizzleRank0Byte3",       TOKEN(emc_swizzle_rank0_byte3) },
	{ "EmcSwizzleRank1Byte0",       TOKEN(emc_swizzle_rank1_byte0) },
	{ "EmcSwizzleRank1Byte1",       TOKEN(emc_swizzle_rank1_byte1) },
	{ "EmcSwizzleRank1Byte2",       TOKEN(emc_swizzle_rank1_byte2) },
	{ "EmcSwizzleRank1Byte3",       TOKEN(emc_swizzle_rank1_byte3) },

	{ "EmcTxdsrvttgen",             TOKEN(emc_txdsrvttgen) },

	{ "EmcDataBrlshft0",            TOKEN(emc_data_brlshft0) },
	{ "EmcDataBrlshft1",            TOKEN(emc_data_brlshft1) },
	{ "EmcDqsBrlshft0",             TOKEN(emc_dqs_brlshft0) },
	{ "EmcDqsBrlshft1",             TOKEN(emc_dqs_brlshft1) },
	{ "EmcCmdBrlshft0",             TOKEN(emc_cmd_brlshft0) },
	{ "EmcCmdBrlshft1",             TOKEN(emc_cmd_brlshft1) },
	{ "EmcCmdBrlshft2",             TOKEN(emc_cmd_brlshft2) },
	{ "EmcCmdBrlshft3",             TOKEN(emc_cmd_brlshft3) },

	{ "EmcQuseBrlshft0",            TOKEN(emc_quse_brlshft0) },
	{ "EmcQuseBrlshft1",            TOKEN(emc_quse_brlshft1) },
	{ "EmcQuseBrlshft2",            TOKEN(emc_quse_brlshft2) },
	{ "EmcQuseBrlshft3",            TOKEN(emc_quse_brlshft3) },

	{ "EmcDllCfg0",                 TOKEN(emc_dll_cfg0) },
	{ "EmcDllCfg1",                 TOKEN(emc_dll_cfg1) },

	{ "EmcPmcScratch1",             TOKEN(emc_pmc_scratch1) },
	{ "EmcPmcScratch2",             TOKEN(emc_pmc_scratch2) },
	{ "EmcPmcScratch3",             TOKEN(emc_pmc_scratch3) },

	{ "EmcPmacroPadCfgCtrl",        TOKEN(emc_pmacro_pad_cfg_ctrl) },

	{ "EmcPmacroVttgenCtrl0",       TOKEN(emc_pmacro_vttgen_ctrl0) },
	{ "EmcPmacroVttgenCtrl1",       TOKEN(emc_pmacro_vttgen_ctrl1) },
	{ "EmcPmacroVttgenCtrl2",       TOKEN(emc_pmacro_vttgen_ctrl2) },

	{ "EmcPmacroBrickCtrlRfu1",     TOKEN(emc_pmacro_brick_ctrl_rfu1) },
	{ "EmcPmacroCmdBrickCtrlFdpd",  TOKEN(emc_pmacro_cmd_brick_ctrl_fdpd) },

	{ "EmcPmacroBrickCtrlRfu2",     TOKEN(emc_pmacro_brick_ctrl_rfu2) },
	{ "EmcPmacroDataBrickCtrlFdpd", TOKEN(emc_pmacro_data_brick_ctrl_fdpd) },

	{ "EmcPmacroBgBiasCtrl0",       TOKEN(emc_pmacro_bg_bias_ctrl0) },
	{ "EmcPmacroDataPadRxCtrl",     TOKEN(emc_pmacro_data_pad_rx_ctrl) },
	{ "EmcPmacroCmdPadRxCtrl",      TOKEN(emc_pmacro_cmd_pad_rx_ctrl) },
	{ "EmcPmacroDataRxTermMode",    TOKEN(emc_pmacro_data_rx_term_mode) },

	{ "EmcPmacroCmdRxTermMode",     TOKEN(emc_pmacro_cmd_rx_term_mode) },
	{ "EmcPmacroDataPadTxCtrl",     TOKEN(emc_pmacro_data_pad_tx_ctrl) },
	{ "EmcPmacroCommonPadTxCtrl",   TOKEN(emc_pmacro_common_pad_tx_ctrl) },
	{ "EmcPmacroCmdPadTxCtrl",      TOKEN(emc_pmacro_cmd_pad_tx_ctrl) },

	{ "EmcCfg3",                    TOKEN(emc_cfg3) },

	{ "EmcPmacroTxPwrd0",           TOKEN(emc_pmacro_tx_pwrd0) },
	{ "EmcPmacroTxPwrd1",           TOKEN(emc_pmacro_tx_pwrd1) },
	{ "EmcPmacroTxPwrd2",           TOKEN(emc_pmacro_tx_pwrd2) },
	{ "EmcPmacroTxPwrd3",           TOKEN(emc_pmacro_tx_pwrd3) },
	{ "EmcPmacroTxPwrd4",           TOKEN(emc_pmacro_tx_pwrd4) },
	{ "EmcPmacroTxPwrd5",           TOKEN(emc_pmacro_tx_pwrd5) },

	{ "EmcConfigSampleDelay",       TOKEN(emc_config_sample_delay) },

	{ "EmcPmacroBrickMapping0",     TOKEN(emc_pmacro_brick_mapping0) },
	{ "EmcPmacroBrickMapping1",     TOKEN(emc_pmacro_brick_mapping1) },
	{ "EmcPmacroBrickMapping2",     TOKEN(emc_pmacro_brick_mapping2) },

	{ "EmcPmacroTxSelClkSrc0",      TOKEN(emc_pmacro_tx_sel_clk_src0) },
	{ "EmcPmacroTxSelClkSrc1",      TOKEN(emc_pmacro_tx_sel_clk_src1) },
	{ "EmcPmacroTxSelClkSrc2",      TOKEN(emc_pmacro_tx_sel_clk_src2) },
	{ "EmcPmacroTxSelClkSrc3",      TOKEN(emc_pmacro_tx_sel_clk_src3) },
	{ "EmcPmacroTxSelClkSrc4",      TOKEN(emc_pmacro_tx_sel_clk_src4) },
	{ "EmcPmacroTxSelClkSrc5",      TOKEN(emc_pmacro_tx_sel_clk_src5) },

	{ "EmcPmacroDdllBypass",        TOKEN(emc_pmacro_ddll_bypass) },

	{ "EmcPmacroDdllPwrd0",         TOKEN(emc_pmacro_ddll_pwrd0) },
	{ "EmcPmacroDdllPwrd1",         TOKEN(emc_pmacro_ddll_pwrd1) },
	{ "EmcPmacroDdllPwrd2",         TOKEN(emc_pmacro_ddll_pwrd2) },

	{ "EmcPmacroCmdCtrl0",          TOKEN(emc_pmacro_cmd_ctrl0) },
	{ "EmcPmacroCmdCtrl1",          TOKEN(emc_pmacro_cmd_ctrl1) },
	{ "EmcPmacroCmdCtrl2",          TOKEN(emc_pmacro_cmd_ctrl2) },

	{ "McEmemAdrCfg",               TOKEN(mc_emem_adr_cfg) },
	{ "McEmemAdrCfgDev0",           TOKEN(mc_emem_adr_cfg_dev0) },
	{ "McEmemAdrCfgDev1",           TOKEN(mc_emem_adr_cfg_dev1) },
	{ "McEmemAdrCfgChannelMask",    TOKEN(mc_emem_adr_cfg_channel_mask) },
	{ "McEmemAdrCfgBankMask0",      TOKEN(mc_emem_adr_cfg_bank_mask0) },
	{ "McEmemAdrCfgBankMask1",      TOKEN(mc_emem_adr_cfg_bank_mask1) },
	{ "McEmemAdrCfgBankMask2",      TOKEN(mc_emem_adr_cfg_bank_mask2) },

	{ "McEmemCfg",                  TOKEN(mc_emem_cfg) },
	{ "McEmemArbCfg",               TOKEN(mc_emem_arb_cfg) },
	{ "McEmemArbOutstandingReq",    TOKEN(mc_emem_arb_outstanding_req) },

	{ "McEmemArbRefpbHpCtrl",       TOKEN(emc_emem_arb_refpb_hp_ctrl) },
	{ "McEmemArbRefpbBankCtrl",     TOKEN(emc_emem_arb_refpb_bank_ctrl) },

	{ "McEmemArbTimingRcd",         TOKEN(mc_emem_arb_timing_rcd) },
	{ "McEmemArbTimingRp",          TOKEN(mc_emem_arb_timing_rp) },
	{ "McEmemArbTimingRc",          TOKEN(mc_emem_arb_timing_rc) },
	{ "McEmemArbTimingRas",         TOKEN(mc_emem_arb_timing_ras) },
	{ "McEmemArbTimingFaw",         TOKEN(mc_emem_arb_timing_faw) },
	{ "McEmemArbTimingRrd",         TOKEN(mc_emem_arb_timing_rrd) },
	{ "McEmemArbTimingRap2Pre",     TOKEN(mc_emem_arb_timing_rap2pre) },
	{ "McEmemArbTimingWap2Pre",     TOKEN(mc_emem_arb_timing_wap2pre) },
	{ "McEmemArbTimingR2R",         TOKEN(mc_emem_arb_timing_r2r) },
	{ "McEmemArbTimingW2W",         TOKEN(mc_emem_arb_timing_w2w) },
	{ "McEmemArbTimingR2W",         TOKEN(mc_emem_arb_timing_r2w) },
	{ "McEmemArbTimingW2R",         TOKEN(mc_emem_arb_timing_w2r) },

	{ "McEmemArbTimingRFCPB",       TOKEN(mc_emem_arb_timing_rfcpb) },

	{ "McEmemArbDaTurns",           TOKEN(mc_emem_arb_da_turns) },
	{ "McEmemArbDaCovers",          TOKEN(mc_emem_arb_da_covers) },
	{ "McEmemArbMisc0",             TOKEN(mc_emem_arb_misc0) },
	{ "McEmemArbMisc1",             TOKEN(mc_emem_arb_misc1) },
	{ "McEmemArbMisc2",             TOKEN(mc_emem_arb_misc2) },
	{ "McEmemArbRing1Throttle",     TOKEN(mc_emem_arb_ring1_throttle) },
	{ "McEmemArbOverride",          TOKEN(mc_emem_arb_override) },
	{ "McEmemArbOverride1",         TOKEN(mc_emem_arb_override1) },
	{ "McEmemArbRsv",               TOKEN(mc_emem_arb_rsv) },

	{ "McDaCfg0",                   TOKEN(mc_da_cfg0) },
	{ "McEmemArbTimingCcdmw",       TOKEN(mc_emem_arb_timing_ccdmw) },

	{ "McClkenOverride",            TOKEN(mc_clken_override) },
	{ "McStatControl",              TOKEN(mc_stat_control) },
	{ "McVideoProtectBom",          TOKEN(mc_video_protect_bom) },
	{ "McVideoProtectBomAdrHi",
			TOKEN(mc_video_protect_bom_adr_hi) },
	{ "McVideoProtectSizeMb",       TOKEN(mc_video_protect_size_mb) },
	{ "McVideoProtectVprOverride",  TOKEN(mc_video_protect_vpr_override) },
	{ "McVideoProtectVprOverride1", TOKEN(mc_video_protect_vpr_override1) },
	{ "McVideoProtectGpuOverride0", TOKEN(mc_video_protect_gpu_override0) },
	{ "McVideoProtectGpuOverride1", TOKEN(mc_video_protect_gpu_override1) },
	{ "McSecCarveoutBom",           TOKEN(mc_sec_carveout_bom) },
	{ "McSecCarveoutAdrHi",         TOKEN(mc_sec_carveout_adr_hi) },
	{ "McSecCarveoutSizeMb",        TOKEN(mc_sec_carveout_size_mb) },
	{ "McVideoProtectWriteAccess",  TOKEN(mc_video_protect_write_access) },
	{ "McSecCarveoutProtectWriteAccess",
			TOKEN(mc_sec_carveout_protect_write_access) },

	{ "McGeneralizedCarveout1Bom",  TOKEN(mc_generalized_carveout1_bom) },
	{ "McGeneralizedCarveout1BomHi",TOKEN(mc_generalized_carveout1_bom_hi) },
	{ "McGeneralizedCarveout1Size128kb",
			TOKEN(mc_generalized_carveout1_size_128kb) },
	{ "McGeneralizedCarveout1Access0",
			TOKEN(mc_generalized_carveout1_access0) },
	{ "McGeneralizedCarveout1Access1",
			TOKEN(mc_generalized_carveout1_access1) },
	{ "McGeneralizedCarveout1Access2",
			TOKEN(mc_generalized_carveout1_access2) },
	{ "McGeneralizedCarveout1Access3",
			TOKEN(mc_generalized_carveout1_access3) },
	{ "McGeneralizedCarveout1Access4",
			TOKEN(mc_generalized_carveout1_access4) },
	{ "McGeneralizedCarveout1ForceInternalAccess0",
			TOKEN(mc_generalized_carveout1_force_internal_access0) },
	{ "McGeneralizedCarveout1ForceInternalAccess1",
			TOKEN(mc_generalized_carveout1_force_internal_access1) },
	{ "McGeneralizedCarveout1ForceInternalAccess2",
			TOKEN(mc_generalized_carveout1_force_internal_access2) },
	{ "McGeneralizedCarveout1ForceInternalAccess3",
			TOKEN(mc_generalized_carveout1_force_internal_access3) },
	{ "McGeneralizedCarveout1ForceInternalAccess4",
			TOKEN(mc_generalized_carveout1_force_internal_access4) },
	{ "McGeneralizedCarveout1Cfg0", TOKEN(mc_generalized_carveout1_cfg0) },

	{ "McGeneralizedCarveout2Bom",  TOKEN(mc_generalized_carveout2_bom) },
	{ "McGeneralizedCarveout2BomHi",TOKEN(mc_generalized_carveout2_bom_hi) },
	{ "McGeneralizedCarveout2Size128kb",
			TOKEN(mc_generalized_carveout2_size_128kb) },
	{ "McGeneralizedCarveout2Access0",
			TOKEN(mc_generalized_carveout2_access0) },
	{ "McGeneralizedCarveout2Access1",
			TOKEN(mc_generalized_carveout2_access1) },
	{ "McGeneralizedCarveout2Access2",
			TOKEN(mc_generalized_carveout2_access2) },
	{ "McGeneralizedCarveout2Access3",
			TOKEN(mc_generalized_carveout2_access3) },
	{ "McGeneralizedCarveout2Access4",
			TOKEN(mc_generalized_carveout2_access4) },
	{ "McGeneralizedCarveout2ForceInternalAccess0",
			TOKEN(mc_generalized_carveout2_force_internal_access0) },
	{ "McGeneralizedCarveout2ForceInternalAccess1",
			TOKEN(mc_generalized_carveout2_force_internal_access1) },
	{ "McGeneralizedCarveout2ForceInternalAccess2",
			TOKEN(mc_generalized_carveout2_force_internal_access2) },
	{ "McGeneralizedCarveout2ForceInternalAccess3",
			TOKEN(mc_generalized_carveout2_force_internal_access3) },
	{ "McGeneralizedCarveout2ForceInternalAccess4",
			TOKEN(mc_generalized_carveout2_force_internal_access4) },
	{ "McGeneralizedCarveout2Cfg0", TOKEN(mc_generalized_carveout2_cfg0) },

	{ "McGeneralizedCarveout3Bom",  TOKEN(mc_generalized_carveout3_bom) },
	{ "McGeneralizedCarveout3BomHi",TOKEN(mc_generalized_carveout3_bom_hi) },
	{ "McGeneralizedCarveout3Size128kb",
			TOKEN(mc_generalized_carveout3_size_128kb) },
	{ "McGeneralizedCarveout3Access0",
			TOKEN(mc_generalized_carveout3_access0) },
	{ "McGeneralizedCarveout3Access1",
			TOKEN(mc_generalized_carveout3_access1) },
	{ "McGeneralizedCarveout3Access2",
			TOKEN(mc_generalized_carveout3_access2) },
	{ "McGeneralizedCarveout3Access3",
			TOKEN(mc_generalized_carveout3_access3) },
	{ "McGeneralizedCarveout3Access4",
			TOKEN(mc_generalized_carveout3_access4) },
	{ "McGeneralizedCarveout3ForceInternalAccess0",
			TOKEN(mc_generalized_carveout3_force_internal_access0) },
	{ "McGeneralizedCarveout3ForceInternalAccess1",
			TOKEN(mc_generalized_carveout3_force_internal_access1) },
	{ "McGeneralizedCarveout3ForceInternalAccess2",
			TOKEN(mc_generalized_carveout3_force_internal_access2) },
	{ "McGeneralizedCarveout3ForceInternalAccess3",
			TOKEN(mc_generalized_carveout3_force_internal_access3) },
	{ "McGeneralizedCarveout3ForceInternalAccess4",
			TOKEN(mc_generalized_carveout3_force_internal_access4) },
	{ "McGeneralizedCarveout3Cfg0", TOKEN(mc_generalized_carveout3_cfg0) },

	{ "McGeneralizedCarveout4Bom",  TOKEN(mc_generalized_carveout4_bom) },
	{ "McGeneralizedCarveout4BomHi",TOKEN(mc_generalized_carveout4_bom_hi) },
	{ "McGeneralizedCarveout4Size128kb",
			TOKEN(mc_generalized_carveout4_size_128kb) },
	{ "McGeneralizedCarveout4Access0",
			TOKEN(mc_generalized_carveout4_access0) },
	{ "McGeneralizedCarveout4Access1",
			TOKEN(mc_generalized_carveout4_access1) },
	{ "McGeneralizedCarveout4Access2",
			TOKEN(mc_generalized_carveout4_access2) },
	{ "McGeneralizedCarveout4Access3",
			TOKEN(mc_generalized_carveout4_access3) },
	{ "McGeneralizedCarveout4Access4",
			TOKEN(mc_generalized_carveout4_access4) },
	{ "McGeneralizedCarveout4ForceInternalAccess0",
			TOKEN(mc_generalized_carveout4_force_internal_access0) },
	{ "McGeneralizedCarveout4ForceInternalAccess1",
			TOKEN(mc_generalized_carveout4_force_internal_access1) },
	{ "McGeneralizedCarveout4ForceInternalAccess2",
			TOKEN(mc_generalized_carveout4_force_internal_access2) },
	{ "McGeneralizedCarveout4ForceInternalAccess3",
			TOKEN(mc_generalized_carveout4_force_internal_access3) },
	{ "McGeneralizedCarveout4ForceInternalAccess4",
			TOKEN(mc_generalized_carveout4_force_internal_access4) },
	{ "McGeneralizedCarveout4Cfg0", TOKEN(mc_generalized_carveout4_cfg0) },

	{ "McGeneralizedCarveout5Bom",  TOKEN(mc_generalized_carveout5_bom) },
	{ "McGeneralizedCarveout5BomHi",TOKEN(mc_generalized_carveout5_bom_hi) },
	{ "McGeneralizedCarveout5Size128kb",
			TOKEN(mc_generalized_carveout5_size_128kb) },
	{ "McGeneralizedCarveout5Access0",
			TOKEN(mc_generalized_carveout5_access0) },
	{ "McGeneralizedCarveout5Access1",
			TOKEN(mc_generalized_carveout5_access1) },
	{ "McGeneralizedCarveout5Access2",
			TOKEN(mc_generalized_carveout5_access2) },
	{ "McGeneralizedCarveout5Access3",
			TOKEN(mc_generalized_carveout5_access3) },
	{ "McGeneralizedCarveout5Access4",
			TOKEN(mc_generalized_carveout5_access4) },
	{ "McGeneralizedCarveout5ForceInternalAccess0",
			TOKEN(mc_generalized_carveout5_force_internal_access0) },
	{ "McGeneralizedCarveout5ForceInternalAccess1",
			TOKEN(mc_generalized_carveout5_force_internal_access1) },
	{ "McGeneralizedCarveout5ForceInternalAccess2",
			TOKEN(mc_generalized_carveout5_force_internal_access2) },
	{ "McGeneralizedCarveout5ForceInternalAccess3",
			TOKEN(mc_generalized_carveout5_force_internal_access3) },
	{ "McGeneralizedCarveout5ForceInternalAccess4",
			TOKEN(mc_generalized_carveout5_force_internal_access4) },
	{ "McGeneralizedCarveout5Cfg0", TOKEN(mc_generalized_carveout5_cfg0) },

	{ "EmcCaTrainingEnable",        TOKEN(emc_ca_training_enable) },
	{ "SwizzleRankByteEncode",      TOKEN(swizzle_rank_byte_encode) },
	{ "BootRomPatchControl",        TOKEN(boot_rom_patch_control) },
	{ "BootRomPatchData",           TOKEN(boot_rom_patch_data) },

	{ "McMtsCarveoutBom",           TOKEN(mc_mts_carveout_bom) },
	{ "McMtsCarveoutAdrHi",         TOKEN(mc_mts_carveout_adr_hi) },
	{ "McMtsCarveoutSizeMb",        TOKEN(mc_mts_carveout_size_mb) },
	{ "McMtsCarveoutRegCtrl",       TOKEN(mc_mts_carveout_reg_ctrl) },
	{ NULL, 0, 0, NULL }
};

field_item s_sdmmc_table_t210[] = {
	{ "ClockDivider",               TOKEN(sdmmc_clock_divider) },
	{ "DataWidth",
	  token_sdmmc_data_width,
	  field_type_enum,
	  s_sdmmc_data_width_table_t210 },
	{ "MaxPowerClassSupported",     TOKEN(sdmmc_max_power_class_supported) },
	{ "MultiPageSupport",           TOKEN(sdmmc_multi_page_support) },
	{ NULL, 0, 0, NULL }
};

field_item s_spiflash_table_t210[] = {
	{ "ReadCommandTypeFast",        TOKEN(spiflash_read_command_type_fast) },
	{ "PageSize2kor16k",            TOKEN(spiflash_page_size_2k_or_16k) },
	{ "ClockDivider",               TOKEN(spiflash_clock_divider) },
	{ "ClockSource",
	  token_spiflash_clock_source,
	  field_type_enum,
	  s_spi_clock_source_table_t210 },
	{ NULL, 0, 0, NULL }
};

parse_subfield_item s_device_type_table_t210[] = {
	{ "SdmmcParams.", token_sdmmc_params,
		s_sdmmc_table_t210, t210_set_dev_param },
	{ "SpiFlashParams.", token_spiflash_params,
		s_spiflash_table_t210, t210_set_dev_param },
	{ NULL, 0, NULL }
};
