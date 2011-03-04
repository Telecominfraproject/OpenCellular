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

#include "nvbctlib.h"
#include "nvboot_bct.h"
#include "string.h"
#include "cbootimage.h"

/* nvbctlib_ap20.c: The implementation of the nvbctlib API for AP20. */

/* Definitions that simplify the code which follows. */
#define CASE_GET_SDRAM_PARAM(x) \
case nvbct_lib_id_sdram_##x:\
	*data = bct_ptr->sdram_params[set].x; \
	break

#define CASE_SET_SDRAM_PARAM(x) \
case nvbct_lib_id_sdram_##x:\
	bct_ptr->sdram_params[set].x = data; \
	break

#define CASE_GET_DEV_PARAM(dev, x) \
case nvbct_lib_id_##dev##_##x:\
	*data = bct_ptr->dev_params[set].dev##_params.x; \
	break

#define CASE_SET_DEV_PARAM(dev, x) \
case nvbct_lib_id_##dev##_##x:\
	bct_ptr->dev_params[set].dev##_params.x = data; \
	break

#define CASE_GET_BL_PARAM(x) \
case nvbct_lib_id_bl_##x:\
	*data = bct_ptr->bootloader[set].x; \
	break

#define CASE_SET_BL_PARAM(x) \
case nvbct_lib_id_bl_##x:\
	bct_ptr->bootloader[set].x = *data; \
	break

#define CASE_GET_NVU32(id) \
case nvbct_lib_id_##id:\
	if (bct == NULL) return -ENODATA; \
	*data = bct_ptr->id; \
	break

#define CASE_GET_CONST(id, val) \
case nvbct_lib_id_##id:\
	*data = val; \
	break

#define CASE_GET_CONST_PREFIX(id, val_prefix) \
case nvbct_lib_id_##id:\
	*data = val_prefix##_##id; \
	break

#define CASE_SET_NVU32(id) \
case nvbct_lib_id_##id:\
	bct_ptr->id = data; \
	break

#define CASE_GET_DATA(id, size) \
case nvbct_lib_id_##id:\
	if (*length < size) return -ENODATA;\
	memcpy(data, &(bct_ptr->id), size);   \
	*length = size;\
	break

#define CASE_SET_DATA(id, size) \
case nvbct_lib_id_##id:\
	if (length < size) return -ENODATA;\
	memcpy(&(bct_ptr->id), data, size);   \
	break

