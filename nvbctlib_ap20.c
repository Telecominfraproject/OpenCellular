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
getdev_param(u_int32_t set,
		nvbct_lib_id id,
		u_int32_t *data,
		u_int8_t *bct)
{
	nvboot_config_table *bct_ptr = (nvboot_config_table*)bct;

	if (data == NULL || bct == NULL)
		return -ENODATA;

	switch (id) {
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

	CASE_GET_CONST_PREFIX(dev_type_sdmmc, nvboot);
	CASE_GET_CONST_PREFIX(dev_type_spi, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_4bit, nvboot);
	CASE_GET_CONST_PREFIX(sdmmc_data_width_8bit, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllp_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllc_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_pllm_out0, nvboot);
	CASE_GET_CONST_PREFIX(spi_clock_source_clockm, nvboot);

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

}
