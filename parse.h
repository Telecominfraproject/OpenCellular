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
	token_sdmmc_params,
	token_spiflash_params,
	token_data_width,
	token_clock_divider,
	token_clock_source,
	token_read_command_type_fast,
	token_max_power_class_supported,

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