static int
get_sdram_params(u_int32_t set,
		nvbct_lib_id id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

	if (set >= NVBOOT_BCT_MAX_SDRAM_SETS)
		return ENODATA;
	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {

	CASE_GET_SDRAM_PARAM(memory_type);
	CASE_GET_SDRAM_PARAM(pllm_charge_pump_setup_ctrl);
	CASE_GET_SDRAM_PARAM(pllm_loop_filter_setup_ctrl);
	CASE_GET_SDRAM_PARAM(pllm_input_divider);
	CASE_GET_SDRAM_PARAM(pllm_feedback_divider);
	CASE_GET_SDRAM_PARAM(pllm_post_divider);
	CASE_GET_SDRAM_PARAM(pllm_stable_time);
	CASE_GET_SDRAM_PARAM(emc_clock_divider);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_interval);
	CASE_GET_SDRAM_PARAM(emc_auto_cal_config);   
	CASE_GET_SDRAM_PARAM(emc_auto_cal_wait);
	CASE_GET_SDRAM_PARAM(emc_pin_program_wait);
	CASE_GET_SDRAM_PARAM(emc_rc);
	CASE_GET_SDRAM_PARAM(emc_rfc);
	CASE_GET_SDRAM_PARAM(emc_ras);
	CASE_GET_SDRAM_PARAM(emc_rp);
	CASE_GET_SDRAM_PARAM(emc_r2w);
	CASE_GET_SDRAM_PARAM(emc_w2r);
	CASE_GET_SDRAM_PARAM(emc_r2p);
	CASE_GET_SDRAM_PARAM(emc_w2p);
	CASE_GET_SDRAM_PARAM(emc_rd_rcd);
	CASE_GET_SDRAM_PARAM(emc_wr_rcd);
	CASE_GET_SDRAM_PARAM(emc_rrd);
	CASE_GET_SDRAM_PARAM(emc_rext);
	CASE_GET_SDRAM_PARAM(emc_wdv);
	CASE_GET_SDRAM_PARAM(emc_quse);
	CASE_GET_SDRAM_PARAM(emc_qrst);
	CASE_GET_SDRAM_PARAM(emc_qsafe);
	CASE_GET_SDRAM_PARAM(emc_rdv);
	CASE_GET_SDRAM_PARAM(emc_refresh);
	CASE_GET_SDRAM_PARAM(emc_burst_refresh_num);
	CASE_GET_SDRAM_PARAM(emc_pdex2wr);
	CASE_GET_SDRAM_PARAM(emc_pdex2rd);
	CASE_GET_SDRAM_PARAM(emc_pchg2pden);
	CASE_GET_SDRAM_PARAM(emc_act2pden);
	CASE_GET_SDRAM_PARAM(emc_ar2pden);
	CASE_GET_SDRAM_PARAM(emc_rw2pden);
	CASE_GET_SDRAM_PARAM(emc_txsr);
	CASE_GET_SDRAM_PARAM(emc_tcke);
	CASE_GET_SDRAM_PARAM(emc_tfaw);
	CASE_GET_SDRAM_PARAM(emc_trpab);
	CASE_GET_SDRAM_PARAM(emc_tclkstable);
	CASE_GET_SDRAM_PARAM(emc_tclkstop);
	CASE_GET_SDRAM_PARAM(emc_trefbw);
	CASE_GET_SDRAM_PARAM(emc_quse_extra);
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg1);
	CASE_GET_SDRAM_PARAM(emc_fbio_dqsib_dly);
	CASE_GET_SDRAM_PARAM(emc_fbio_dqsib_dly_msb);
	CASE_GET_SDRAM_PARAM(emc_fbio_quse_dly);
	CASE_GET_SDRAM_PARAM(emc_fbio_quse_dly_msb);
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg5);
	CASE_GET_SDRAM_PARAM(emc_fbio_cfg6);
	CASE_GET_SDRAM_PARAM(emc_fbio_spare);
	CASE_GET_SDRAM_PARAM(emc_mrs);
	CASE_GET_SDRAM_PARAM(emc_emrs);
	CASE_GET_SDRAM_PARAM(emc_mrw1);
	CASE_GET_SDRAM_PARAM(emc_mrw2);
	CASE_GET_SDRAM_PARAM(emc_mrw3);
	CASE_GET_SDRAM_PARAM(emc_mrw_reset_command);
	CASE_GET_SDRAM_PARAM(emc_mrw_reset_ninit_wait);
	CASE_GET_SDRAM_PARAM(emc_adr_cfg);
	CASE_GET_SDRAM_PARAM(emc_adr_cfg1);
	CASE_GET_SDRAM_PARAM(mc_emem_Cfg);
	CASE_GET_SDRAM_PARAM(mc_lowlatency_config);
	CASE_GET_SDRAM_PARAM(emc_cfg);
	CASE_GET_SDRAM_PARAM(emc_cfg2);
	CASE_GET_SDRAM_PARAM(emc_dbg);
	CASE_GET_SDRAM_PARAM(ahb_arbitration_xbar_ctrl);
	CASE_GET_SDRAM_PARAM(emc_cfg_dig_dll);
	CASE_GET_SDRAM_PARAM(emc_dll_xform_dqs);
	CASE_GET_SDRAM_PARAM(emc_dll_xform_quse);
	CASE_GET_SDRAM_PARAM(warm_boot_wait);
	CASE_GET_SDRAM_PARAM(emc_ctt_term_ctrl);
	CASE_GET_SDRAM_PARAM(emc_odt_write);
	CASE_GET_SDRAM_PARAM(emc_odt_read);
	CASE_GET_SDRAM_PARAM(emc_zcal_ref_cnt);
	CASE_GET_SDRAM_PARAM(emc_zcal_wait_cnt);
	CASE_GET_SDRAM_PARAM(emc_zcal_mrw_cmd);
	CASE_GET_SDRAM_PARAM(emc_mrs_reset_dll);
	CASE_GET_SDRAM_PARAM(emc_mrw_zq_init_dev0);
	CASE_GET_SDRAM_PARAM(emc_mrw_zq_init_dev1);
	CASE_GET_SDRAM_PARAM(emc_mrw_zq_init_wait);
	CASE_GET_SDRAM_PARAM(emc_mrs_reset_dll_wait);
	CASE_GET_SDRAM_PARAM(emc_emrs_emr2);
	CASE_GET_SDRAM_PARAM(emc_emrs_emr3);
	CASE_GET_SDRAM_PARAM(emc_emrs_ddr2_dll_enable);
	CASE_GET_SDRAM_PARAM(emc_mrs_ddr2_dll_reset);
	CASE_GET_SDRAM_PARAM(emc_emrs_ddr2_ocd_calib);
	CASE_GET_SDRAM_PARAM(emc_ddr2_wait);
	CASE_GET_SDRAM_PARAM(emc_cfg_clktrim0);
	CASE_GET_SDRAM_PARAM(emc_cfg_clktrim1);
	CASE_GET_SDRAM_PARAM(emc_cfg_clktrim2);
	CASE_GET_SDRAM_PARAM(pmc_ddr_pwr);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfga_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl2);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl2);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2clkcfg_Pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2comp_pad_ctrl);
	CASE_GET_SDRAM_PARAM(apb_misc_gp_xm2vttgen_pad_ctrl);

	default:
		return -ENODATA;
	}

	return 0;

}

