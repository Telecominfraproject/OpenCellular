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
 * set.h - Definitions for the cbootimage state setting code.
 */

#ifndef INCLUDED_SET_H
#define INCLUDED_SET_H

#include "cbootimage.h"
#include "parse.h"
#include "string.h"
#include "sys/stat.h"

int
context_set_array(build_image_context	*context,
		u_int32_t	index,
		parse_token	token,
		u_int32_t	value);

int
set_bootloader(build_image_context	*context,
		char	*filename,
		u_int32_t	load_addr,
		u_int32_t	entry_point);

int
context_set_value(build_image_context	*context,
		parse_token	token,
		u_int32_t	value);

int
set_addon_filename(build_image_context	*context,
		char	*filename,
		int	index);

int
set_addon_attr(build_image_context *context,
		u_int32_t	file_attr,
		int	index);

int
set_unique_name(build_image_context *context,
		char	*uname,
		int	index);

int
set_other_field(build_image_context *context,
		char	*other_str,
		int	other,
		int	index);

int
set_nand_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t value);

int
set_sdmmc_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t value);

int
set_spiflash_param(build_image_context *context,
		u_int32_t index,
		parse_token token,
		u_int32_t value);

int
read_from_image(char *filename,
		u_int32_t	page_size,
		u_int8_t	**Image,
		u_int32_t	*storage_size,
		u_int32_t	*actual_size,
		file_type	f_type);

#endif /* #ifndef INCLUDED_SET_H */
