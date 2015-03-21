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

#include "../cbootimage.h"
#include "../parse.h"
#include "../crypto.h"
#include "nvboot_bct_t210.h"
#include "string.h"

/* nvbctlib_t210.c: The implementation of the nvbctlib API for t210. */

/* Definitions that simplify the code which follows. */
#define CASE_GET_SDRAM_PARAM(x) \
case token_##x:\
	*value = params->x; \
	break

#define CASE_SET_SDRAM_PARAM(x) \
case token_##x:\
	params->x = value; \
	break

#define CASE_GET_DEV_PARAM(dev, x) \
case token_##dev##_##x:\
	*value = bct->dev_params[index].dev##_params.x; \
	break

#define CASE_SET_DEV_PARAM(dev, x) \
case token_##dev##_##x:\
	bct->dev_params[index].dev##_params.x = value; \
	break

#define CASE_GET_BL_PARAM(x) \
case token_bl_##x:\
	*data = bct_ptr->bootloader[set].x; \
	break

#define CASE_SET_BL_PARAM(x) \
case token_bl_##x:\
	bct_ptr->bootloader[set].x = *data; \
	break

#define CASE_GET_NVU32(id) \
case token_##id:\
	if (bct == NULL) \
		return -ENODATA; \
	*((u_int32_t *)data) = bct_ptr->id; \
	break

#define CASE_GET_CONST(id, val) \
case token_##id:\
	*((u_int32_t *)data) = val; \
	break

#define CASE_GET_CONST_PREFIX(id, val_prefix) \
case token_##id:\
	*((u_int32_t *)data) = val_prefix##_##id; \
	break

#define CASE_SET_NVU32(id) \
case token_##id:\
	bct_ptr->id = *((u_int32_t *)data); \
	break

#define CASE_GET_DATA(id, size) \
case token_##id:\
	if (*length < size) \
		return -ENODATA;\
	memcpy(data, &(bct_ptr->id), size);   \
	*length = size;\
	break

#define CASE_SET_DATA(id, size) \
case token_##id:\
	if (length < size) \
		return -ENODATA;\
	memcpy(&(bct_ptr->id), data, size);   \
	break

#define DEFAULT()                                   \
default :                                           \
	printf("Unexpected token %d at line %d\n",  \
		token, __LINE__);                   \
	return 1

parse_token t210_root_token_list[] = {
	token_boot_data_version,
	token_block_size,
	token_page_size,
	token_partition_size,
	token_odm_data,
	token_bootloader_used,
	token_bootloaders_max,
	token_bct_size,
	token_hash_size,
	token_crypto_offset,
	token_crypto_length,
	token_max_bct_search_blks,
	token_unique_chip_id,
	token_secure_debug_control
};

