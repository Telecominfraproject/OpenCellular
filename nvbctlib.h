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
 * nvbctlib is a library for accessing data in BCTs from different versions
 * of the BootROM.  This provides a means for one utility program to
 * reference data which is stored differently by different versions of chips,
 * often with the same header file names.
 *
 * In essence, nvbctlib.h defines an API for selecting chip versions and
 * accessing BCT data, and a separate source file wraps the API implementation
 * around the header files that are unique to a chip version.
 */

#ifndef INCLUDED_NVBCTLIB_H
#define INCLUDED_NVBCTLIB_H

#include <sys/types.h>
/*
 * nvbct_lib_id defines tokens used for querying or setting values within, or
 * about, the BCT.  These are used to identify values within structures,
 * sizes and other properties of structures, and values for enumerated
 * constants.
 */
typedef enum {
	nvbct_lib_id_none = 0,

	nvbct_lib_id_crypto_hash,
	nvbct_lib_id_random_aes_blk,
	nvbct_lib_id_boot_data_version,
	nvbct_lib_id_block_size_log2,
	nvbct_lib_id_page_size_log2,
	nvbct_lib_id_partition_size,
	nvbct_lib_id_dev_type,
	nvbct_lib_id_bootloader_used,
	nvbct_lib_id_bootloaders_max,
	nvbct_lib_id_reserved,
	nvbct_lib_id_reserved_size,
	nvbct_lib_id_reserved_offset,
	nvbct_lib_id_bct_size,
	nvbct_lib_id_hash_size,
	nvbct_lib_id_crypto_offset,
	nvbct_lib_id_crypto_length,
	nvbct_lib_id_max_bct_search_blks,
	nvbct_lib_id_num_param_sets,
	nvbct_lib_id_dev_type_nand,
	nvbct_lib_id_dev_type_sdmmc,
	nvbct_lib_id_dev_type_spi,
	nvbct_lib_id_num_sdram_sets,

	nvbct_lib_id_nand_clock_divider,
	nvbct_lib_id_nand_nand_timing,
	nvbct_lib_id_nand_nand_timing2,
	nvbct_lib_id_nand_block_size_log2,
	nvbct_lib_id_nand_page_size_log2,
	nvbct_lib_id_sdmmc_clock_divider,
	nvbct_lib_id_sdmmc_data_width,
	nvbct_lib_id_sdmmc_max_power_class_supported,
	nvbct_lib_id_spiflash_read_command_type_fast,
	nvbct_lib_id_spiflash_clock_source,
	nvbct_lib_id_spiflash_clock_divider,
	nvbct_lib_id_sdmmc_data_width_4bit,
	nvbct_lib_id_sdmmc_data_width_8bit,
	nvbct_lib_id_spi_clock_source_pllp_out0,
	nvbct_lib_id_spi_clock_source_pllc_out0,
	nvbct_lib_id_spi_clock_source_pllm_out0,
	nvbct_lib_id_spi_clock_source_clockm,

	nvbct_lib_id_bl_version,
	nvbct_lib_id_bl_start_blk,
	nvbct_lib_id_bl_start_page,
	nvbct_lib_id_bl_length,
	nvbct_lib_id_bl_load_addr,
	nvbct_lib_id_bl_entry_point,
	nvbct_lib_id_bl_attribute,
	nvbct_lib_id_bl_crypto_hash,

	nvbct_lib_id_memory_type_none,
	nvbct_lib_id_memory_type_ddr2,
	nvbct_lib_id_memory_type_ddr,
	nvbct_lib_id_memory_type_lpddr2,
	nvbct_lib_id_memory_type_lpddr,

	nvbct_lib_id_sdram_memory_type,
	nvbct_lib_id_sdram_pllm_charge_pump_setup_ctrl,
	nvbct_lib_id_sdram_pllm_loop_filter_setup_ctrl,
	nvbct_lib_id_sdram_pllm_input_divider,
	nvbct_lib_id_sdram_pllm_feedback_divider,
	nvbct_lib_id_sdram_pllm_post_divider,
	nvbct_lib_id_sdram_pllm_stable_time,
	nvbct_lib_id_sdram_emc_clock_divider,
	nvbct_lib_id_sdram_emc_auto_cal_interval,
	nvbct_lib_id_sdram_emc_auto_cal_config,   
	nvbct_lib_id_sdram_emc_auto_cal_wait,
	nvbct_lib_id_sdram_emc_pin_program_wait,
	nvbct_lib_id_sdram_emc_rc,
	nvbct_lib_id_sdram_emc_rfc,
	nvbct_lib_id_sdram_emc_ras,
	nvbct_lib_id_sdram_emc_rp,
	nvbct_lib_id_sdram_emc_r2w,
	nvbct_lib_id_sdram_emc_w2r,
	nvbct_lib_id_sdram_emc_r2p,
	nvbct_lib_id_sdram_emc_w2p,
	nvbct_lib_id_sdram_emc_rd_rcd,
	nvbct_lib_id_sdram_emc_wr_rcd,
	nvbct_lib_id_sdram_emc_rrd,
	nvbct_lib_id_sdram_emc_rext,
	nvbct_lib_id_sdram_emc_wdv,
	nvbct_lib_id_sdram_emc_quse,
	nvbct_lib_id_sdram_emc_qrst,
	nvbct_lib_id_sdram_emc_qsafe,
	nvbct_lib_id_sdram_emc_rdv,
	nvbct_lib_id_sdram_emc_refresh,
	nvbct_lib_id_sdram_emc_burst_refresh_num,
	nvbct_lib_id_sdram_emc_pdex2wr,
	nvbct_lib_id_sdram_emc_pdex2rd,
	nvbct_lib_id_sdram_emc_pchg2pden,
	nvbct_lib_id_sdram_emc_act2pden,
	nvbct_lib_id_sdram_emc_ar2pden,
	nvbct_lib_id_sdram_emc_rw2pden,
	nvbct_lib_id_sdram_emc_txsr,
	nvbct_lib_id_sdram_emc_tcke,
	nvbct_lib_id_sdram_emc_tfaw,
	nvbct_lib_id_sdram_emc_trpab,
	nvbct_lib_id_sdram_emc_tclkstable,
	nvbct_lib_id_sdram_emc_tclkstop,
	nvbct_lib_id_sdram_emc_trefbw,
	nvbct_lib_id_sdram_emc_quse_extra,
	nvbct_lib_id_sdram_emc_fbio_cfg1,
	nvbct_lib_id_sdram_emc_fbio_dqsib_dly,
	nvbct_lib_id_sdram_emc_fbio_dqsib_dly_msb,
	nvbct_lib_id_sdram_emc_fbio_quse_dly,
	nvbct_lib_id_sdram_emc_fbio_quse_dly_msb,
	nvbct_lib_id_sdram_emc_fbio_cfg5,
	nvbct_lib_id_sdram_emc_fbio_cfg6,
	nvbct_lib_id_sdram_emc_fbio_spare,
	nvbct_lib_id_sdram_emc_mrs,
	nvbct_lib_id_sdram_emc_emrs,
	nvbct_lib_id_sdram_emc_mrw1,
	nvbct_lib_id_sdram_emc_mrw2,
	nvbct_lib_id_sdram_emc_mrw3,
	nvbct_lib_id_sdram_emc_mrw_reset_command,
	nvbct_lib_id_sdram_emc_mrw_reset_ninit_wait,
	nvbct_lib_id_sdram_emc_adr_cfg,
	nvbct_lib_id_sdram_emc_adr_cfg1,
	nvbct_lib_id_sdram_mc_emem_Cfg,
	nvbct_lib_id_sdram_mc_lowlatency_config,
	nvbct_lib_id_sdram_emc_cfg,
	nvbct_lib_id_sdram_emc_cfg2,
	nvbct_lib_id_sdram_emc_dbg,
	nvbct_lib_id_sdram_ahb_arbitration_xbar_ctrl,
	nvbct_lib_id_sdram_emc_cfg_dig_dll,
	nvbct_lib_id_sdram_emc_dll_xform_dqs,
	nvbct_lib_id_sdram_emc_dll_xform_quse,
	nvbct_lib_id_sdram_warm_boot_wait,
	nvbct_lib_id_sdram_emc_ctt_term_ctrl,
	nvbct_lib_id_sdram_emc_odt_write,
	nvbct_lib_id_sdram_emc_odt_read,
	nvbct_lib_id_sdram_emc_zcal_ref_cnt,
	nvbct_lib_id_sdram_emc_zcal_wait_cnt,
	nvbct_lib_id_sdram_emc_zcal_mrw_cmd,
	nvbct_lib_id_sdram_emc_mrs_reset_dll,
	nvbct_lib_id_sdram_emc_mrw_zq_init_dev0,
	nvbct_lib_id_sdram_emc_mrw_zq_init_dev1,
	nvbct_lib_id_sdram_emc_mrw_zq_init_wait,
	nvbct_lib_id_sdram_emc_mrs_reset_dll_wait,
	nvbct_lib_id_sdram_emc_emrs_emr2,
	nvbct_lib_id_sdram_emc_emrs_emr3,
	nvbct_lib_id_sdram_emc_emrs_ddr2_dll_enable,
	nvbct_lib_id_sdram_emc_mrs_ddr2_dll_reset,
	nvbct_lib_id_sdram_emc_emrs_ddr2_ocd_calib,
	nvbct_lib_id_sdram_emc_ddr2_wait,
	nvbct_lib_id_sdram_emc_cfg_clktrim0,
	nvbct_lib_id_sdram_emc_cfg_clktrim1,
	nvbct_lib_id_sdram_emc_cfg_clktrim2,
	nvbct_lib_id_sdram_pmc_ddr_pwr,
	nvbct_lib_id_sdram_apb_misc_gp_xm2cfga_pad_ctrl,
	nvbct_lib_id_sdram_apb_misc_gp_xm2cfgc_pad_ctrl,
	nvbct_lib_id_sdram_apb_misc_gp_xm2cfgc_pad_ctrl2,
	nvbct_lib_id_sdram_apb_misc_gp_xm2cfgd_pad_ctrl,
	nvbct_lib_id_sdram_apb_misc_gp_xm2cfgd_pad_ctrl2,
	nvbct_lib_id_sdram_apb_misc_gp_xm2clkcfg_Pad_ctrl,
	nvbct_lib_id_sdram_apb_misc_gp_xm2comp_pad_ctrl,
	nvbct_lib_id_sdram_apb_misc_gp_xm2vttgen_pad_ctrl,

	nvbct_lib_id_max,

	nvbct_lib_id_force32 = 0x7fffffff

} nvbct_lib_id;