static int
set_sdram_params(u_int32_t set,
		nvbct_lib_id id,
		u_int32_t data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

	if (set >= NVBOOT_BCT_MAX_SDRAM_SETS)
		return ENODATA;
	if (bct == NULL)
		return -ENODATA;

	switch (id) {

	CASE_SET_SDRAM_PARAM(memory_type);
	CASE_SET_SDRAM_PARAM(pllm_charge_pump_setup_ctrl);
	CASE_SET_SDRAM_PARAM(pllm_loop_filter_setup_ctrl);
	CASE_SET_SDRAM_PARAM(pllm_input_divider);
	CASE_SET_SDRAM_PARAM(pllm_feedback_divider);
	CASE_SET_SDRAM_PARAM(pllm_post_divider);
	CASE_SET_SDRAM_PARAM(pllm_stable_time);
	CASE_SET_SDRAM_PARAM(emc_clock_divider);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_interval);
	CASE_SET_SDRAM_PARAM(emc_auto_cal_config);   
	CASE_SET_SDRAM_PARAM(emc_auto_cal_wait);
	CASE_SET_SDRAM_PARAM(emc_pin_program_wait);
	CASE_SET_SDRAM_PARAM(emc_rc);
	CASE_SET_SDRAM_PARAM(emc_rfc);
	CASE_SET_SDRAM_PARAM(emc_ras);
	CASE_SET_SDRAM_PARAM(emc_rp);
	CASE_SET_SDRAM_PARAM(emc_r2w);
	CASE_SET_SDRAM_PARAM(emc_w2r);
	CASE_SET_SDRAM_PARAM(emc_r2p);
	CASE_SET_SDRAM_PARAM(emc_w2p);
	CASE_SET_SDRAM_PARAM(emc_rd_rcd);
	CASE_SET_SDRAM_PARAM(emc_wr_rcd);
	CASE_SET_SDRAM_PARAM(emc_rrd);
	CASE_SET_SDRAM_PARAM(emc_rext);
	CASE_SET_SDRAM_PARAM(emc_wdv);
	CASE_SET_SDRAM_PARAM(emc_quse);
	CASE_SET_SDRAM_PARAM(emc_qrst);
	CASE_SET_SDRAM_PARAM(emc_qsafe);
	CASE_SET_SDRAM_PARAM(emc_rdv);
	CASE_SET_SDRAM_PARAM(emc_refresh);
	CASE_SET_SDRAM_PARAM(emc_burst_refresh_num);
	CASE_SET_SDRAM_PARAM(emc_pdex2wr);
	CASE_SET_SDRAM_PARAM(emc_pdex2rd);
	CASE_SET_SDRAM_PARAM(emc_pchg2pden);
	CASE_SET_SDRAM_PARAM(emc_act2pden);
	CASE_SET_SDRAM_PARAM(emc_ar2pden);
	CASE_SET_SDRAM_PARAM(emc_rw2pden);
	CASE_SET_SDRAM_PARAM(emc_txsr);
	CASE_SET_SDRAM_PARAM(emc_tcke);
	CASE_SET_SDRAM_PARAM(emc_tfaw);
	CASE_SET_SDRAM_PARAM(emc_trpab);
	CASE_SET_SDRAM_PARAM(emc_tclkstable);
	CASE_SET_SDRAM_PARAM(emc_tclkstop);
	CASE_SET_SDRAM_PARAM(emc_trefbw);
	CASE_SET_SDRAM_PARAM(emc_quse_extra);
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg1);
	CASE_SET_SDRAM_PARAM(emc_fbio_dqsib_dly);
	CASE_SET_SDRAM_PARAM(emc_fbio_dqsib_dly_msb);
	CASE_SET_SDRAM_PARAM(emc_fbio_quse_dly);
	CASE_SET_SDRAM_PARAM(emc_fbio_quse_dly_msb);
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg5);
	CASE_SET_SDRAM_PARAM(emc_fbio_cfg6);
	CASE_SET_SDRAM_PARAM(emc_fbio_spare);
	CASE_SET_SDRAM_PARAM(emc_mrs);
	CASE_SET_SDRAM_PARAM(emc_emrs);
	CASE_SET_SDRAM_PARAM(emc_mrw1);
	CASE_SET_SDRAM_PARAM(emc_mrw2);
	CASE_SET_SDRAM_PARAM(emc_mrw3);
	CASE_SET_SDRAM_PARAM(emc_mrw_reset_command);
	CASE_SET_SDRAM_PARAM(emc_mrw_reset_ninit_wait);
	CASE_SET_SDRAM_PARAM(emc_adr_cfg);
	CASE_SET_SDRAM_PARAM(emc_adr_cfg1);
	CASE_SET_SDRAM_PARAM(mc_emem_Cfg);
	CASE_SET_SDRAM_PARAM(mc_lowlatency_config);
	CASE_SET_SDRAM_PARAM(emc_cfg);
	CASE_SET_SDRAM_PARAM(emc_cfg2);
	CASE_SET_SDRAM_PARAM(emc_dbg);
	CASE_SET_SDRAM_PARAM(ahb_arbitration_xbar_ctrl);
	CASE_SET_SDRAM_PARAM(emc_cfg_dig_dll);
	CASE_SET_SDRAM_PARAM(emc_dll_xform_dqs);
	CASE_SET_SDRAM_PARAM(emc_dll_xform_quse);
	CASE_SET_SDRAM_PARAM(warm_boot_wait);
	CASE_SET_SDRAM_PARAM(emc_ctt_term_ctrl);
	CASE_SET_SDRAM_PARAM(emc_odt_write);
	CASE_SET_SDRAM_PARAM(emc_odt_read);
	CASE_SET_SDRAM_PARAM(emc_zcal_ref_cnt);
	CASE_SET_SDRAM_PARAM(emc_zcal_wait_cnt);
	CASE_SET_SDRAM_PARAM(emc_zcal_mrw_cmd);
	CASE_SET_SDRAM_PARAM(emc_mrs_reset_dll);
	CASE_SET_SDRAM_PARAM(emc_mrw_zq_init_dev0);
	CASE_SET_SDRAM_PARAM(emc_mrw_zq_init_dev1);
	CASE_SET_SDRAM_PARAM(emc_mrw_zq_init_wait);
	CASE_SET_SDRAM_PARAM(emc_mrs_reset_dll_wait);
	CASE_SET_SDRAM_PARAM(emc_emrs_emr2);
	CASE_SET_SDRAM_PARAM(emc_emrs_emr3);
	CASE_SET_SDRAM_PARAM(emc_emrs_ddr2_dll_enable);
	CASE_SET_SDRAM_PARAM(emc_mrs_ddr2_dll_reset);
	CASE_SET_SDRAM_PARAM(emc_emrs_ddr2_ocd_calib);
	CASE_SET_SDRAM_PARAM(emc_ddr2_wait);
	CASE_SET_SDRAM_PARAM(emc_cfg_clktrim0);
	CASE_SET_SDRAM_PARAM(emc_cfg_clktrim1);
	CASE_SET_SDRAM_PARAM(emc_cfg_clktrim2);
	CASE_SET_SDRAM_PARAM(pmc_ddr_pwr);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfga_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgc_pad_ctrl2);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2cfgd_pad_ctrl2);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2clkcfg_Pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2comp_pad_ctrl);
	CASE_SET_SDRAM_PARAM(apb_misc_gp_xm2vttgen_pad_ctrl);

	default:
		return -ENODATA;
	}

	return 0;
}
static int
getdev_param(u_int32_t set,
		nvbct_lib_id id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {
	CASE_GET_DEV_PARAM(nand, clock_divider);
	CASE_GET_DEV_PARAM(nand, nand_timing);
	CASE_GET_DEV_PARAM(nand, nand_timing2);
	CASE_GET_DEV_PARAM(nand, block_size_log2);
	CASE_GET_DEV_PARAM(nand, page_size_log2);

	CASE_GET_DEV_PARAM(sdmmc, clock_divider);
	CASE_GET_DEV_PARAM(sdmmc, data_width);
	CASE_GET_DEV_PARAM(sdmmc, max_power_class_supported);

	CASE_GET_DEV_PARAM(spiflash, clock_source);
	CASE_GET_DEV_PARAM(spiflash, clock_divider);
	CASE_GET_DEV_PARAM(spiflash, read_command_type_fast);

	case nvbct_lib_id_dev_type:
		*data = bct_ptr->dev_type[set];
		break;

	default:
		return -ENODATA;
	}

	return 0;
}

