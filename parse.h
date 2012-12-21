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

	token_nand_clock_divider,
	token_nand_nand_timing,
	token_nand_nand_timing2,
	token_nand_block_size_log2,
	token_nand_page_size_log2,
	token_sdmmc_clock_divider,
	token_sdmmc_data_width,
	token_sdmmc_sd_controller,
	token_sdmmc_max_power_class_supported,
	token_spiflash_read_command_type_fast,
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
	token_mc_emem_cfg,
	token_emc_cfg,
	token_emc_cfg2,
	token_emc_dbg,
	token_emc_cfg_dig_dll,
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
	token_emc_clock_use_pll_mud,
	token_emc_pin_extra_wait,
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
	token_emc_cmd_q,
	token_emc_mc2emc_q,
	token_emc_dyn_self_ref_control,
	token_ahb_arbitration_xbar_ctrl_meminit_done,
	token_emc_dev_select,
	token_emc_sel_dpd_ctrl,
	token_emc_dll_xform_dqs0,
	token_emc_dll_xform_dqs1,
	token_emc_dll_xform_dqs2,
	token_emc_dll_xform_dqs3,
	token_emc_dll_xform_dqs4,
	token_emc_dll_xform_dqs5,
	token_emc_dll_xform_dqs6,
	token_emc_dll_xform_dqs7,
	token_emc_dll_xform_quse0,
	token_emc_dll_xform_quse1,
	token_emc_dll_xform_quse2,
	token_emc_dll_xform_quse3,
	token_emc_dll_xform_quse4,
	token_emc_dll_xform_quse5,
	token_emc_dll_xform_quse6,
	token_emc_dll_xform_quse7,
	token_emc_dli_trim_tx_dqs0,
	token_emc_dli_trim_tx_dqs1,
	token_emc_dli_trim_tx_dqs2,
	token_emc_dli_trim_tx_dqs3,
	token_emc_dli_trim_tx_dqs4,
	token_emc_dli_trim_tx_dqs5,
	token_emc_dli_trim_tx_dqs6,
	token_emc_dli_trim_tx_dqs7,
	token_emc_dll_xform_dq0,
	token_emc_dll_xform_dq1,
	token_emc_dll_xform_dq2,
	token_emc_dll_xform_dq3,
	token_emc_zcal_interval,
	token_emc_zcal_init_dev0,
	token_emc_zcal_init_dev1,
	token_emc_zcal_init_wait,
	token_emc_zcal_cold_boot_enable,
	token_emc_zcal_warm_boot_enable,
	token_emc_mrw_lpddr2zcal_warm_boot,
	token_emc_zqcal_ddr3_warm_boot,
	token_emc_zcal_warm_boot_wait,
	token_emc_mrs_warm_boot_enable,
	token_emc_mrs_extra,
	token_emc_warm_boot_mrs,
	token_emc_warm_boot_emrs,
	token_emc_warm_boot_emr2,
	token_emc_warm_boot_emr3,
	token_emc_warm_boot_mrs_extra,
	token_emc_clken_override,
	token_emc_extra_refresh_num,
	token_emc_clken_override_allwarm_boot,
	token_mc_clken_override_allwarm_boot,
	token_emc_cfg_dig_dll_period_warm_boot,
	token_pmc_vddp_sel,
	token_pmc_ddr_cfg,
	token_pmc_io_dpd_req,
	token_pmc_eno_vtt_gen,
	token_pmc_no_io_power,
	token_emc_xm2cmd_pad_ctrl,
	token_emc_xm2cmd_pad_ctrl2,
	token_emc_xm2dqs_pad_ctrl,
	token_emc_xm2dqs_pad_ctrl2,
	token_emc_xm2dqs_pad_ctrl3,
	token_emc_xm2dq_pad_ctrl,
	token_emc_xm2dq_pad_ctrl2,
	token_emc_xm2clk_pad_ctrl,
	token_emc_xm2comp_pad_ctrl,
	token_emc_xm2vttgen_pad_ctrl,
	token_emc_xm2vttgen_pad_ctrl2,
	token_emc_xm2quse_pad_ctrl,
	token_mc_emem_adr_cfg,
	token_mc_emem_adr_cfg_dev0,
	token_mc_emem_adr_cfg_dev1,
	token_mc_emem_arb_cfg,
	token_mc_emem_arb_outstanding_req,
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
	token_mc_emem_arb_da_turns,
	token_mc_emem_arb_da_covers,
	token_mc_emem_arb_misc0,
	token_mc_emem_arb_misc1,
	token_mc_emem_arb_ring1_throttle,
	token_mc_emem_arb_override,
	token_mc_emem_arb_rsv,
	token_mc_clken_override,

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
			u_int32_t index,
			parse_token token,
			u_int32_t value);
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
			u_int32_t index,
			parse_token token,
			u_int32_t *value);
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
			u_int32_t index,
			parse_token token,
			u_int32_t value);
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
			u_int32_t index,
			parse_token token,
			u_int32_t *value);
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
	int (*setbl_param)(u_int32_t set,
			parse_token id,
			u_int32_t *data,
			u_int8_t *bct);
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
	int (*getbl_param)(u_int32_t set,
			parse_token id,
			u_int32_t *data,
			u_int8_t *bct);
	/*
	 * Set the specified bct value stored in context bct data structure.
	 *
	 * @param id  	The parse token value
	 * @param data	Value to set
	 * @param bct 	Bct pointer
	 * @return 0 and -ENODATA for success and failure
	 */
	int (*set_value)(parse_token id,
			u_int32_t  data,
			u_int8_t *bct);
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
			u_int32_t *data,
			u_int8_t *bct);
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
			u_int8_t *data,
			u_int32_t  length,
			u_int8_t *bct);

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