typedef int  (*nvbct_lib_get_dev_param)(u_int32_t set,
					nvbct_lib_id id,
					u_int32_t *data,
					u_int8_t *bct);
typedef int  (*nvbct_lib_set_dev_param)(u_int32_t set,
					nvbct_lib_id id,
					u_int32_t data,
					u_int8_t *bct);

typedef int  (*nvbct_lib_get_sdram_param)(u_int32_t set,
					nvbct_lib_id id,
					u_int32_t *data,
					u_int8_t *bct);
typedef int  (*nvbct_lib_set_sdram_param)(u_int32_t set,
					nvbct_lib_id id,
					u_int32_t data,
					u_int8_t *bct);

typedef int (*nvbct_lib_get_bl_param)(u_int32_t set,
					nvbct_lib_id id,
					u_int32_t *data,
					u_int8_t *bct);
typedef int (*nvbct_lib_set_bl_param)(u_int32_t set,
					nvbct_lib_id id,
					u_int32_t *data,
					u_int8_t *bct);

typedef int (*nvbct_lib_get_value)(nvbct_lib_id id,
					u_int32_t *data,
					u_int8_t *bct);
typedef int (*nvbct_lib_set_value)(nvbct_lib_id id,
					u_int32_t  data,
					u_int8_t *bct);

/*
 * Note: On input, *length is the size of data.  On output, *length is the
 * actual size used.
 */
typedef int (*nvbct_lib_get_data)(nvbct_lib_id id,
					u_int8_t *data,
					u_int32_t *length,
					u_int8_t *bct);
typedef int (*nvbct_lib_set_data)(nvbct_lib_id id,
					u_int8_t *data,
					u_int32_t  length,
					u_int8_t *bct);

/*
 * Structure of function pointers used to access a specific BCT variant.
 */
typedef struct nvbct_lib_fns_rec
{
	nvbct_lib_get_value    get_value;
	nvbct_lib_set_value    set_value;

	nvbct_lib_get_data    get_data;
	nvbct_lib_set_data    set_data;

	nvbct_lib_get_bl_param    getbl_param;
	nvbct_lib_set_bl_param    setbl_param;

	nvbct_lib_get_dev_param getdev_param;
	nvbct_lib_set_dev_param setdev_param;

	nvbct_lib_get_sdram_param get_sdram_params;
	nvbct_lib_set_sdram_param set_sdram_params;
} nvbct_lib_fns;

void nvbct_lib_get_fns(nvbct_lib_fns *fns);

#endif /* #ifndef INCLUDED_NVBCTLIB_H */