int
t210_set_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t value)
{
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);

	bct->num_param_sets = NV_MAX(bct->num_param_sets, index + 1);

	switch (token) {
	CASE_SET_DEV_PARAM(sdmmc, clock_divider);
	CASE_SET_DEV_PARAM(sdmmc, data_width);
	CASE_SET_DEV_PARAM(sdmmc, max_power_class_supported);
	CASE_SET_DEV_PARAM(sdmmc, multi_page_support);

	CASE_SET_DEV_PARAM(spiflash, clock_source);
	CASE_SET_DEV_PARAM(spiflash, clock_divider);
	CASE_SET_DEV_PARAM(spiflash, read_command_type_fast);
	CASE_SET_DEV_PARAM(spiflash, page_size_2k_or_16k);

	case token_dev_type:
		bct->dev_type[index] = value;
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t210_get_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t *value)
{
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);

	switch (token) {
	CASE_GET_DEV_PARAM(sdmmc, clock_divider);
	CASE_GET_DEV_PARAM(sdmmc, data_width);
	CASE_GET_DEV_PARAM(sdmmc, max_power_class_supported);
	CASE_GET_DEV_PARAM(sdmmc, multi_page_support);

	CASE_GET_DEV_PARAM(spiflash, clock_source);
	CASE_GET_DEV_PARAM(spiflash, clock_divider);
	CASE_GET_DEV_PARAM(spiflash, read_command_type_fast);
	CASE_GET_DEV_PARAM(spiflash, page_size_2k_or_16k);

	case token_dev_type:
		*value = bct->dev_type[index];
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t210_get_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t *value)
{
	nvboot_sdram_params *params;
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);
	params = &(bct->sdram_params[index]);

	switch (token) {
	/* Specifies the type of memory device */
	CASE_GET_SDRAM_PARAM(memory_type);

	/* MC/EMC clock source configuration */

	/* Specifies the M value for PllM */
	CASE_GET_SDRAM_PARAM(pllm_input_divider);
	/* Specifies the N value for PllM */
	CASE_GET_SDRAM_PARAM(pllm_feedback_divider);
	/* Specifies the time to wait for PLLM to lock (in microseconds) */
	CASE_GET_SDRAM_PARAM(pllm_stable_time);
	/* Specifies misc. control bits */
	CASE_GET_SDRAM_PARAM(pllm_setup_control);
	/* Specifies the P value for PLLM */
	CASE_GET_SDRAM_PARAM(pllm_post_divider);
	/* Specifies value for Charge Pump Gain Control */
	CASE_GET_SDRAM_PARAM(pllm_kcp);
	/* Specifies VCO gain */
	CASE_GET_SDRAM_PARAM(pllm_kvco);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare0);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare1);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare2);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare3);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare4);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare5);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare6);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare7);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare8);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare9);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare10);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare11);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare12);
	/* Spare BCT param */
	CASE_GET_SDRAM_PARAM(emc_bct_spare13);

	/* Defines EMC_2X_CLK_SRC, EMC_2X_CLK_DIVISOR, EMC_INVERT_DCD */
	CASE_GET_SDRAM_PARAM(emc_clock_source);
	CASE_GET_SDRAM_PARAM(emc_clock_source_dll);

	/* Defines possible override for PLLLM_MISC2 */
	CASE_GET_SDRAM_PARAM(clk_rst_pllm_misc20_override);
	/* enables override for PLLLM_MISC2 */
	CASE_GET_SDRAM_PARAM(clk_rst_pllm_misc20_override_enable);
	/* defines CLK_ENB_MC1 in register clk_rst_controller_clk_enb_w_clr */
	CASE_GET_SDRAM_PARAM(clear_clock2_mc1);

	/* Auto-calibration of EMC pads */

	/* Specifies the value for EMC_AUTO_CAL_INTERVAL */
	CASE_GET_SDRAM_PARAM(emc_auto_cal_interval);
	/*
	 * Specifies the value for EMC_AUTO_CAL_CONFIG
	 * Note: Trigger bits are set by the SDRAM code.
	 */
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config);

	/* Specifies the value for EMC_AUTO_CAL_CONFIG2 */
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config2);

	/* Specifies the value for EMC_AUTO_CAL_CONFIG3 */
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config3);

	CASE_GET_SDRAM_PARAM(emc_auto_cal_config4);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config5);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config6);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config7);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config8);
	/* Specifies the value for EMC_AUTO_CAL_VREF_SEL_0 */
	CASE_GET_SDRAM_PARAM(emc_auto_cal_vref_sel0);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_vref_sel1);

	/* Specifies the value for EMC_AUTO_CAL_CHANNEL */
	CASE_GET_SDRAM_PARAM(emc_auto_cal_channel);

	/* Specifies the value for EMC_PMACRO_AUTOCAL_CFG_0 */
	CASE_GET_SDRAM_PARAM(emc_pmacro_auto_cal_cfg0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_auto_cal_cfg1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_auto_cal_cfg2);

	CASE_GET_SDRAM_PARAM(emc_pmacro_rx_term);
	CASE_GET_SDRAM_PARAM(emc_pmacro_dq_tx_drive);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ca_tx_drive);
	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_tx_drive);
	CASE_GET_SDRAM_PARAM(emc_pmacro_auto_cal_common);
	CASE_GET_SDRAM_PARAM(emc_pmacro_zcrtl);

	/*
	 * Specifies the time for the calibration
	 * to stabilize (in microseconds)
	 */
	CASE_GET_SDRAM_PARAM(emc_auto_cal_wait);

	CASE_GET_SDRAM_PARAM(emc_xm2_comp_pad_ctrl);
	CASE_GET_SDRAM_PARAM(emc_xm2_comp_pad_ctrl2);
	CASE_GET_SDRAM_PARAM(emc_xm2_comp_pad_ctrl3);

	/*
	 * DRAM size information
	 * Specifies the value for EMC_ADR_CFG
	 */
	CASE_GET_SDRAM_PARAM(emc_adr_cfg);

	/*
	 * Specifies the time to wait after asserting pin
	 * CKE (in microseconds)
	 */
	CASE_GET_SDRAM_PARAM(emc_pin_program_wait);
	/* Specifies the extra delay before/after pin RESET/CKE command */
	CASE_GET_SDRAM_PARAM(emc_pin_extra_wait);

	CASE_GET_SDRAM_PARAM(emc_pin_gpio_enable);
	CASE_GET_SDRAM_PARAM(emc_pin_gpio);

	/*
	 * Specifies the extra delay after the first writing
	 * of EMC_TIMING_CONTROL
	 */
	CASE_GET_SDRAM_PARAM(emc_timing_control_wait);

	/* Timing parameters required for the SDRAM */

	/* Specifies the value for EMC_RC */
	CASE_GET_SDRAM_PARAM(emc_rc);
	/* Specifies the value for EMC_RFC */
	CASE_GET_SDRAM_PARAM(emc_rfc);

	CASE_GET_SDRAM_PARAM(emc_rfc_pb);
	CASE_GET_SDRAM_PARAM(emc_ref_ctrl2);

	/* Specifies the value for EMC_RFC_SLR */
	CASE_GET_SDRAM_PARAM(emc_rfc_slr);
	/* Specifies the value for EMC_RAS */
	CASE_GET_SDRAM_PARAM(emc_ras);
	/* Specifies the value for EMC_RP */
	CASE_GET_SDRAM_PARAM(emc_rp);
	/* Specifies the value for EMC_R2R */
	CASE_GET_SDRAM_PARAM(emc_r2r);
	/* Specifies the value for EMC_W2W */
	CASE_GET_SDRAM_PARAM(emc_w2w);
	/* Specifies the value for EMC_R2W */
	CASE_GET_SDRAM_PARAM(emc_r2w);
	/* Specifies the value for EMC_W2R */
	CASE_GET_SDRAM_PARAM(emc_w2r);
	/* Specifies the value for EMC_R2P */
	CASE_GET_SDRAM_PARAM(emc_r2p);
	/* Specifies the value for EMC_W2P */
	CASE_GET_SDRAM_PARAM(emc_w2p);
	/* Specifies the value for EMC_RD_RCD */

	CASE_GET_SDRAM_PARAM(emc_tppd);
	CASE_GET_SDRAM_PARAM(emc_ccdmw);

	CASE_GET_SDRAM_PARAM(emc_rd_rcd);
	/* Specifies the value for EMC_WR_RCD */
	CASE_GET_SDRAM_PARAM(emc_wr_rcd);
	/* Specifies the value for EMC_RRD */
	CASE_GET_SDRAM_PARAM(emc_rrd);
	/* Specifies the value for EMC_REXT */
	CASE_GET_SDRAM_PARAM(emc_rext);
	/* Specifies the value for EMC_WEXT */
	CASE_GET_SDRAM_PARAM(emc_wext);
	/* Specifies the value for EMC_WDV */
	CASE_GET_SDRAM_PARAM(emc_wdv);

	CASE_GET_SDRAM_PARAM(emc_wdv_chk);
	CASE_GET_SDRAM_PARAM(emc_wsv);
	CASE_GET_SDRAM_PARAM(emc_wev);

	/* Specifies the value for EMC_WDV_MASK */
	CASE_GET_SDRAM_PARAM(emc_wdv_mask);

	CASE_GET_SDRAM_PARAM(emc_ws_duration);
	CASE_GET_SDRAM_PARAM(emc_we_duration);

	/* Specifies the value for EMC_QUSE */
	CASE_GET_SDRAM_PARAM(emc_quse);
	/* Specifies the value for EMC_QUSE_WIDTH */
	CASE_GET_SDRAM_PARAM(emc_quse_width);
	/* Specifies the value for EMC_IBDLY */
	CASE_GET_SDRAM_PARAM(emc_ibdly);

	CASE_GET_SDRAM_PARAM(emc_obdly);

	/* Specifies the value for EMC_EINPUT */
	CASE_GET_SDRAM_PARAM(emc_einput);
	/* Specifies the value for EMC_EINPUT_DURATION */
	CASE_GET_SDRAM_PARAM(emc_einput_duration);
	/* Specifies the value for EMC_PUTERM_EXTRA */
	CASE_GET_SDRAM_PARAM(emc_puterm_extra);
	/* Specifies the value for EMC_PUTERM_WIDTH */
	CASE_GET_SDRAM_PARAM(emc_puterm_width);

	CASE_GET_SDRAM_PARAM(emc_qrst);
	CASE_GET_SDRAM_PARAM(emc_qsafe);
	CASE_GET_SDRAM_PARAM(emc_rdv);
	CASE_GET_SDRAM_PARAM(emc_rdv_mask);

	CASE_GET_SDRAM_PARAM(emc_rdv_early);
	CASE_GET_SDRAM_PARAM(emc_rdv_early_mask);

	/* Specifies the value for EMC_QPOP */
	CASE_GET_SDRAM_PARAM(emc_qpop);

	/* Specifies the value for EMC_REFRESH */
	CASE_GET_SDRAM_PARAM(emc_refresh);
	/* Specifies the value for EMC_BURST_REFRESH_NUM */
	CASE_GET_SDRAM_PARAM(emc_burst_refresh_num);
	/* Specifies the value for EMC_PRE_REFRESH_REQ_CNT */
	CASE_GET_SDRAM_PARAM(emc_prerefresh_req_cnt);
	/* Specifies the value for EMC_PDEX2WR */
	CASE_GET_SDRAM_PARAM(emc_pdex2wr);
	/* Specifies the value for EMC_PDEX2RD */
	CASE_GET_SDRAM_PARAM(emc_pdex2rd);
	/* Specifies the value for EMC_PCHG2PDEN */
	CASE_GET_SDRAM_PARAM(emc_pchg2pden);
	/* Specifies the value for EMC_ACT2PDEN */
	CASE_GET_SDRAM_PARAM(emc_act2pden);
	/* Specifies the value for EMC_AR2PDEN */
	CASE_GET_SDRAM_PARAM(emc_ar2pden);
	/* Specifies the value for EMC_RW2PDEN */
	CASE_GET_SDRAM_PARAM(emc_rw2pden);

	CASE_GET_SDRAM_PARAM(emc_cke2pden);
	CASE_GET_SDRAM_PARAM(emc_pdex2che);
	CASE_GET_SDRAM_PARAM(emc_pdex2mrr);

	/* Specifies the value for EMC_TXSR */
	CASE_GET_SDRAM_PARAM(emc_txsr);
	/* Specifies the value for EMC_TXSRDLL */
	CASE_GET_SDRAM_PARAM(emc_txsr_dll);
	/* Specifies the value for EMC_TCKE */
	CASE_GET_SDRAM_PARAM(emc_tcke);
	/* Specifies the value for EMC_TCKESR */
	CASE_GET_SDRAM_PARAM(emc_tckesr);
	/* Specifies the value for EMC_TPD */
	CASE_GET_SDRAM_PARAM(emc_tpd);
	/* Specifies the value for EMC_TFAW */
	CASE_GET_SDRAM_PARAM(emc_tfaw);
	/* Specifies the value for EMC_TRPAB */
	CASE_GET_SDRAM_PARAM(emc_trpab);
	/* Specifies the value for EMC_TCLKSTABLE */
	CASE_GET_SDRAM_PARAM(emc_tclkstable);
	/* Specifies the value for EMC_TCLKSTOP */
	CASE_GET_SDRAM_PARAM(emc_tclkstop);
	/* Specifies the value for EMC_TREFBW */
	CASE_GET_SDRAM_PARAM(emc_trefbw);

	/* FBIO configuration values */

	/* Specifies the value for EMC_FBIO_CFG5 */
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg5);
	/* Specifies the value for EMC_FBIO_CFG7 */
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg7);
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg8);

	/* Command mapping for CMD brick 0 */
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd0_0);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd0_1);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd0_2);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd1_0);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd1_1);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd1_2);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd2_0);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd2_1);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd2_2);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd3_0);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd3_1);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_cmd3_2);
	CASE_GET_SDRAM_PARAM(emc_cmd_mapping_byte);

	/* Specifies the value for EMC_FBIO_SPARE */
	CASE_GET_SDRAM_PARAM(emc_fbio_spare);

	/* Specifies the value for EMC_CFG_RSV */
	CASE_GET_SDRAM_PARAM(emc_cfg_rsv);

	/* MRS command values */

	/* Specifies the value for EMC_MRS */
	CASE_GET_SDRAM_PARAM(emc_mrs);
	/* Specifies the MP0 command to initialize mode registers */
	CASE_GET_SDRAM_PARAM(emc_emrs);
	/* Specifies the MP2 command to initialize mode registers */
	CASE_GET_SDRAM_PARAM(emc_emrs2);
	/* Specifies the MP3 command to initialize mode registers */
	CASE_GET_SDRAM_PARAM(emc_emrs3);
	/* Specifies the programming to LPDDR2 Mode Register 1 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw1);
	/* Specifies the programming to LPDDR2 Mode Register 2 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw2);
	/* Specifies the programming to LPDDR2 Mode Register 3 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw3);
	/* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw4);

	/* Specifies the programming to LPDDR4 Mode Register 3 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw6);
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw8);
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw9);
	/* Specifies the programming to LPDDR4 Mode Register 12 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw10);
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw12);
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw13);
	/* Specifies the programming to LPDDR4 Mode Register 22 at cold boot */
	CASE_GET_SDRAM_PARAM(emc_mrw14);

	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at cold boot
	 */
	CASE_GET_SDRAM_PARAM(emc_mrw_extra);
	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at warm boot
	 */
	CASE_GET_SDRAM_PARAM(emc_warm_boot_mrw_extra);
	/*
	 * Specify the enable of extra Mode Register programming at
	 * warm boot
	 */
	CASE_GET_SDRAM_PARAM(emc_warm_boot_extramode_reg_write_enable);
	/*
	 * Specify the enable of extra Mode Register programming at
	 * cold boot
	 */
	CASE_GET_SDRAM_PARAM(emc_extramode_reg_write_enable);

	/* Specifies the EMC_MRW reset command value */
	CASE_GET_SDRAM_PARAM(emc_mrw_reset_command);
	/* Specifies the EMC Reset wait time (in microseconds) */
	CASE_GET_SDRAM_PARAM(emc_mrw_reset_ninit_wait);
	/* Specifies the value for EMC_MRS_WAIT_CNT */
	CASE_GET_SDRAM_PARAM(emc_mrs_wait_cnt);
	/* Specifies the value for EMC_MRS_WAIT_CNT2 */
	CASE_GET_SDRAM_PARAM(emc_mrs_wait_cnt2);

	/* EMC miscellaneous configurations */

	/* Specifies the value for EMC_CFG */
	CASE_GET_SDRAM_PARAM(emc_cfg);
	/* Specifies the value for EMC_CFG_2 */
	CASE_GET_SDRAM_PARAM(emc_cfg2);
	/* Specifies the pipe bypass controls */
	CASE_GET_SDRAM_PARAM(emc_cfg_pipe);

	CASE_GET_SDRAM_PARAM(emc_cfg_pipe_clk);
	CASE_GET_SDRAM_PARAM(emc_fdpd_ctrl_cmd_no_ramp);
	CASE_GET_SDRAM_PARAM(emc_cfg_update);

	/* Specifies the value for EMC_DBG */
	CASE_GET_SDRAM_PARAM(emc_dbg);

	CASE_GET_SDRAM_PARAM(emc_dbg_write_mux);

	/* Specifies the value for EMC_CMDQ */
	CASE_GET_SDRAM_PARAM(emc_cmd_q);
	/* Specifies the value for EMC_MC2EMCQ */
	CASE_GET_SDRAM_PARAM(emc_mc2emc_q);
	/* Specifies the value for EMC_DYN_SELF_REF_CONTROL */
	CASE_GET_SDRAM_PARAM(emc_dyn_self_ref_control);

	/* Specifies the value for MEM_INIT_DONE */
	CASE_GET_SDRAM_PARAM(ahb_arbitration_xbar_ctrl_meminit_done);

	/* Specifies the value for EMC_CFG_DIG_DLL */
	CASE_GET_SDRAM_PARAM(emc_cfg_dig_dll);
	CASE_GET_SDRAM_PARAM(emc_cfg_dig_dll_1);

	/* Specifies the value for EMC_CFG_DIG_DLL_PERIOD */
	CASE_GET_SDRAM_PARAM(emc_cfg_dig_dll_period);
	/* Specifies the value of *DEV_SELECTN of various EMC registers */
	CASE_GET_SDRAM_PARAM(emc_dev_select);

	/* Specifies the value for EMC_SEL_DPD_CTRL */
	CASE_GET_SDRAM_PARAM(emc_sel_dpd_ctrl);

	/* Pads trimmer delays */
	CASE_GET_SDRAM_PARAM(emc_fdpd_ctrl_dq);
	CASE_GET_SDRAM_PARAM(emc_fdpd_ctrl_cmd);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_vref_dq_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_vref_dq_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_vref_dqs_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_vref_dqs_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_rxrt);
	CASE_GET_SDRAM_PARAM(emc_cfg_pipe1);
	CASE_GET_SDRAM_PARAM(emc_cfg_pipe2);

	/* Specifies the value for EMC_PMACRO_QUSE_DDLL_RANK0_0 */
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_5);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_5);

	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_5);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_5);

	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_5);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_5);

	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_3);

	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_short_cmd_0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_short_cmd_1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_short_cmd_2);

	/*
	 * Specifies the delay after asserting CKE pin during a WarmBoot0
	 * sequence (in microseconds)
	 */
	CASE_GET_SDRAM_PARAM(warm_boot_wait);

	/* Specifies the value for EMC_ODT_WRITE */
	CASE_GET_SDRAM_PARAM(emc_odt_write);

	/* Periodic ZQ calibration */

	/*
	 * Specifies the value for EMC_ZCAL_INTERVAL
	 * Value 0 disables ZQ calibration
	 */
	CASE_GET_SDRAM_PARAM(emc_zcal_interval);
	/* Specifies the value for EMC_ZCAL_WAIT_CNT */
	CASE_GET_SDRAM_PARAM(emc_zcal_wait_cnt);
	/* Specifies the value for EMC_ZCAL_MRW_CMD */
	CASE_GET_SDRAM_PARAM(emc_zcal_mrw_cmd);

	/* DRAM initialization sequence flow control */

	/* Specifies the MRS command value for resetting DLL */
	CASE_GET_SDRAM_PARAM(emc_mrs_reset_dll);
	/* Specifies the command for ZQ initialization of device 0 */
	CASE_GET_SDRAM_PARAM(emc_zcal_init_dev0);
	/* Specifies the command for ZQ initialization of device 1 */
	CASE_GET_SDRAM_PARAM(emc_zcal_init_dev1);
	/*
	 * Specifies the wait time after programming a ZQ initialization
	 * command (in microseconds)
	 */
	CASE_GET_SDRAM_PARAM(emc_zcal_init_wait);
	/*
	 * Specifies the enable for ZQ calibration at cold boot [bit 0]
	 * and warm boot [bit 1]
	 */
	CASE_GET_SDRAM_PARAM(emc_zcal_warm_cold_boot_enables);

	/*
	 * Specifies the MRW command to LPDDR2 for ZQ calibration
	 * on warmboot
	 */
	/* Is issued to both devices separately */
	CASE_GET_SDRAM_PARAM(emc_mrw_lpddr2zcal_warm_boot);
	/*
	 * Specifies the ZQ command to DDR3 for ZQ calibration on warmboot
	 * Is issued to both devices separately
	 */
	CASE_GET_SDRAM_PARAM(emc_zqcal_ddr3_warm_boot);

	CASE_GET_SDRAM_PARAM(emc_zqcal_lpddr4_warm_boot);

	/*
	 * Specifies the wait time for ZQ calibration on warmboot
	 * (in microseconds)
	 */
	CASE_GET_SDRAM_PARAM(emc_zcal_warm_boot_wait);
	/*
	 * Specifies the enable for DRAM Mode Register programming
	 * at warm boot
	 */
	CASE_GET_SDRAM_PARAM(emc_mrs_warm_boot_enable);
	/*
	 * Specifies the wait time after sending an MRS DLL reset command
	 * in microseconds)
	 */
	CASE_GET_SDRAM_PARAM(emc_mrs_reset_dll_wait);
	/* Specifies the extra MRS command to initialize mode registers */
	CASE_GET_SDRAM_PARAM(emc_mrs_extra);
	/* Specifies the extra MRS command at warm boot */
	CASE_GET_SDRAM_PARAM(emc_warm_boot_mrs_extra);
	/* Specifies the EMRS command to enable the DDR2 DLL */
	CASE_GET_SDRAM_PARAM(emc_emrs_ddr2_dll_enable);
	/* Specifies the MRS command to reset the DDR2 DLL */
	CASE_GET_SDRAM_PARAM(emc_mrs_ddr2_dll_reset);
	/* Specifies the EMRS command to set OCD calibration */
	CASE_GET_SDRAM_PARAM(emc_emrs_ddr2_ocd_calib);
	/*
	 * Specifies the wait between initializing DDR and setting OCD
	 * calibration (in microseconds)
	 */
	CASE_GET_SDRAM_PARAM(emc_ddr2_wait);
	/* Specifies the value for EMC_CLKEN_OVERRIDE */
	CASE_GET_SDRAM_PARAM(emc_clken_override);
	/*
	 * Specifies LOG2 of the extra refresh numbers after booting
	 * Program 0 to disable
	 */
	CASE_GET_SDRAM_PARAM(emc_extra_refresh_num);
	/* Specifies the master override for all EMC clocks */
	CASE_GET_SDRAM_PARAM(emc_clken_override_allwarm_boot);
	/* Specifies the master override for all MC clocks */
	CASE_GET_SDRAM_PARAM(mc_clken_override_allwarm_boot);
	/* Specifies digital dll period, choosing between 4 to 64 ms */
	CASE_GET_SDRAM_PARAM(emc_cfg_dig_dll_period_warm_boot);

	/* Pad controls */

	/* Specifies the value for PMC_VDDP_SEL */
	CASE_GET_SDRAM_PARAM(pmc_vddp_sel);
	/* Specifies the wait time after programming PMC_VDDP_SEL */
	CASE_GET_SDRAM_PARAM(pmc_vddp_sel_wait);
	/* Specifies the value for PMC_DDR_PWR */
	CASE_GET_SDRAM_PARAM(pmc_ddr_pwr);
	/* Specifies the value for PMC_DDR_CFG */
	CASE_GET_SDRAM_PARAM(pmc_ddr_cfg);
	/* Specifies the value for PMC_IO_DPD3_REQ */
	CASE_GET_SDRAM_PARAM(pmc_io_dpd3_req);
	/* Specifies the wait time after programming PMC_IO_DPD3_REQ */
	CASE_GET_SDRAM_PARAM(pmc_io_dpd3_req_wait);

	CASE_GET_SDRAM_PARAM(pmc_io_dpd4_req_wait);

	/* Specifies the value for PMC_REG_SHORT */
	CASE_GET_SDRAM_PARAM(pmc_reg_short);
	/* Specifies the value for PMC_NO_IOPOWER */
	CASE_GET_SDRAM_PARAM(pmc_no_io_power);

	CASE_GET_SDRAM_PARAM(pmc_ddr_ctrl_wait);
	CASE_GET_SDRAM_PARAM(pmc_ddr_ctrl);

	/* Specifies the value for EMC_ACPD_CONTROL */
	CASE_GET_SDRAM_PARAM(emc_acpd_control);

	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE0 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank0_byte0);
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE1 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank0_byte1);
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE2 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank0_byte2);
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE3 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank0_byte3);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE0 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank1_byte0);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE1 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank1_byte1);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE2 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank1_byte2);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE3 */
	CASE_GET_SDRAM_PARAM(emc_swizzle_rank1_byte3);

	/* Specifies the value for EMC_TXDSRVTTGEN */
	CASE_GET_SDRAM_PARAM(emc_txdsrvttgen);

	/* Specifies the value for EMC_DATA_BRLSHFT_0 */
	CASE_GET_SDRAM_PARAM(emc_data_brlshft0);
	CASE_GET_SDRAM_PARAM(emc_data_brlshft1);

	CASE_GET_SDRAM_PARAM(emc_dqs_brlshft0);
	CASE_GET_SDRAM_PARAM(emc_dqs_brlshft1);

	CASE_GET_SDRAM_PARAM(emc_cmd_brlshft0);
	CASE_GET_SDRAM_PARAM(emc_cmd_brlshft1);
	CASE_GET_SDRAM_PARAM(emc_cmd_brlshft2);
	CASE_GET_SDRAM_PARAM(emc_cmd_brlshft3);

	CASE_GET_SDRAM_PARAM(emc_quse_brlshft0);
	CASE_GET_SDRAM_PARAM(emc_quse_brlshft1);
	CASE_GET_SDRAM_PARAM(emc_quse_brlshft2);
	CASE_GET_SDRAM_PARAM(emc_quse_brlshft3);

	CASE_GET_SDRAM_PARAM(emc_dll_cfg0);
	CASE_GET_SDRAM_PARAM(emc_dll_cfg1);

	CASE_GET_SDRAM_PARAM(emc_pmc_scratch1);
	CASE_GET_SDRAM_PARAM(emc_pmc_scratch2);
	CASE_GET_SDRAM_PARAM(emc_pmc_scratch3);

	CASE_GET_SDRAM_PARAM(emc_pmacro_pad_cfg_ctrl);

	CASE_GET_SDRAM_PARAM(emc_pmacro_vttgen_ctrl0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_vttgen_ctrl1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_vttgen_ctrl2);

	CASE_GET_SDRAM_PARAM(emc_pmacro_brick_ctrl_rfu1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_brick_ctrl_fdpd);
	CASE_GET_SDRAM_PARAM(emc_pmacro_brick_ctrl_rfu2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_data_brick_ctrl_fdpd);
	CASE_GET_SDRAM_PARAM(emc_pmacro_bg_bias_ctrl0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_data_pad_rx_ctrl);
	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_pad_rx_ctrl);
	CASE_GET_SDRAM_PARAM(emc_pmacro_data_rx_term_mode);
	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_rx_term_mode);
	CASE_GET_SDRAM_PARAM(emc_pmacro_data_pad_tx_ctrl);
	CASE_GET_SDRAM_PARAM(emc_pmacro_common_pad_tx_ctrl);
	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_pad_tx_ctrl);
	CASE_GET_SDRAM_PARAM(emc_cfg3);

	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_pwrd0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_pwrd1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_pwrd2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_pwrd3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_pwrd4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_pwrd5);

	CASE_GET_SDRAM_PARAM(emc_config_sample_delay);

	CASE_GET_SDRAM_PARAM(emc_pmacro_brick_mapping0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_brick_mapping1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_brick_mapping2);

	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src2);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src3);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src4);
	CASE_GET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src5);

	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_bypass);

	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_pwrd0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_pwrd1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_ddll_pwrd2);

	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_ctrl0);
	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_ctrl1);
	CASE_GET_SDRAM_PARAM(emc_pmacro_cmd_ctrl2);

	/* DRAM size information */

	/* Specifies the value for MC_EMEM_ADR_CFG */
	CASE_GET_SDRAM_PARAM(mc_emem_adr_cfg);
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV0 */
	CASE_GET_SDRAM_PARAM(mc_emem_adr_cfg_dev0);
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV1 */
	CASE_GET_SDRAM_PARAM(mc_emem_adr_cfg_dev1);

	CASE_GET_SDRAM_PARAM(mc_emem_adr_cfg_channel_mask);

	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG0 */
	CASE_GET_SDRAM_PARAM(mc_emem_adr_cfg_bank_mask0);
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG1 */
	CASE_GET_SDRAM_PARAM(mc_emem_adr_cfg_bank_mask1);
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG2 */
	CASE_GET_SDRAM_PARAM(mc_emem_adr_cfg_bank_mask2);

	/*
	 * Specifies the value for MC_EMEM_CFG which holds the external memory
	 * size (in KBytes)
	 */
	CASE_GET_SDRAM_PARAM(mc_emem_cfg);

	/* MC arbitration configuration */

	/* Specifies the value for MC_EMEM_ARB_CFG */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_cfg);
	/* Specifies the value for MC_EMEM_ARB_OUTSTANDING_REQ */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_outstanding_req);

	CASE_GET_SDRAM_PARAM(emc_emem_arb_refpb_hp_ctrl);
	CASE_GET_SDRAM_PARAM(emc_emem_arb_refpb_bank_ctrl);

	/* Specifies the value for MC_EMEM_ARB_TIMING_RCD */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_rcd);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RP */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_rp);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RC */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_rc);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAS */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_ras);
	/* Specifies the value for MC_EMEM_ARB_TIMING_FAW */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_faw);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RRD */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_rrd);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAP2PRE */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_rap2pre);
	/* Specifies the value for MC_EMEM_ARB_TIMING_WAP2PRE */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_wap2pre);
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2R */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_r2r);
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2W */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_w2w);
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2W */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_r2w);
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2R */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_w2r);

	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_rfcpb);

	/* Specifies the value for MC_EMEM_ARB_DA_TURNS */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_da_turns);
	/* Specifies the value for MC_EMEM_ARB_DA_COVERS */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_da_covers);
	/* Specifies the value for MC_EMEM_ARB_MISC0 */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_misc0);
	/* Specifies the value for MC_EMEM_ARB_MISC1 */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_misc1);
	CASE_GET_SDRAM_PARAM(mc_emem_arb_misc2);

	/* Specifies the value for MC_EMEM_ARB_RING1_THROTTLE */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_ring1_throttle);
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_override);
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE_1 */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_override1);
	/* Specifies the value for MC_EMEM_ARB_RSV */
	CASE_GET_SDRAM_PARAM(mc_emem_arb_rsv);

	CASE_GET_SDRAM_PARAM(mc_da_cfg0);
	CASE_GET_SDRAM_PARAM(mc_emem_arb_timing_ccdmw);

	/* Specifies the value for MC_CLKEN_OVERRIDE */
	CASE_GET_SDRAM_PARAM(mc_clken_override);

	/* Specifies the value for MC_STAT_CONTROL */
	CASE_GET_SDRAM_PARAM(mc_stat_control);
	/* Specifies the value for MC_VIDEO_PROTECT_BOM */
	CASE_GET_SDRAM_PARAM(mc_video_protect_bom);
	/* Specifies the value for MC_VIDEO_PROTECT_BOM_ADR_HI */
	CASE_GET_SDRAM_PARAM(mc_video_protect_bom_adr_hi);
	/* Specifies the value for MC_VIDEO_PROTECT_SIZE_MB */
	CASE_GET_SDRAM_PARAM(mc_video_protect_size_mb);
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE */
	CASE_GET_SDRAM_PARAM(mc_video_protect_vpr_override);
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE1 */
	CASE_GET_SDRAM_PARAM(mc_video_protect_vpr_override1);
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_0 */
	CASE_GET_SDRAM_PARAM(mc_video_protect_gpu_override0);
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_1 */
	CASE_GET_SDRAM_PARAM(mc_video_protect_gpu_override1);
	/* Specifies the value for MC_SEC_CARVEOUT_BOM */
	CASE_GET_SDRAM_PARAM(mc_sec_carveout_bom);
	/* Specifies the value for MC_SEC_CARVEOUT_ADR_HI */
	CASE_GET_SDRAM_PARAM(mc_sec_carveout_adr_hi);
	/* Specifies the value for MC_SEC_CARVEOUT_SIZE_MB */
	CASE_GET_SDRAM_PARAM(mc_sec_carveout_size_mb);
	/* Specifies the value for MC_VIDEO_PROTECT_REG_CTRL.VIDEO_PROTECT_WRITE_ACCESS */
	CASE_GET_SDRAM_PARAM(mc_video_protect_write_access);
	/* Specifies the value for MC_SEC_CARVEOUT_REG_CTRL.SEC_CARVEOUT_WRITE_ACCESS */
	CASE_GET_SDRAM_PARAM(mc_sec_carveout_protect_write_access);

	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_bom);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_bom_hi);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_size_128kb);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout1_cfg0);

	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_bom);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_bom_hi);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_size_128kb);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout2_cfg0);

	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_bom);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_bom_hi);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_size_128kb);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout3_cfg0);

	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_bom);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_bom_hi);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_size_128kb);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout4_cfg0);

	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_bom);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_bom_hi);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_size_128kb);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access0);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access1);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access2);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access3);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access4);
	CASE_GET_SDRAM_PARAM(mc_generalized_carveout5_cfg0);

	/* Specifies enable for CA training */
	CASE_GET_SDRAM_PARAM(emc_ca_training_enable);
	/* Set if bit 6 select is greater than bit 7 select); uses aremc.spec packet SWIZZLE_BIT6_GT_BIT7 */
	CASE_GET_SDRAM_PARAM(swizzle_rank_byte_encode);
	/* Specifies enable and offset for patched boot rom write */
	CASE_GET_SDRAM_PARAM(boot_rom_patch_control);
	/* Specifies data for patched boot rom write */
	CASE_GET_SDRAM_PARAM(boot_rom_patch_data);

	/* Specifies the value for MC_MTS_CARVEOUT_BOM */
	CASE_GET_SDRAM_PARAM(mc_mts_carveout_bom);
	/* Specifies the value for MC_MTS_CARVEOUT_ADR_HI */
	CASE_GET_SDRAM_PARAM(mc_mts_carveout_adr_hi);
	/* Specifies the value for MC_MTS_CARVEOUT_SIZE_MB */
	CASE_GET_SDRAM_PARAM(mc_mts_carveout_size_mb);
	/* Specifies the value for MC_MTS_CARVEOUT_REG_CTRL */
	CASE_GET_SDRAM_PARAM(mc_mts_carveout_reg_ctrl);
	DEFAULT();
	}
	return 0;
}

