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
 * data_layout.h - Definitions for the cbootimage data layout code.
 */

#ifndef INCLUDED_DATA_LAYOUT_H
#define INCLUDED_DATA_LAYOUT_H

#include "cbootimage.h"

/* Foward declarations */
struct build_image_context_rec;

typedef struct blk_data_rec *blk_data_handle;

blk_data_handle new_block_list(void);
void destroy_block_list(blk_data_handle);

int
update_bl(struct build_image_context_rec *context);

int
update_mts_image(build_image_context *context);

void
update_context(struct build_image_context_rec *context);

int
read_bct_file(struct build_image_context_rec *context);

int
init_bct(struct build_image_context_rec *context);

int
write_block_raw(struct build_image_context_rec *context);

int
write_data_block(FILE *fp, u_int32_t offset, u_int32_t size, u_int8_t *buffer);

int
data_is_valid_bct(build_image_context *context);

int
get_bct_size_from_image(build_image_context *context);

int
begin_update(build_image_context *context);

#endif /* #ifndef INCLUDED_DATA_LAYOUT_H */
