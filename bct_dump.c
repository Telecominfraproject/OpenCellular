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

#include "cbootimage.h"
#include "nvbctlib.h"
#include "data_layout.h"
#include "context.h"

#include <string.h>

int enable_debug = 0;

typedef struct {
	nvbct_lib_id id;
	char const * message;
} value_data;

static value_data const	values[] = {
	{nvbct_lib_id_boot_data_version,
	 "Version..................: 0x%08x\n"},
	{nvbct_lib_id_block_size_log2,
	 "Block size (log2)........: %d\n"},
	{nvbct_lib_id_page_size_log2,
	 "Page size (log2).........: %d\n"},
	{nvbct_lib_id_partition_size,
	 "Parition size............: 0x%08x\n"},
	{nvbct_lib_id_bootloader_used,
	 "Bootloader used..........: %d\n"},
	{nvbct_lib_id_bootloaders_max,
	 "Bootloaders max..........: %d\n"},
	{nvbct_lib_id_bct_size,
	 "BCT size.................: %d\n"},
	{nvbct_lib_id_hash_size,
	 "Hash size................: %d\n"},
	{nvbct_lib_id_crypto_offset,
	 "Crypto offset............: %d\n"},
	{nvbct_lib_id_crypto_length,
	 "Crypto length............: %d\n"},
	{nvbct_lib_id_max_bct_search_blks,
	 "Max BCT search blocks....: %d\n"},
	{nvbct_lib_id_num_param_sets,
	 "Device parameters used...: %d\n"},
};

static value_data const	bl_values[] = {
	{nvbct_lib_id_bl_version,
	 "    Version.......: 0x%08x\n"},
	{nvbct_lib_id_bl_start_blk,
	 "    Start block...: %d\n"},
	{nvbct_lib_id_bl_start_page,
	 "    Start page....: %d\n"},
	{nvbct_lib_id_bl_length,
	 "    Length........: %d\n"},
	{nvbct_lib_id_bl_load_addr,
	 "    Load address..: 0x%08x\n"},
	{nvbct_lib_id_bl_entry_point,
	 "    Entry point...: 0x%08x\n"},
	{nvbct_lib_id_bl_attribute,
	 "    Attributes....: 0x%08x\n"},
};

static value_data const	spi_values[] = {
	{nvbct_lib_id_spiflash_read_command_type_fast,
	 "    Command fast...: %d\n"},
	{nvbct_lib_id_spiflash_clock_source,
	 "    Clock source...: %d\n"},
	{nvbct_lib_id_spiflash_clock_divider,
	 "    Clock divider..: %d\n"},
};

static value_data const	sdmmc_values[] = {
	{nvbct_lib_id_sdmmc_clock_divider,
	 "    Clock divider..: %d\n"},
	{nvbct_lib_id_sdmmc_data_width,
	 "    Data width.....: %d\n"},
	{nvbct_lib_id_sdmmc_max_power_class_supported,
	 "    Power class....: %d\n"},
};

