/**
 * Copyright (c) 2011 NVIDIA Corporation.  All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
	token_block_size,
	token_page_size,
	token_partition_size,
	token_dev_type,
	token_dev_param,
	token_redundancy,
	token_version,
	token_bct_file,
	token_addon,
	token_nand_params,
	token_sdmmc_params,
	token_spiflash_params,
	token_data_width,
	token_clock_divider,
	token_clock_source,
	token_read_command_type_fast,
	token_max_power_class_supported,
	token_nand_timing2,
	token_nand_timing,
	token_block_size_log2,
	token_page_size_log2,
	token_sdram,

	token_memory_type,
	token_pllm_charge_pump_setup_ctrl,
	token_pllm_loop_filter_setup_ctrl,
	token_pllm_input_divider,
	token_pllm_feedback_divider,
	token_pllm_post_divider,
	token_pllm_stable_time,
	token_emc_clock_divider,
	token_emc_auto_cal_interval,
	token_emc_auto_cal_config,   
	token_emc_auto_cal_wait,
	token_emc_pin_program_wait,
	token_emc_rc,
	token_emc_rfc,
	token_emc_ras,
	token_emc_rp,
	token_emc_r2w,
	token_emc_w2r,
	token_emc_r2p,
	token_emc_w2p,
	token_emc_rd_rcd,
	token_emc_wr_rcd,
	token_emc_rrd,
	token_emc_rext,
	token_emc_wdv,
	token_emc_quse,
	token_emc_qrst,
	token_emc_qsafe,
	token_emc_rdv,
	token_emc_refresh,
	token_emc_burst_refresh_num,
	token_emc_pdex2wr,
	token_emc_pdex2rd,
	token_emc_pchg2pden,
	token_emc_act2pden,
	token_emc_ar2pden,
	token_emc_rw2pden,
	token_emc_txsr,
	token_emc_tcke,
	token_emc_tfaw,
	token_emc_trpab,
	token_emc_tclkstable,
	token_emc_tclkstop,
	token_emc_trefbw,
	token_emc_quse_extra,
	token_emc_fbio_cfg1,
	token_emc_fbio_dqsib_dly,
	token_emc_fbio_dqsib_dly_msb,
	token_emc_fbio_quse_dly,
	token_emc_fbio_quse_dly_msb,
	token_emc_fbio_cfg5,
	token_emc_fbio_cfg6,
	token_emc_fbio_spare,
	token_emc_mrs,
	token_emc_emrs,
	token_emc_mrw1,
	token_emc_mrw2,
	token_emc_mrw3,
	token_emc_mrw_reset_command,
	token_emc_mrw_reset_ninit_wait,
	token_emc_adr_cfg,
	token_emc_adr_cfg1,
	token_mc_emem_Cfg,
	token_mc_lowlatency_config,
	token_emc_cfg,
	token_emc_cfg2,
	token_emc_dbg,
	token_ahb_arbitration_xbar_ctrl,
	token_emc_cfg_dig_dll,
	token_emc_dll_xform_dqs,
	token_emc_dll_xform_quse,
	token_warm_boot_wait,
	token_emc_ctt_term_ctrl,
	token_emc_odt_write,
	token_emc_odt_read,
	token_emc_zcal_ref_cnt,
	token_emc_zcal_wait_cnt,
	token_emc_zcal_mrw_cmd,
	token_emc_mrs_reset_dll,
	token_emc_mrw_zq_init_dev0,
	token_emc_mrw_zq_init_dev1,
	token_emc_mrw_zq_init_wait,
	token_emc_mrs_reset_dll_wait,
	token_emc_emrs_emr2,
	token_emc_emrs_emr3,
	token_emc_emrs_ddr2_dll_enable,
	token_emc_mrs_ddr2_dll_reset,
	token_emc_emrs_ddr2_ocd_calib,
	token_emc_ddr2_wait,
	token_emc_cfg_clktrim0,
	token_emc_cfg_clktrim1,
	token_emc_cfg_clktrim2,
	token_pmc_ddr_pwr,
	token_apb_misc_gp_xm2cfga_pad_ctrl,
	token_apb_misc_gp_xm2cfgc_pad_ctrl,
	token_apb_misc_gp_xm2cfgc_pad_ctrl2,
	token_apb_misc_gp_xm2cfgd_pad_ctrl,
	token_apb_misc_gp_xm2cfgd_pad_ctrl2,
	token_apb_misc_gp_xm2clkcfg_Pad_ctrl,
	token_apb_misc_gp_xm2comp_pad_ctrl,
	token_apb_misc_gp_xm2vttgen_pad_ctrl,

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
					u_int32_t index,
					parse_token token,
					u_int32_t value);


typedef struct
{
	char *name;
	u_int32_t value;
} enum_item;

typedef struct
{
	char *name;
	u_int32_t token;
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
 * Function prototypes
 */
void process_config_file(build_image_context *context);


#endif /* #ifndef INCLUDED_PARSE_H */
