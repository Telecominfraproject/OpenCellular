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
 * set.h - Definitions for the cbootimage state setting code.
 */

#ifndef INCLUDED_SET_H
#define INCLUDED_SET_H

#include "cbootimage.h"
#include "parse.h"
#include "string.h"
#include "sys/stat.h"

int
set_bootloader(build_image_context	*context,
		char	*filename,
		uint32_t	load_addr,
		uint32_t	entry_point);

int
set_mts_image(build_image_context	*context,
		char	*filename,
		uint32_t	load_addr,
		uint32_t	entry_point);

int
set_rsa_param(build_image_context	*context,
		parse_token	token,
		char	*filename);

int
context_set_value(build_image_context	*context,
		parse_token	token,
		void		*value);

int
read_from_image(char *filename,
		uint32_t	offset,
		uint32_t	max_size,
		uint8_t	**Image,
		uint32_t	*actual_size,
		file_type	f_type);

#endif /* #ifndef INCLUDED_SET_H */