int
t210_set_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t value)
{
	nvboot_sdram_params *params;
	nvboot_config_table *bct = NULL;

	bct = (nvboot_config_table *)(context->bct);
	assert(context != NULL);
	assert(bct != NULL);
	params = &(bct->sdram_params[index]);
	/* Update the number of SDRAM parameter sets. */
	bct->num_sdram_sets = NV_MAX(bct->num_sdram_sets, index + 1);

	switch (token) {
	/* Specifies the type of memory device */
	CASE_SET_SDRAM_PARAM(memory_type);

	/* MC/EMC clock source configuration */

	/* Specifies the M value for PllM */
	CASE_SET_SDRAM_PARAM(pllm_input_divider);
	/* Specifies the N value for PllM */
	CASE_SET_SDRAM_PARAM(pllm_feedback_divider);
	/* Specifies the time to wait for PLLM to lock (in microseconds) */
	CASE_SET_SDRAM_PARAM(pllm_stable_time);
	/* Specifies misc. control bits */
	CASE_SET_SDRAM_PARAM(pllm_setup_control);
	/* Specifies the P value for PLLM */
	CASE_SET_SDRAM_PARAM(pllm_post_divider);
	/* Specifies value for Charge Pump Gain Control */
	CASE_SET_SDRAM_PARAM(pllm_kcp);
	/* Specifies VCO gain */
	CASE_SET_SDRAM_PARAM(pllm_kvco);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare0);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare1);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare2);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare3);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare4);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare5);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare6);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare7);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare8);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare9);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare10);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare11);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare12);
	/* Spare BCT param */
	CASE_SET_SDRAM_PARAM(emc_bct_spare13);

	/* Defines EMC_2X_CLK_SRC, EMC_2X_CLK_DIVISOR, EMC_INVERT_DCD */
	CASE_SET_SDRAM_PARAM(emc_clock_source);
	CASE_SET_SDRAM_PARAM(emc_clock_source_dll);

	/* Defines possible override for PLLLM_MISC2 */
	CASE_SET_SDRAM_PARAM(clk_rst_pllm_misc20_override);
	/* enables override for PLLLM_MISC2 */
	CASE_SET_SDRAM_PARAM(clk_rst_pllm_misc20_override_enable);
	/* defines CLK_ENB_MC1 in register clk_rst_controller_clk_enb_w_clr */
	CASE_SET_SDRAM_PARAM(clear_clock2_mc1);

	/* Auto-calibration of EMC pads */

	/* Specifies the value for EMC_AUTO_CAL_INTERVAL */
	CASE_SET_SDRAM_PARAM(emc_auto_cal_interval);
	/*
	 * Specifies the value for EMC_AUTO_CAL_CONFIG
	 * Note: Trigger bits are set by the SDRAM code.
	 */
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config);

	/* Specifies the value for EMC_AUTO_CAL_CONFIG2 */
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config2);

	/* Specifies the value for EMC_AUTO_CAL_CONFIG3 */
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config3);

	CASE_SET_SDRAM_PARAM(emc_auto_cal_config4);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config5);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config6);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config7);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config8);
	/* Specifies the value for EMC_AUTO_CAL_VREF_SEL_0 */
	CASE_SET_SDRAM_PARAM(emc_auto_cal_vref_sel0);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_vref_sel1);

	/* Specifies the value for EMC_AUTO_CAL_CHANNEL */
	CASE_SET_SDRAM_PARAM(emc_auto_cal_channel);

	/* Specifies the value for EMC_PMACRO_AUTOCAL_CFG_0 */
	CASE_SET_SDRAM_PARAM(emc_pmacro_auto_cal_cfg0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_auto_cal_cfg1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_auto_cal_cfg2);

	CASE_SET_SDRAM_PARAM(emc_pmacro_rx_term);
	CASE_SET_SDRAM_PARAM(emc_pmacro_dq_tx_drive);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ca_tx_drive);
	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_tx_drive);
	CASE_SET_SDRAM_PARAM(emc_pmacro_auto_cal_common);
	CASE_SET_SDRAM_PARAM(emc_pmacro_zcrtl);

	/*
	 * Specifies the time for the calibration
	 * to stabilize (in microseconds)
	 */
	CASE_SET_SDRAM_PARAM(emc_auto_cal_wait);

	CASE_SET_SDRAM_PARAM(emc_xm2_comp_pad_ctrl);
	CASE_SET_SDRAM_PARAM(emc_xm2_comp_pad_ctrl2);
	CASE_SET_SDRAM_PARAM(emc_xm2_comp_pad_ctrl3);

	/*
	 * DRAM size information
	 * Specifies the value for EMC_ADR_CFG
	 */
	CASE_SET_SDRAM_PARAM(emc_adr_cfg);

	/*
	 * Specifies the time to wait after asserting pin
	 * CKE (in microseconds)
	 */
	CASE_SET_SDRAM_PARAM(emc_pin_program_wait);
	/* Specifies the extra delay before/after pin RESET/CKE command */
	CASE_SET_SDRAM_PARAM(emc_pin_extra_wait);

	CASE_SET_SDRAM_PARAM(emc_pin_gpio_enable);
	CASE_SET_SDRAM_PARAM(emc_pin_gpio);

	/*
	 * Specifies the extra delay after the first writing
	 * of EMC_TIMING_CONTROL
	 */
	CASE_SET_SDRAM_PARAM(emc_timing_control_wait);

	/* Timing parameters required for the SDRAM */

	/* Specifies the value for EMC_RC */
	CASE_SET_SDRAM_PARAM(emc_rc);
	/* Specifies the value for EMC_RFC */
	CASE_SET_SDRAM_PARAM(emc_rfc);

	CASE_SET_SDRAM_PARAM(emc_rfc_pb);
	CASE_SET_SDRAM_PARAM(emc_ref_ctrl2);

	/* Specifies the value for EMC_RFC_SLR */
	CASE_SET_SDRAM_PARAM(emc_rfc_slr);
	/* Specifies the value for EMC_RAS */
	CASE_SET_SDRAM_PARAM(emc_ras);
	/* Specifies the value for EMC_RP */
	CASE_SET_SDRAM_PARAM(emc_rp);
	/* Specifies the value for EMC_R2R */
	CASE_SET_SDRAM_PARAM(emc_r2r);
	/* Specifies the value for EMC_W2W */
	CASE_SET_SDRAM_PARAM(emc_w2w);
	/* Specifies the value for EMC_R2W */
	CASE_SET_SDRAM_PARAM(emc_r2w);
	/* Specifies the value for EMC_W2R */
	CASE_SET_SDRAM_PARAM(emc_w2r);
	/* Specifies the value for EMC_R2P */
	CASE_SET_SDRAM_PARAM(emc_r2p);
	/* Specifies the value for EMC_W2P */
	CASE_SET_SDRAM_PARAM(emc_w2p);
	/* Specifies the value for EMC_RD_RCD */

	CASE_SET_SDRAM_PARAM(emc_tppd);
	CASE_SET_SDRAM_PARAM(emc_ccdmw);

	CASE_SET_SDRAM_PARAM(emc_rd_rcd);
	/* Specifies the value for EMC_WR_RCD */
	CASE_SET_SDRAM_PARAM(emc_wr_rcd);
	/* Specifies the value for EMC_RRD */
	CASE_SET_SDRAM_PARAM(emc_rrd);
	/* Specifies the value for EMC_REXT */
	CASE_SET_SDRAM_PARAM(emc_rext);
	/* Specifies the value for EMC_WEXT */
	CASE_SET_SDRAM_PARAM(emc_wext);
	/* Specifies the value for EMC_WDV */
	CASE_SET_SDRAM_PARAM(emc_wdv);

	CASE_SET_SDRAM_PARAM(emc_wdv_chk);
	CASE_SET_SDRAM_PARAM(emc_wsv);
	CASE_SET_SDRAM_PARAM(emc_wev);

	/* Specifies the value for EMC_WDV_MASK */
	CASE_SET_SDRAM_PARAM(emc_wdv_mask);

	CASE_SET_SDRAM_PARAM(emc_ws_duration);
	CASE_SET_SDRAM_PARAM(emc_we_duration);

	/* Specifies the value for EMC_QUSE */
	CASE_SET_SDRAM_PARAM(emc_quse);
	/* Specifies the value for EMC_QUSE_WIDTH */
	CASE_SET_SDRAM_PARAM(emc_quse_width);
	/* Specifies the value for EMC_IBDLY */
	CASE_SET_SDRAM_PARAM(emc_ibdly);

	CASE_SET_SDRAM_PARAM(emc_obdly);

	/* Specifies the value for EMC_EINPUT */
	CASE_SET_SDRAM_PARAM(emc_einput);
	/* Specifies the value for EMC_EINPUT_DURATION */
	CASE_SET_SDRAM_PARAM(emc_einput_duration);
	/* Specifies the value for EMC_PUTERM_EXTRA */
	CASE_SET_SDRAM_PARAM(emc_puterm_extra);
	/* Specifies the value for EMC_PUTERM_WIDTH */
	CASE_SET_SDRAM_PARAM(emc_puterm_width);

	CASE_SET_SDRAM_PARAM(emc_qrst);
	CASE_SET_SDRAM_PARAM(emc_qsafe);
	CASE_SET_SDRAM_PARAM(emc_rdv);
	CASE_SET_SDRAM_PARAM(emc_rdv_mask);

	CASE_SET_SDRAM_PARAM(emc_rdv_early);
	CASE_SET_SDRAM_PARAM(emc_rdv_early_mask);

	/* Specifies the value for EMC_QPOP */
	CASE_SET_SDRAM_PARAM(emc_qpop);

	/* Specifies the value for EMC_REFRESH */
	CASE_SET_SDRAM_PARAM(emc_refresh);
	/* Specifies the value for EMC_BURST_REFRESH_NUM */
	CASE_SET_SDRAM_PARAM(emc_burst_refresh_num);
	/* Specifies the value for EMC_PRE_REFRESH_REQ_CNT */
	CASE_SET_SDRAM_PARAM(emc_prerefresh_req_cnt);
	/* Specifies the value for EMC_PDEX2WR */
	CASE_SET_SDRAM_PARAM(emc_pdex2wr);
	/* Specifies the value for EMC_PDEX2RD */
	CASE_SET_SDRAM_PARAM(emc_pdex2rd);
	/* Specifies the value for EMC_PCHG2PDEN */
	CASE_SET_SDRAM_PARAM(emc_pchg2pden);
	/* Specifies the value for EMC_ACT2PDEN */
	CASE_SET_SDRAM_PARAM(emc_act2pden);
	/* Specifies the value for EMC_AR2PDEN */
	CASE_SET_SDRAM_PARAM(emc_ar2pden);
	/* Specifies the value for EMC_RW2PDEN */
	CASE_SET_SDRAM_PARAM(emc_rw2pden);

	CASE_SET_SDRAM_PARAM(emc_cke2pden);
	CASE_SET_SDRAM_PARAM(emc_pdex2che);
	CASE_SET_SDRAM_PARAM(emc_pdex2mrr);

	/* Specifies the value for EMC_TXSR */
	CASE_SET_SDRAM_PARAM(emc_txsr);
	/* Specifies the value for EMC_TXSRDLL */
	CASE_SET_SDRAM_PARAM(emc_txsr_dll);
	/* Specifies the value for EMC_TCKE */
	CASE_SET_SDRAM_PARAM(emc_tcke);
	/* Specifies the value for EMC_TCKESR */
	CASE_SET_SDRAM_PARAM(emc_tckesr);
	/* Specifies the value for EMC_TPD */
	CASE_SET_SDRAM_PARAM(emc_tpd);
	/* Specifies the value for EMC_TFAW */
	CASE_SET_SDRAM_PARAM(emc_tfaw);
	/* Specifies the value for EMC_TRPAB */
	CASE_SET_SDRAM_PARAM(emc_trpab);
	/* Specifies the value for EMC_TCLKSTABLE */
	CASE_SET_SDRAM_PARAM(emc_tclkstable);
	/* Specifies the value for EMC_TCLKSTOP */
	CASE_SET_SDRAM_PARAM(emc_tclkstop);
	/* Specifies the value for EMC_TREFBW */
	CASE_SET_SDRAM_PARAM(emc_trefbw);

	/* FBIO configuration values */

	/* Specifies the value for EMC_FBIO_CFG5 */
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg5);
	/* Specifies the value for EMC_FBIO_CFG7 */
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg7);
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg8);

	/* Command mapping for CMD brick 0 */
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd0_0);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd0_1);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd0_2);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd1_0);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd1_1);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd1_2);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd2_0);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd2_1);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd2_2);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd3_0);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd3_1);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_cmd3_2);
	CASE_SET_SDRAM_PARAM(emc_cmd_mapping_byte);

	/* Specifies the value for EMC_FBIO_SPARE */
	CASE_SET_SDRAM_PARAM(emc_fbio_spare);

	/* Specifies the value for EMC_CFG_RSV */
	CASE_SET_SDRAM_PARAM(emc_cfg_rsv);

	/* MRS command values */

	/* Specifies the value for EMC_MRS */
	CASE_SET_SDRAM_PARAM(emc_mrs);
	/* Specifies the MP0 command to initialize mode registers */
	CASE_SET_SDRAM_PARAM(emc_emrs);
	/* Specifies the MP2 command to initialize mode registers */
	CASE_SET_SDRAM_PARAM(emc_emrs2);
	/* Specifies the MP3 command to initialize mode registers */
	CASE_SET_SDRAM_PARAM(emc_emrs3);
	/* Specifies the programming to LPDDR2 Mode Register 1 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw1);
	/* Specifies the programming to LPDDR2 Mode Register 2 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw2);
	/* Specifies the programming to LPDDR2 Mode Register 3 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw3);
	/* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw4);

	/* Specifies the programming to LPDDR4 Mode Register 3 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw6);
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw8);
	/* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw9);
	/* Specifies the programming to LPDDR4 Mode Register 12 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw10);
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw12);
	/* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw13);
	/* Specifies the programming to LPDDR4 Mode Register 22 at cold boot */
	CASE_SET_SDRAM_PARAM(emc_mrw14);

	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at cold boot
	 */
	CASE_SET_SDRAM_PARAM(emc_mrw_extra);
	/*
	 * Specifies the programming to extra LPDDR2 Mode Register
	 * at warm boot
	 */
	CASE_SET_SDRAM_PARAM(emc_warm_boot_mrw_extra);
	/*
	 * Specify the enable of extra Mode Register programming at
	 * warm boot
	 */
	CASE_SET_SDRAM_PARAM(emc_warm_boot_extramode_reg_write_enable);
	/*
	 * Specify the enable of extra Mode Register programming at
	 * cold boot
	 */
	CASE_SET_SDRAM_PARAM(emc_extramode_reg_write_enable);

	/* Specifies the EMC_MRW reset command value */
	CASE_SET_SDRAM_PARAM(emc_mrw_reset_command);
	/* Specifies the EMC Reset wait time (in microseconds) */
	CASE_SET_SDRAM_PARAM(emc_mrw_reset_ninit_wait);
	/* Specifies the value for EMC_MRS_WAIT_CNT */
	CASE_SET_SDRAM_PARAM(emc_mrs_wait_cnt);
	/* Specifies the value for EMC_MRS_WAIT_CNT2 */
	CASE_SET_SDRAM_PARAM(emc_mrs_wait_cnt2);

	/* EMC miscellaneous configurations */

	/* Specifies the value for EMC_CFG */
	CASE_SET_SDRAM_PARAM(emc_cfg);
	/* Specifies the value for EMC_CFG_2 */
	CASE_SET_SDRAM_PARAM(emc_cfg2);
	/* Specifies the pipe bypass controls */
	CASE_SET_SDRAM_PARAM(emc_cfg_pipe);

	CASE_SET_SDRAM_PARAM(emc_cfg_pipe_clk);
	CASE_SET_SDRAM_PARAM(emc_fdpd_ctrl_cmd_no_ramp);
	CASE_SET_SDRAM_PARAM(emc_cfg_update);

	/* Specifies the value for EMC_DBG */
	CASE_SET_SDRAM_PARAM(emc_dbg);

	CASE_SET_SDRAM_PARAM(emc_dbg_write_mux);

	/* Specifies the value for EMC_CMDQ */
	CASE_SET_SDRAM_PARAM(emc_cmd_q);
	/* Specifies the value for EMC_MC2EMCQ */
	CASE_SET_SDRAM_PARAM(emc_mc2emc_q);
	/* Specifies the value for EMC_DYN_SELF_REF_CONTROL */
	CASE_SET_SDRAM_PARAM(emc_dyn_self_ref_control);

	/* Specifies the value for MEM_INIT_DONE */
	CASE_SET_SDRAM_PARAM(ahb_arbitration_xbar_ctrl_meminit_done);

	/* Specifies the value for EMC_CFG_DIG_DLL */
	CASE_SET_SDRAM_PARAM(emc_cfg_dig_dll);
	CASE_SET_SDRAM_PARAM(emc_cfg_dig_dll_1);

	/* Specifies the value for EMC_CFG_DIG_DLL_PERIOD */
	CASE_SET_SDRAM_PARAM(emc_cfg_dig_dll_period);
	/* Specifies the value of *DEV_SELECTN of various EMC registers */
	CASE_SET_SDRAM_PARAM(emc_dev_select);

	/* Specifies the value for EMC_SEL_DPD_CTRL */
	CASE_SET_SDRAM_PARAM(emc_sel_dpd_ctrl);

	/* Pads trimmer delays */
	CASE_SET_SDRAM_PARAM(emc_fdpd_ctrl_dq);
	CASE_SET_SDRAM_PARAM(emc_fdpd_ctrl_cmd);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_vref_dq_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_vref_dq_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_vref_dqs_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_vref_dqs_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_rxrt);
	CASE_SET_SDRAM_PARAM(emc_cfg_pipe1);
	CASE_SET_SDRAM_PARAM(emc_cfg_pipe2);

	/* Specifies the value for EMC_PMACRO_QUSE_DDLL_RANK0_0 */
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank0_5);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_quse_ddll_rank1_5);

	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank0_5);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dq_rank1_5);

	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank0_5);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ob_ddll_long_dqs_rank1_5);

	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank0_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ib_ddll_long_dqs_rank1_3);

	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_long_cmd_4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_short_cmd_0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_short_cmd_1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_short_cmd_2);

	/*
	 * Specifies the delay after asserting CKE pin during a WarmBoot0
	 * sequence (in microseconds)
	 */
	CASE_SET_SDRAM_PARAM(warm_boot_wait);

	/* Specifies the value for EMC_ODT_WRITE */
	CASE_SET_SDRAM_PARAM(emc_odt_write);

	/* Periodic ZQ calibration */

	/*
	 * Specifies the value for EMC_ZCAL_INTERVAL
	 * Value 0 disables ZQ calibration
	 */
	CASE_SET_SDRAM_PARAM(emc_zcal_interval);
	/* Specifies the value for EMC_ZCAL_WAIT_CNT */
	CASE_SET_SDRAM_PARAM(emc_zcal_wait_cnt);
	/* Specifies the value for EMC_ZCAL_MRW_CMD */
	CASE_SET_SDRAM_PARAM(emc_zcal_mrw_cmd);

	/* DRAM initialization sequence flow control */

	/* Specifies the MRS command value for resetting DLL */
	CASE_SET_SDRAM_PARAM(emc_mrs_reset_dll);
	/* Specifies the command for ZQ initialization of device 0 */
	CASE_SET_SDRAM_PARAM(emc_zcal_init_dev0);
	/* Specifies the command for ZQ initialization of device 1 */
	CASE_SET_SDRAM_PARAM(emc_zcal_init_dev1);
	/*
	 * Specifies the wait time after programming a ZQ initialization
	 * command (in microseconds)
	 */
	CASE_SET_SDRAM_PARAM(emc_zcal_init_wait);
	/*
	 * Specifies the enable for ZQ calibration at cold boot [bit 0]
	 * and warm boot [bit 1]
	 */
	CASE_SET_SDRAM_PARAM(emc_zcal_warm_cold_boot_enables);

	/*
	 * Specifies the MRW command to LPDDR2 for ZQ calibration
	 * on warmboot
	 */
	/* Is issued to both devices separately */
	CASE_SET_SDRAM_PARAM(emc_mrw_lpddr2zcal_warm_boot);
	/*
	 * Specifies the ZQ command to DDR3 for ZQ calibration on warmboot
	 * Is issued to both devices separately
	 */
	CASE_SET_SDRAM_PARAM(emc_zqcal_ddr3_warm_boot);

	CASE_SET_SDRAM_PARAM(emc_zqcal_lpddr4_warm_boot);

	/*
	 * Specifies the wait time for ZQ calibration on warmboot
	 * (in microseconds)
	 */
	CASE_SET_SDRAM_PARAM(emc_zcal_warm_boot_wait);
	/*
	 * Specifies the enable for DRAM Mode Register programming
	 * at warm boot
	 */
	CASE_SET_SDRAM_PARAM(emc_mrs_warm_boot_enable);
	/*
	 * Specifies the wait time after sending an MRS DLL reset command
	 * in microseconds)
	 */
	CASE_SET_SDRAM_PARAM(emc_mrs_reset_dll_wait);
	/* Specifies the extra MRS command to initialize mode registers */
	CASE_SET_SDRAM_PARAM(emc_mrs_extra);
	/* Specifies the extra MRS command at warm boot */
	CASE_SET_SDRAM_PARAM(emc_warm_boot_mrs_extra);
	/* Specifies the EMRS command to enable the DDR2 DLL */
	CASE_SET_SDRAM_PARAM(emc_emrs_ddr2_dll_enable);
	/* Specifies the MRS command to reset the DDR2 DLL */
	CASE_SET_SDRAM_PARAM(emc_mrs_ddr2_dll_reset);
	/* Specifies the EMRS command to set OCD calibration */
	CASE_SET_SDRAM_PARAM(emc_emrs_ddr2_ocd_calib);
	/*
	 * Specifies the wait between initializing DDR and setting OCD
	 * calibration (in microseconds)
	 */
	CASE_SET_SDRAM_PARAM(emc_ddr2_wait);
	/* Specifies the value for EMC_CLKEN_OVERRIDE */
	CASE_SET_SDRAM_PARAM(emc_clken_override);
	/*
	 * Specifies LOG2 of the extra refresh numbers after booting
	 * Program 0 to disable
	 */
	CASE_SET_SDRAM_PARAM(emc_extra_refresh_num);
	/* Specifies the master override for all EMC clocks */
	CASE_SET_SDRAM_PARAM(emc_clken_override_allwarm_boot);
	/* Specifies the master override for all MC clocks */
	CASE_SET_SDRAM_PARAM(mc_clken_override_allwarm_boot);
	/* Specifies digital dll period, choosing between 4 to 64 ms */
	CASE_SET_SDRAM_PARAM(emc_cfg_dig_dll_period_warm_boot);

	/* Pad controls */

	/* Specifies the value for PMC_VDDP_SEL */
	CASE_SET_SDRAM_PARAM(pmc_vddp_sel);
	/* Specifies the wait time after programming PMC_VDDP_SEL */
	CASE_SET_SDRAM_PARAM(pmc_vddp_sel_wait);
	/* Specifies the value for PMC_DDR_PWR */
	CASE_SET_SDRAM_PARAM(pmc_ddr_pwr);
	/* Specifies the value for PMC_DDR_CFG */
	CASE_SET_SDRAM_PARAM(pmc_ddr_cfg);
	/* Specifies the value for PMC_IO_DPD3_REQ */
	CASE_SET_SDRAM_PARAM(pmc_io_dpd3_req);
	/* Specifies the wait time after programming PMC_IO_DPD3_REQ */
	CASE_SET_SDRAM_PARAM(pmc_io_dpd3_req_wait);

	CASE_SET_SDRAM_PARAM(pmc_io_dpd4_req_wait);

	/* Specifies the value for PMC_REG_SHORT */
	CASE_SET_SDRAM_PARAM(pmc_reg_short);
	/* Specifies the value for PMC_NO_IOPOWER */
	CASE_SET_SDRAM_PARAM(pmc_no_io_power);

	CASE_SET_SDRAM_PARAM(pmc_ddr_ctrl_wait);
	CASE_SET_SDRAM_PARAM(pmc_ddr_ctrl);

	/* Specifies the value for EMC_ACPD_CONTROL */
	CASE_SET_SDRAM_PARAM(emc_acpd_control);

	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE0 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank0_byte0);
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE1 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank0_byte1);
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE2 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank0_byte2);
	/* Specifies the value for EMC_SWIZZLE_RANK0_BYTE3 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank0_byte3);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE0 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank1_byte0);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE1 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank1_byte1);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE2 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank1_byte2);
	/* Specifies the value for EMC_SWIZZLE_RANK1_BYTE3 */
	CASE_SET_SDRAM_PARAM(emc_swizzle_rank1_byte3);

	/* Specifies the value for EMC_TXDSRVTTGEN */
	CASE_SET_SDRAM_PARAM(emc_txdsrvttgen);

	/* Specifies the value for EMC_DATA_BRLSHFT_0 */
	CASE_SET_SDRAM_PARAM(emc_data_brlshft0);
	CASE_SET_SDRAM_PARAM(emc_data_brlshft1);

	CASE_SET_SDRAM_PARAM(emc_dqs_brlshft0);
	CASE_SET_SDRAM_PARAM(emc_dqs_brlshft1);

	CASE_SET_SDRAM_PARAM(emc_cmd_brlshft0);
	CASE_SET_SDRAM_PARAM(emc_cmd_brlshft1);
	CASE_SET_SDRAM_PARAM(emc_cmd_brlshft2);
	CASE_SET_SDRAM_PARAM(emc_cmd_brlshft3);

	CASE_SET_SDRAM_PARAM(emc_quse_brlshft0);
	CASE_SET_SDRAM_PARAM(emc_quse_brlshft1);
	CASE_SET_SDRAM_PARAM(emc_quse_brlshft2);
	CASE_SET_SDRAM_PARAM(emc_quse_brlshft3);

	CASE_SET_SDRAM_PARAM(emc_dll_cfg0);
	CASE_SET_SDRAM_PARAM(emc_dll_cfg1);

	CASE_SET_SDRAM_PARAM(emc_pmc_scratch1);
	CASE_SET_SDRAM_PARAM(emc_pmc_scratch2);
	CASE_SET_SDRAM_PARAM(emc_pmc_scratch3);

	CASE_SET_SDRAM_PARAM(emc_pmacro_pad_cfg_ctrl);

	CASE_SET_SDRAM_PARAM(emc_pmacro_vttgen_ctrl0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_vttgen_ctrl1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_vttgen_ctrl2);

	CASE_SET_SDRAM_PARAM(emc_pmacro_brick_ctrl_rfu1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_brick_ctrl_fdpd);
	CASE_SET_SDRAM_PARAM(emc_pmacro_brick_ctrl_rfu2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_data_brick_ctrl_fdpd);
	CASE_SET_SDRAM_PARAM(emc_pmacro_bg_bias_ctrl0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_data_pad_rx_ctrl);
	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_pad_rx_ctrl);
	CASE_SET_SDRAM_PARAM(emc_pmacro_data_rx_term_mode);
	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_rx_term_mode);
	CASE_SET_SDRAM_PARAM(emc_pmacro_data_pad_tx_ctrl);
	CASE_SET_SDRAM_PARAM(emc_pmacro_common_pad_tx_ctrl);
	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_pad_tx_ctrl);
	CASE_SET_SDRAM_PARAM(emc_cfg3);

	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_pwrd0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_pwrd1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_pwrd2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_pwrd3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_pwrd4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_pwrd5);

	CASE_SET_SDRAM_PARAM(emc_config_sample_delay);

	CASE_SET_SDRAM_PARAM(emc_pmacro_brick_mapping0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_brick_mapping1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_brick_mapping2);

	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src2);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src3);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src4);
	CASE_SET_SDRAM_PARAM(emc_pmacro_tx_sel_clk_src5);

	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_bypass);

	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_pwrd0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_pwrd1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_ddll_pwrd2);

	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_ctrl0);
	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_ctrl1);
	CASE_SET_SDRAM_PARAM(emc_pmacro_cmd_ctrl2);

	/* DRAM size information */

	/* Specifies the value for MC_EMEM_ADR_CFG */
	CASE_SET_SDRAM_PARAM(mc_emem_adr_cfg);
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV0 */
	CASE_SET_SDRAM_PARAM(mc_emem_adr_cfg_dev0);
	/* Specifies the value for MC_EMEM_ADR_CFG_DEV1 */
	CASE_SET_SDRAM_PARAM(mc_emem_adr_cfg_dev1);

	CASE_SET_SDRAM_PARAM(mc_emem_adr_cfg_channel_mask);

	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG0 */
	CASE_SET_SDRAM_PARAM(mc_emem_adr_cfg_bank_mask0);
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG1 */
	CASE_SET_SDRAM_PARAM(mc_emem_adr_cfg_bank_mask1);
	/* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG2 */
	CASE_SET_SDRAM_PARAM(mc_emem_adr_cfg_bank_mask2);

	/*
	 * Specifies the value for MC_EMEM_CFG which holds the external memory
	 * size (in KBytes)
	 */
	CASE_SET_SDRAM_PARAM(mc_emem_cfg);

	/* MC arbitration configuration */

	/* Specifies the value for MC_EMEM_ARB_CFG */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_cfg);
	/* Specifies the value for MC_EMEM_ARB_OUTSTANDING_REQ */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_outstanding_req);

	CASE_SET_SDRAM_PARAM(emc_emem_arb_refpb_hp_ctrl);
	CASE_SET_SDRAM_PARAM(emc_emem_arb_refpb_bank_ctrl);

	/* Specifies the value for MC_EMEM_ARB_TIMING_RCD */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_rcd);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RP */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_rp);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RC */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_rc);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAS */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_ras);
	/* Specifies the value for MC_EMEM_ARB_TIMING_FAW */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_faw);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RRD */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_rrd);
	/* Specifies the value for MC_EMEM_ARB_TIMING_RAP2PRE */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_rap2pre);
	/* Specifies the value for MC_EMEM_ARB_TIMING_WAP2PRE */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_wap2pre);
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2R */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_r2r);
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2W */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_w2w);
	/* Specifies the value for MC_EMEM_ARB_TIMING_R2W */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_r2w);
	/* Specifies the value for MC_EMEM_ARB_TIMING_W2R */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_w2r);

	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_rfcpb);

	/* Specifies the value for MC_EMEM_ARB_DA_TURNS */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_da_turns);
	/* Specifies the value for MC_EMEM_ARB_DA_COVERS */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_da_covers);
	/* Specifies the value for MC_EMEM_ARB_MISC0 */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_misc0);
	/* Specifies the value for MC_EMEM_ARB_MISC1 */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_misc1);
	CASE_SET_SDRAM_PARAM(mc_emem_arb_misc2);

	/* Specifies the value for MC_EMEM_ARB_RING1_THROTTLE */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_ring1_throttle);
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_override);
	/* Specifies the value for MC_EMEM_ARB_OVERRIDE_1 */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_override1);
	/* Specifies the value for MC_EMEM_ARB_RSV */
	CASE_SET_SDRAM_PARAM(mc_emem_arb_rsv);

	CASE_SET_SDRAM_PARAM(mc_da_cfg0);
	CASE_SET_SDRAM_PARAM(mc_emem_arb_timing_ccdmw);

	/* Specifies the value for MC_CLKEN_OVERRIDE */
	CASE_SET_SDRAM_PARAM(mc_clken_override);

	/* Specifies the value for MC_STAT_CONTROL */
	CASE_SET_SDRAM_PARAM(mc_stat_control);
	/* Specifies the value for MC_VIDEO_PROTECT_BOM */
	CASE_SET_SDRAM_PARAM(mc_video_protect_bom);
	/* Specifies the value for MC_VIDEO_PROTECT_BOM_ADR_HI */
	CASE_SET_SDRAM_PARAM(mc_video_protect_bom_adr_hi);
	/* Specifies the value for MC_VIDEO_PROTECT_SIZE_MB */
	CASE_SET_SDRAM_PARAM(mc_video_protect_size_mb);
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE */
	CASE_SET_SDRAM_PARAM(mc_video_protect_vpr_override);
	/* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE1 */
	CASE_SET_SDRAM_PARAM(mc_video_protect_vpr_override1);
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_0 */
	CASE_SET_SDRAM_PARAM(mc_video_protect_gpu_override0);
	/* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_1 */
	CASE_SET_SDRAM_PARAM(mc_video_protect_gpu_override1);
	/* Specifies the value for MC_SEC_CARVEOUT_BOM */
	CASE_SET_SDRAM_PARAM(mc_sec_carveout_bom);
	/* Specifies the value for MC_SEC_CARVEOUT_ADR_HI */
	CASE_SET_SDRAM_PARAM(mc_sec_carveout_adr_hi);
	/* Specifies the value for MC_SEC_CARVEOUT_SIZE_MB */
	CASE_SET_SDRAM_PARAM(mc_sec_carveout_size_mb);
	/* Specifies the value for MC_VIDEO_PROTECT_REG_CTRL.VIDEO_PROTECT_WRITE_ACCESS */
	CASE_SET_SDRAM_PARAM(mc_video_protect_write_access);
	/* Specifies the value for MC_SEC_CARVEOUT_REG_CTRL.SEC_CARVEOUT_WRITE_ACCESS */
	CASE_SET_SDRAM_PARAM(mc_sec_carveout_protect_write_access);

	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_bom);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_bom_hi);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_size_128kb);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_force_internal_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout1_cfg0);

	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_bom);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_bom_hi);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_size_128kb);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_force_internal_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout2_cfg0);

	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_bom);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_bom_hi);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_size_128kb);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_force_internal_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout3_cfg0);

	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_bom);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_bom_hi);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_size_128kb);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_force_internal_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout4_cfg0);

	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_bom);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_bom_hi);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_size_128kb);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access0);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access1);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access2);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access3);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_force_internal_access4);
	CASE_SET_SDRAM_PARAM(mc_generalized_carveout5_cfg0);

	/* Specifies enable for CA training */
	CASE_SET_SDRAM_PARAM(emc_ca_training_enable);
	/* Set if bit 6 select is greater than bit 7 select); uses aremc.spec packet SWIZZLE_BIT6_GT_BIT7 */
	CASE_SET_SDRAM_PARAM(swizzle_rank_byte_encode);
	/* Specifies enable and offset for patched boot rom write */
	CASE_SET_SDRAM_PARAM(boot_rom_patch_control);
	/* Specifies data for patched boot rom write */
	CASE_SET_SDRAM_PARAM(boot_rom_patch_data);

	/* Specifies the value for MC_MTS_CARVEOUT_BOM */
	CASE_SET_SDRAM_PARAM(mc_mts_carveout_bom);
	/* Specifies the value for MC_MTS_CARVEOUT_ADR_HI */
	CASE_SET_SDRAM_PARAM(mc_mts_carveout_adr_hi);
	/* Specifies the value for MC_MTS_CARVEOUT_SIZE_MB */
	CASE_SET_SDRAM_PARAM(mc_mts_carveout_size_mb);
	/* Specifies the value for MC_MTS_CARVEOUT_REG_CTRL */
	CASE_SET_SDRAM_PARAM(mc_mts_carveout_reg_ctrl);

	DEFAULT();
	}
	return 0;
}