static int
setdev_param(u_int32_t set,
		nvbct_lib_id id,
		u_int32_t data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

	if (bct == NULL)
		return -ENODATA;

	switch (id) {
	CASE_SET_DEV_PARAM(nand, clock_divider);
	CASE_SET_DEV_PARAM(nand, nand_timing);
	CASE_SET_DEV_PARAM(nand, nand_timing2);
	CASE_SET_DEV_PARAM(nand, block_size_log2);
	CASE_SET_DEV_PARAM(nand, page_size_log2);

	CASE_SET_DEV_PARAM(sdmmc, clock_divider);
	CASE_SET_DEV_PARAM(sdmmc, data_width);
	CASE_SET_DEV_PARAM(sdmmc, max_power_class_supported);

	CASE_SET_DEV_PARAM(spiflash, clock_source);
	CASE_SET_DEV_PARAM(spiflash, clock_divider);
	CASE_SET_DEV_PARAM(spiflash, read_command_type_fast);

	case nvbct_lib_id_dev_type:
		bct_ptr->dev_type[set] = data;
		break;

	default:
		return -ENODATA;
	}

	return 0;

}

static int
getbl_param(u_int32_t set,
		nvbct_lib_id id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

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

	case nvbct_lib_id_bl_crypto_hash:
		memcpy(data,
		&(bct_ptr->bootloader[set].crypto_hash),
		sizeof(nvboot_hash));
	break;

	default:
		return -ENODATA;
	}

	return 0;
}

