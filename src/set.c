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
 * set.c - State setting support for the cbootimage tool
 */

#include "set.h"
#include "cbootimage.h"
#include "crypto.h"
#include "data_layout.h"

/*
 * Function prototypes
 *
 * ParseXXX() parses XXX in the input
 * SetXXX() sets state based on the parsing results but does not perform
 *      any parsing of its own
 * A ParseXXX() function may call other parse functions and set functions.
 * A SetXXX() function may not call any parseing functions.
 */
#define DEFAULT()                                                     \
	default:                                                      \
		printf("Unexpected token %d at line %d\n",            \
		token, __LINE__);                              \
	return 1

int
read_from_image(char	*filename,
		u_int32_t	offset,
		u_int32_t	max_size,
		u_int8_t	**image,
		u_int32_t	*actual_size,
		file_type	f_type)
{
	int result = 0; /* 0 = success, 1 = failure */
	FILE *fp;
	struct stat stats;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		result = 1;
		return result;
	}

	if (fseek(fp, offset, SEEK_SET) == -1) {
		printf("Error: Couldn't seek to %s(%d)\n", filename, offset);
		result = 1;
		goto cleanup;
	}

	if (stat(filename, &stats) != 0) {
		printf("Error: Unable to query info on bootloader path %s\n",
			filename);
		result = 1;
		goto cleanup;
	}

	if (f_type == file_type_bl) {
		if ((stats.st_size - offset) > max_size) {
			printf("Error: Bootloader file %s is too large.\n",
				filename);
			result = 1;
			goto cleanup;
		}
		*actual_size = (u_int32_t)stats.st_size;
	} else {
		if ((stats.st_size - offset) < max_size)
			*actual_size = stats.st_size - offset;
		else
			*actual_size = max_size;
	}

	*image = malloc(*actual_size);
	if (*image == NULL) {
		result = 1;
		goto cleanup;
	}

	memset(*image, 0, *actual_size);

	if (fread(*image, 1, (size_t)(*actual_size), fp) !=
		(size_t)(*actual_size)) {
		result = 1;
		goto cleanup;
	}

cleanup:
	fclose(fp);
	return result;
}

/*
 * Processes commands to set a bootloader.
 *
 * @param context    	The main context pointer
 * @param filename   	The file name of bootloader
 * @param load_addr  	The load address value for bootloader
 * @param entry_point	The entry point value for bootloader
 * @return 0 and 1 for success and failure
 */
int
set_bootloader(build_image_context	*context,
		char	*filename,
		u_int32_t	load_addr,
		u_int32_t	entry_point)
{
	context->newbl_filename = filename;
	context->newbl_load_addr = load_addr;
	context->newbl_entry_point = entry_point;
	return update_bl(context);
}

/*
 * Processes commands to set a MTS image.
 *
 * @param context    	The main context pointer
 * @param filename   	The file name of MTS image
 * @param load_addr  	The load address value for MTS image
 * @param entry_point	The entry point value for MTS image
 * @return 0 and 1 for success and failure
 */
int
set_mts_image(build_image_context	*context,
		char	*filename,
		u_int32_t	load_addr,
		u_int32_t	entry_point)
{
	context->mts_filename = filename;
	context->mts_load_addr = load_addr;
	context->mts_entry_point = entry_point;
	return update_mts_image(context);
}
#define DEFAULT()                                                     \
	default:                                                      \
		printf("Unexpected token %d at line %d\n",            \
			token, __LINE__);                              \
		return 1

/*
 * General handler for setting values in config files.
 *
 * @param context	The main context pointer
 * @param token  	The parse token value
 * @param value  	The pointer of value to set
 * @return 0 for success
 */
int context_set_value(build_image_context *context,
		parse_token token,
		void *value)
{
	assert(context != NULL);

	switch (token) {
	case token_attribute:
		context->newbl_attr = *((u_int32_t *)value);
		break;

	case token_block_size:
		context->block_size = *((u_int32_t *)value);
		context->block_size_log2 = log2(*((u_int32_t *)value));

		if (context->memory != NULL) {
			printf("Error: Too late to change block size.\n");
			return 1;
		}

		if (context->block_size != (u_int32_t)(1 << context->block_size_log2)) {
			printf("Error: Block size must be a power of 2.\n");
			return 1;
		}
		context->pages_per_blk= 1 << (context->block_size_log2- 
				context->page_size_log2);
		g_soc_config->set_value(token_block_size_log2,
			&(context->block_size_log2), context->bct);
		break;

	case token_partition_size:
		if (context->memory != NULL) {
			printf("Error: Too late to change block size.\n");
			return 1;
		}

		context->partition_size= *((u_int32_t *)value);
		g_soc_config->set_value(token_partition_size,
			value, context->bct);
		break;

	case token_page_size:
		context->page_size = *((u_int32_t *)value);
		context->page_size_log2 = log2(*((u_int32_t *)value));

		if (context->page_size != (u_int32_t)(1 << context->page_size_log2)) {
			printf("Error: Page size must be a power of 2.\n");
			return 1;
		}
		context->pages_per_blk= 1 << (context->block_size_log2- 
			context->page_size_log2);

		g_soc_config->set_value(token_page_size_log2,
			&(context->page_size_log2), context->bct);
		break;
	case token_redundancy:
		context->redundancy = *((u_int32_t *)value);
		break;

	case token_version:
		context->version = *((u_int32_t *)value);
		break;

	case token_bct_copy:
		context->bct_copy = *((u_int32_t *)value);
		break;

	case token_odm_data:
		context->odm_data = *((u_int32_t *)value);
		break;

	case token_pre_bct_pad_blocks:
		if (context->bct_init) {
			printf("Error: Too late to pre-BCT pad.\n");
			return 1;
		}
		context->pre_bct_pad_blocks = *((u_int32_t *)value);
		break;

	case token_secure_jtag_control:
		context->secure_jtag_control = *((u_int32_t *)value);
		g_soc_config->set_value(token_secure_jtag_control,
			value, context->bct);
		break;

	case token_secure_debug_control:
		context->secure_debug_control = *((u_int32_t *)value);
		g_soc_config->set_value(token_secure_debug_control,
			value, context->bct);
		break;

	case token_unique_chip_id:
		memcpy(context->unique_chip_id, value, 16);
		g_soc_config->set_value(token_unique_chip_id,
			value, context->bct);
		break;

	DEFAULT();
	}

	return 0;
}