static value_data const	sdram_values[] = {
	{nvbct_lib_id_sdram_pllm_charge_pump_setup_ctrl,
	 "    PLLM_CHARGE_PUMP_SETUP_CTRL.....: 0x%08x\n"},
	{nvbct_lib_id_sdram_pllm_loop_filter_setup_ctrl,
	 "    PLLM_LOOP_FILTER_SETUP_CTRL.....: 0x%08x\n"},
	{nvbct_lib_id_sdram_pllm_input_divider,
	 "    PLLM_INPUT_DIVIDER..............: 0x%08x\n"},
	{nvbct_lib_id_sdram_pllm_feedback_divider,
	 "    PLLM_FEEDBACK_DIVIDER...........: 0x%08x\n"},
	{nvbct_lib_id_sdram_pllm_post_divider,
	 "    PLLM_POST_DIVIDER...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_pllm_stable_time,
	 "    PLLM_STABLE_TIME................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_clock_divider,
	 "    EMC_CLOCK_DIVIDER...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_auto_cal_interval,
	 "    EMC_AUTO_CAL_INTERVAL...........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_auto_cal_config,
	 "    EMC_AUTO_CAL_CONFIG.............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_auto_cal_wait,
	 "    EMC_AUTO_CAL_WAIT...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_pin_program_wait,
	 "    EMC_PIN_PROGRAM_WAIT............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rc,
	 "    EMC_RC..........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rfc,
	 "    EMC_RFC.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_ras,
	 "    EMC_RAS.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rp,
	 "    EMC_RP..........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_r2w,
	 "    EMC_R2W.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_w2r,
	 "    EMC_W2R.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_r2p,
	 "    EMC_R2P.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_w2p,
	 "    EMC_W2P.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rd_rcd,
	 "    EMC_RD_RCD......................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_wr_rcd,
	 "    EMC_WR_RCD......................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rrd,
	 "    EMC_RRD.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rext,
	 "    EMC_REXT........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_wdv,
	 "    EMC_WDV.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_quse,
	 "    EMC_QUSE........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_qrst,
	 "    EMC_QRST........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_qsafe,
	 "    EMC_QSAFE.......................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rdv,
	 "    EMC_RDV.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_refresh,
	 "    EMC_REFRESH.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_burst_refresh_num,
	 "    EMC_BURST_REFRESH_NUM...........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_pdex2wr,
	 "    EMC_PDEX2WR.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_pdex2rd,
	 "    EMC_PDEX2RD.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_pchg2pden,
	 "    EMC_PCHG2PDEN...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_act2pden,
	 "    EMC_ACT2PDEN....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_ar2pden,
	 "    EMC_AR2PDEN.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_rw2pden,
	 "    EMC_RW2PDEN.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_txsr,
	 "    EMC_TXSR........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_tcke,
	 "    EMC_TCKE........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_tfaw,
	 "    EMC_TFAW........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_trpab,
	 "    EMC_TRPAB.......................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_tclkstable,
	 "    EMC_TCLKSTABLE..................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_tclkstop,
	 "    EMC_TCLKSTOP....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_trefbw,
	 "    EMC_TREFBW......................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_quse_extra,
	 "    EMC_QUSE_EXTRA..................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_cfg1,
	 "    EMC_FBIO_CFG1...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_dqsib_dly,
	 "    EMC_FBIO_DQSIB_DLY..............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_dqsib_dly_msb,
	 "    EMC_FBIO_DQSIB_DLY_MSB..........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_quse_dly,
	 "    EMC_FBIO_QUSE_DLY...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_quse_dly_msb,
	 "    EMC_FBIO_QUSE_DLY_MSB...........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_cfg5,
	 "    EMC_FBIO_CFG5...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_cfg6,
	 "    EMC_FBIO_CFG6...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_fbio_spare,
	 "    EMC_FBIO_SPARE..................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrs,
	 "    EMC_MRS.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_emrs,
	 "    EMC_EMRS........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw1,
	 "    EMC_MRW1........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw2,
	 "    EMC_MRW2........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw3,
	 "    EMC_MRW3........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw_reset_command,
	 "    EMC_MRW_RESET_COMMAND...........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw_reset_ninit_wait,
	 "    EMC_MRW_RESET_NINIT_WAIT........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_adr_cfg,
	 "    EMC_ADR_CFG.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_adr_cfg1,
	 "    EMC_ADR_CFG1....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_mc_emem_Cfg,
	 "    MC_EMEM_CFG.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_mc_lowlatency_config,
	 "    MC_LOWLATENCY_CONFIG............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_cfg,
	 "    EMC_CFG.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_cfg2,
	 "    EMC_CFG2........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_dbg,
	 "    EMC_DBG.........................: 0x%08x\n"},
	{nvbct_lib_id_sdram_ahb_arbitration_xbar_ctrl,
	 "    AHB_ARBITRATION_XBAR_CTRL.......: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_cfg_dig_dll,
	 "    EMC_CFG_DIG_DLL.................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_dll_xform_dqs,
	 "    EMC_DLL_XFORM_DQS...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_dll_xform_quse,
	 "    EMC_DLL_XFORM_QUSE..............: 0x%08x\n"},
	{nvbct_lib_id_sdram_warm_boot_wait,
	 "    WARM_BOOT_WAIT..................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_ctt_term_ctrl,
	 "    EMC_CTT_TERM_CTRL...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_odt_write,
	 "    EMC_ODT_WRITE...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_odt_read,
	 "    EMC_ODT_READ....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_zcal_ref_cnt,
	 "    EMC_ZCAL_REF_CNT................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_zcal_wait_cnt,
	 "    EMC_ZCAL_WAIT_CNT...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_zcal_mrw_cmd,
	 "    EMC_ZCAL_MRW_CMD................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrs_reset_dll,
	 "    EMC_MRS_RESET_DLL...............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw_zq_init_dev0,
	 "    EMC_MRW_ZQ_INIT_DEV0............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw_zq_init_dev1,
	 "    EMC_MRW_ZQ_INIT_DEV1............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrw_zq_init_wait,
	 "    EMC_MRW_ZQ_INIT_WAIT............: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrs_reset_dll_wait,
	 "    EMC_MRS_RESET_DLL_WAIT..........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_emrs_emr2,
	 "    EMC_EMRS_EMR2...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_emrs_emr3,
	 "    EMC_EMRS_EMR3...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_emrs_ddr2_dll_enable,
	 "    EMC_EMRS_DDR2_DLL_ENABLE........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_mrs_ddr2_dll_reset,
	 "    EMC_MRS_DDR2_DLL_RESET..........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_emrs_ddr2_ocd_calib,
	 "    EMC_EMRS_DDR2_OCD_CALIB.........: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_ddr2_wait,
	 "    EMC_DDR2_WAIT...................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_cfg_clktrim0,
	 "    EMC_CFG_CLKTRIM0................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_cfg_clktrim1,
	 "    EMC_CFG_CLKTRIM1................: 0x%08x\n"},
	{nvbct_lib_id_sdram_emc_cfg_clktrim2,
	 "    EMC_CFG_CLKTRIM2................: 0x%08x\n"},
	{nvbct_lib_id_sdram_pmc_ddr_pwr,
	 "    PMC_DDR_PWR.....................: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2cfga_pad_ctrl,
	 "    APB_MISC_GP_XM2CFGA_PAD_CTRL....: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2cfgc_pad_ctrl,
	 "    APB_MISC_GP_XM2CFGC_PAD_CTRL....: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2cfgc_pad_ctrl2,
	 "    APB_MISC_GP_XM2CFGC_PAD_CTRL2...: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2cfgd_pad_ctrl,
	 "    APB_MISC_GP_XM2CFGD_PAD_CTRL....: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2cfgd_pad_ctrl2,
	 "    APB_MISC_GP_XM2CFGD_PAD_CTRL2...: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2clkcfg_Pad_ctrl,
	 "    APB_MISC_GP_XM2CLKCFG_PAD_CTRL..: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2comp_pad_ctrl,
	 "    APB_MISC_GP_XM2COMP_PAD_CTRL....: 0x%08x\n"},
	{nvbct_lib_id_sdram_apb_misc_gp_xm2vttgen_pad_ctrl,
	 "    APB_MISC_GP_XM2VTTGEN_PAD_CTRL..: 0x%08x\n"},
};