/* Note: The *Data argument is to support hash data. */
static int
setbl_param(u_int32_t set,
		nvbct_lib_id id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

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

	case nvbct_lib_id_bl_crypto_hash:
		memcpy(&(bct_ptr->bootloader[set].crypto_hash),
		data,
		sizeof(nvboot_hash));
		break;

	default:
		return -ENODATA;
	}

	return 0;
}


static int
bct_get_value(nvbct_lib_id id, u_int32_t *data, u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;
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

	/*
	 * Constants.
	 */

	CASE_GET_CONST(bootloaders_max,   NVBOOT_MAX_BOOTLOADERS);
	CASE_GET_CONST(reserved_size,     NVBOOT_BCT_RESERVED_SIZE);

	case nvbct_lib_id_reserved_offset:
		*data = (u_int8_t*)&(samplebct.reserved)
				- (u_int8_t*)&samplebct;
		break;

	case nvbct_lib_id_bct_size:
		*data = sizeof(nvboot_config_table);
		break;

	CASE_GET_CONST(hash_size, sizeof(nvboot_hash));

	case nvbct_lib_id_crypto_offset:
		/* Offset to region in BCT to encrypt & sign */
		*data = (u_int8_t*)&(samplebct.random_aes_blk)
				- (u_int8_t*)&samplebct;
		break;

	case nvbct_lib_id_crypto_length:
		/* size   of region in BCT to encrypt & sign */
		*data = sizeof(nvboot_config_table) - sizeof(nvboot_hash);
	break;

	CASE_GET_CONST(max_bct_search_blks, NVBOOT_MAX_BCT_SEARCH_BLOCKS);

	CASE_GET_CONST_PREFIX(dev_type_nand, nvboot);
	CASE_GET_CONST_PREFIX(dev_type_sdmmc, nvboot);
	CASE_GET_CONST_PREFIX(dev_type_spi, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_4bit, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_8bit, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllp_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllc_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllm_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_clockm, nvboot);

	CASE_GET_CONST_PREFIX(memory_type_none, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_ddr, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_lpddr, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_ddr2, nvboot);
	CASE_GET_CONST_PREFIX(memory_type_lpddr2, nvboot);

	default:
		return -ENODATA;
	}
	return 0;
}

static int
bct_set_value(nvbct_lib_id id, u_int32_t  data, u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

	if (bct == NULL)
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

	default:
		return -ENODATA;
    }

    return 0;
}



/*
 * Note: On input, *length is the size of Data.  On output, *length is the
 * actual size used.
 */
static int
bct_get_data(nvbct_lib_id id,
	u_int8_t *data,
	u_int32_t *length,
	u_int8_t *bct)
{
	return 0;
}

static int
bct_set_data(nvbct_lib_id id,
	u_int8_t *data,
	u_int32_t  length,
	u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {

	CASE_SET_DATA(crypto_hash, sizeof(nvboot_hash));

	default:
		return -ENODATA;
	}

	return 0;
}


void
nvbct_lib_get_fns(nvbct_lib_fns *fns)
{
	fns->get_value = bct_get_value;
	fns->set_value = bct_set_value;

	fns->get_data = bct_get_data;
	fns->set_data = bct_set_data;

	fns->getbl_param = getbl_param;
	fns->setbl_param = setbl_param;

	fns->getdev_param = getdev_param;
	fns->setdev_param = setdev_param;

	fns->get_sdram_params = get_sdram_params;
	fns->set_sdram_params = set_sdram_params;
}