void process_config_file(build_image_context *context, u_int8_t simple_parse);

void t30_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
void t20_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);

int if_bct_is_t30_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);
int if_bct_is_t20_get_soc_config(build_image_context *context,
	cbootimage_soc_config **soc_config);

int
t30_get_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t *value);
int
t30_set_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t value);
int
t30_get_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t *value);
int
t30_set_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t value);
int
t20_get_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t *value);
int
t20_set_dev_param(build_image_context *context,
	u_int32_t index,
	parse_token token,
	u_int32_t value);

int
t20_get_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t *value);
int
t20_set_sdram_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t value);

u_int32_t iceil_log2(u_int32_t a, u_int32_t b);

/* Returns the smallest power of 2 >= a */
u_int32_t ceil_log2(u_int32_t a);

extern cbootimage_soc_config *g_soc_config;

/*
 * Provide access to enum and field tables.  These tables are useful when
 * pretty printing a BCT file using bct_dump.
 */
extern enum_item s_devtype_table_t20[];
extern enum_item s_devtype_table_t30[];

extern enum_item s_sdmmc_data_width_table_t20[];
extern enum_item s_sdmmc_data_width_table_t30[];

extern enum_item s_spi_clock_source_table_t20[];
extern enum_item s_spi_clock_source_table_t30[];

extern enum_item s_nvboot_memory_type_table_t20[];
extern enum_item s_nvboot_memory_type_table_t30[];
extern field_item s_sdram_field_table_t20[];
extern field_item s_sdram_field_table_t30[];
extern field_item s_nand_table_t20[];
extern field_item s_nand_table_t30[];
extern field_item s_sdmmc_table_t20[];
extern field_item s_sdmmc_table_t30[];
extern field_item s_spiflash_table_t20[];
extern field_item s_spiflash_table_t30[];

extern parse_subfield_item s_device_type_table_t20[];
extern parse_subfield_item s_device_type_table_t30[];

#endif /* #ifndef INCLUDED_PARSE_H */