int
t210_getbl_param(u_int32_t set,
		parse_token id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (set >= NVBOOT_MAX_BOOTLOADERS)
		return -ENODATA;
	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {
	CASE_GET_BL_PARAM(version);
	CASE_GET_BL_PARAM(start_blk);
	CASE_GET_BL_PARAM(start_page);
	CASE_GET_BL_PARAM(length);
	CASE_GET_BL_PARAM(load_addr);
	CASE_GET_BL_PARAM(entry_point);
	CASE_GET_BL_PARAM(attribute);

	case token_bl_crypto_hash:
		memcpy(data,
		&(bct_ptr->bootloader[set].signature.crypto_hash),
		sizeof(nvboot_hash));
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t210_setbl_param(u_int32_t set,
		parse_token id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (set >= NVBOOT_MAX_BOOTLOADERS)
		return -ENODATA;
	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {
	CASE_SET_BL_PARAM(version);
	CASE_SET_BL_PARAM(start_blk);
	CASE_SET_BL_PARAM(start_page);
	CASE_SET_BL_PARAM(length);
	CASE_SET_BL_PARAM(load_addr);
	CASE_SET_BL_PARAM(entry_point);
	CASE_SET_BL_PARAM(attribute);

	case token_bl_crypto_hash:
		memcpy(&(bct_ptr->bootloader[set].signature.crypto_hash),
		data,
		sizeof(nvboot_hash));
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t210_bct_get_value(parse_token id, void *data, u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;
	nvboot_config_table  samplebct; /* Used for computing offsets. */

	/*
	 * Note: Not all queries require use of the BCT, so testing for a
	 *       valid BCT is distributed within the code.
	 */
	if (data == NULL)
		return -ENODATA;

	switch (id) {
	/*
	 * Simple BCT fields
	 */
	CASE_GET_NVU32(boot_data_version);
	CASE_GET_NVU32(block_size_log2);
	CASE_GET_NVU32(page_size_log2);
	CASE_GET_NVU32(partition_size);
	CASE_GET_NVU32(num_param_sets);
	CASE_GET_NVU32(num_sdram_sets);
	CASE_GET_NVU32(bootloader_used);
	CASE_GET_NVU32(odm_data);
	CASE_GET_NVU32(secure_debug_control);

	case token_block_size:
		if (bct == NULL)
			return -ENODATA;
		*((u_int32_t *)data) = 1 << bct_ptr->block_size_log2;
		break;

	case token_page_size:
		if (bct == NULL)
			return -ENODATA;
		*((u_int32_t *)data) = 1 << bct_ptr->page_size_log2;
		break;

	/*
	 * Constants.
	 */
	CASE_GET_CONST(bootloaders_max,   NVBOOT_MAX_BOOTLOADERS);
	CASE_GET_CONST(reserved_size,     NVBOOT_BCT_RESERVED_SIZE);
	case token_crypto_hash:
		memcpy(data,
		&(bct_ptr->signature.crypto_hash),
		sizeof(nvboot_hash));
		break;

	case token_unique_chip_id:
		memcpy(data, &(bct_ptr->unique_chip_id), sizeof(nvboot_ecid));
		break;

	case token_reserved_offset:
		*((u_int32_t *)data) = (u_int8_t *)&(samplebct.reserved)
				- (u_int8_t *)&samplebct;
		break;

	case token_bct_size:
		*((u_int32_t *)data) = sizeof(nvboot_config_table);
		break;

	CASE_GET_CONST(hash_size, sizeof(nvboot_hash));

	case token_crypto_offset:
		/* Offset to region in BCT to encrypt & sign */
		*((u_int32_t *)data) = (u_int8_t *)&(samplebct.random_aes_blk)
				- (u_int8_t *)&samplebct;
		break;

	case token_crypto_length:
		/* size of region in BCT to encrypt & sign */
		*((u_int32_t *)data) = (u_int8_t *)bct_ptr + sizeof(nvboot_config_table)
				- (u_int8_t *)&(bct_ptr->random_aes_blk);
		break;

	CASE_GET_CONST(max_bct_search_blks, NVBOOT_MAX_BCT_SEARCH_BLOCKS);

	CASE_GET_CONST_PREFIX(dev_type_sdmmc, nvboot);
	CASE_GET_CONST_PREFIX(dev_type_spi, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_4bit, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_8bit, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllp_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_clockm, nvboot);

	CASE_GET_CONST_PREFIX(memory_type_none, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_ddr, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_lpddr, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_ddr2, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_lpddr2, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_ddr3, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_lpddr4, nvboot);

	default:
		return -ENODATA;
	}
	return 0;
}

int
t210_bct_set_value(parse_token id, void *data, u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {
	/*
	 * Simple BCT fields
	 */
	CASE_SET_NVU32(boot_data_version);
	CASE_SET_NVU32(block_size_log2);
	CASE_SET_NVU32(page_size_log2);
	CASE_SET_NVU32(partition_size);
	CASE_SET_NVU32(num_param_sets);
	CASE_SET_NVU32(num_sdram_sets);
	CASE_SET_NVU32(bootloader_used);
	CASE_SET_NVU32(odm_data);
	CASE_SET_NVU32(secure_debug_control);
	case token_unique_chip_id:
		memcpy(&bct_ptr->unique_chip_id, data, sizeof(nvboot_ecid));
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int
t210_bct_set_data(parse_token id,
	u_int8_t *data,
	u_int32_t length,
	u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table *)bct;

	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {

	case token_crypto_hash:
		if (length < sizeof(nvboot_hash))
			return -ENODATA;
		memcpy(&bct_ptr->signature.crypto_hash, data, sizeof(nvboot_hash));
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

int t210_get_bct_size()
{
	return sizeof(nvboot_config_table);
}

int t210_bct_token_supported(parse_token token)
{
	int index;

	for (index = 0; index < ARRAY_SIZE(t210_root_token_list); index++)
		if (t210_root_token_list[index] == token)
			return 1;

	return 0;
}

void t210_init_bad_block_table(build_image_context *context)
{
	u_int32_t bytes_per_entry;
	nvboot_badblock_table *table;
	nvboot_config_table *bct;

	bct = (nvboot_config_table *)(context->bct);

	assert(context != NULL);
	assert(bct != NULL);

	table = &bct->badblock_table;

	bytes_per_entry = ICEIL(context->partition_size,
				NVBOOT_BAD_BLOCK_TABLE_SIZE);
	table->block_size_log2 = context->block_size_log2;
	table->virtual_blk_size_log2 = NV_MAX(ceil_log2(bytes_per_entry),
					table->block_size_log2);
	table->entries_used = iceil_log2(context->partition_size,
					table->virtual_blk_size_log2);
}

cbootimage_soc_config tegra210_config = {
	.init_bad_block_table		= t210_init_bad_block_table,
	.set_dev_param				= t210_set_dev_param,
	.get_dev_param				= t210_get_dev_param,
	.set_sdram_param			= t210_set_sdram_param,
	.get_sdram_param			= t210_get_sdram_param,
	.setbl_param				= t210_setbl_param,
	.getbl_param				= t210_getbl_param,
	.set_value					= t210_bct_set_value,
	.get_value					= t210_bct_get_value,
	.set_data					= t210_bct_set_data,
	.get_bct_size				= t210_get_bct_size,
	.token_supported			= t210_bct_token_supported,

	.devtype_table				= s_devtype_table_t210,
	.sdmmc_data_width_table		= s_sdmmc_data_width_table_t210,
	.spi_clock_source_table		= s_spi_clock_source_table_t210,
	.nvboot_memory_type_table	= s_nvboot_memory_type_table_t210,
	.sdram_field_table			= s_sdram_field_table_t210,
	.nand_table					= 0,
	.sdmmc_table				= s_sdmmc_table_t210,
	.spiflash_table				= s_spiflash_table_t210,
	.device_type_table			= s_device_type_table_t210,
};

void t210_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config)
{
	context->boot_data_version = BOOTDATA_VERSION_T210;
	*soc_config = &tegra210_config;
}

int if_bct_is_t210_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config)
{
	nvboot_config_table *bct = (nvboot_config_table*) context->bct;

	if (bct->boot_data_version == BOOTDATA_VERSION_T210) {
		t210_get_soc_config(context, soc_config);
		return 1;
	}
	return 0;
}