static const char *sdram_types[nvboot_memory_type_num] = {
	"None", "DDR", "LPDDR", "DDR2", "LPDDR2"
};

static void
usage(void)
{
	printf("Usage: bct_dump bctfile\n");
	printf("    bctfile       BCT filename to read and display\n");
}

int
main(int argc, char *argv[])
{
	int e;
	build_image_context context;
	u_int32_t bootloaders_used;
	u_int32_t parameters_used;
	u_int32_t sdram_used;
	nvboot_dev_type type;
	u_int32_t data;
	int i;
	int j;
	value_data const * device_values;
	int	device_count;

	if (argc != 2)
		usage();

	memset(&context, 0, sizeof(build_image_context));

	context.bct_filename = argv[1];

	/* Set up the Nvbctlib function pointers. */
	nvbct_lib_get_fns(&(context.bctlib));

	e = init_context(&context);
	if (e != 0) {
		printf("context initialization failed.  Aborting.\n");
		return e;
	}

	read_bct_file(&context);

	/* Display root values */
	for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
		e = context.bctlib.get_value(values[i].id, &data, context.bct);
		printf(values[i].message, e == 0 ? data : -1);
	}

	/* Display bootloader values */
	e = context.bctlib.get_value(nvbct_lib_id_bootloader_used,
				     &bootloaders_used,
				     context.bct);

	for (i = 0; (e == 0) && (i < bootloaders_used); ++i) {
		printf("Bootloader[%d]\n", i);

		for (j = 0; j < sizeof(bl_values) / sizeof(bl_values[0]); ++j) {
			e = context.bctlib.getbl_param(i,
						       bl_values[j].id,
						       &data,
						       context.bct);
			printf(bl_values[j].message, e == 0 ? data : -1);
		}
	}

	/* Display device values */
	e = context.bctlib.get_value(nvbct_lib_id_num_param_sets,
				     &parameters_used,
				     context.bct);

	for (i = 0; (e == 0) && (i < parameters_used); ++i) {
		printf("DeviceParameter[%d]\n", i);

		e = context.bctlib.getdev_param(i,
						nvbct_lib_id_dev_type,
						&type,
						context.bct);

		switch (type)
		{
			case nvboot_dev_type_spi:
				printf("    Type...........: SPI\n");
				device_values = spi_values;
				device_count  = (sizeof(spi_values) /
						 sizeof(spi_values[0]));
				break;

			case nvboot_dev_type_sdmmc:
				printf("    Type...........: SDMMC\n");
				device_values = sdmmc_values;
				device_count  = (sizeof(sdmmc_values) /
						 sizeof(sdmmc_values[0]));
				break;

			default:
				printf("    Type...........: Unknown (%d)\n",
				       type);
				device_values = NULL;
				device_count  = 0;
				break;
		}

		for (j = 0; j < device_count; ++j) {
			value_data	value = device_values[j];

			e = context.bctlib.getdev_param(i,
							value.id,
							&data,
							context.bct);
			printf(value.message, e == 0 ? data : -1);
		}
	}

	/* Display SDRAM parameters sets */
	e = context.bctlib.get_value(nvbct_lib_id_num_sdram_sets,
				     &sdram_used,
				     context.bct);

	for (i = 0; (e == 0) && (i < sdram_used); ++i) {
		printf("SDRAMParameter[%d]\n", i);

		e = context.bctlib.get_sdram_params(i,
						nvbct_lib_id_sdram_memory_type,
						&type,
						context.bct);

		printf("    Type............................: %s\n",
			type < nvboot_memory_type_num ? sdram_types[type]
						      : "Unknown");

		for (j = 0; j < sizeof(sdram_values) / sizeof(sdram_values[0]);
		     ++j) {
			e = context.bctlib.get_sdram_params(i,
						       sdram_values[j].id,
						       &data,
						       context.bct);
			printf(sdram_values[j].message, e == 0 ? data : -1);
		}
	}

	/* Clean up memory. */
	cleanup_context(&context);

	return e;
}
