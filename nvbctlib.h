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
	nvbct_lib_id_dev_type_sdmmc,
	nvbct_lib_id_dev_type_spi,

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
} nvbct_lib_fns;

void nvbct_lib_get_fns(nvbct_lib_fns *fns);

#endif /* #ifndef INCLUDED_NVBCTLIB_H */
