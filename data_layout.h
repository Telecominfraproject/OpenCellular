/**
 * Copyright (c) 2012 NVIDIA Corporation.  All rights reserved.
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

void
update_context(struct build_image_context_rec *context);

int
read_bct_file(struct build_image_context_rec *context);

int
init_bct(struct build_image_context_rec *context);

int
write_block_raw(struct build_image_context_rec *context);

int
begin_update(build_image_context *context);

#endif /* #ifndef INCLUDED_DATA_LAYOUT_H */
