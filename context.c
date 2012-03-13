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

#include "cbootimage.h"
#include "data_layout.h"
#include "set.h"

void
cleanup_context(build_image_context *context)
{
	destroy_block_list(context->memory);
	free(context->bct);
}

int
init_context(build_image_context *context)
{
	/* Set defaults */
	context->memory = new_block_list();
	context->next_bct_blk = 0; /* Default to block 0 */
	context_set_value(context, token_redundancy, 1);
	context_set_value(context, token_version, 1);
	context_set_value(context, token_bct_copy, 2);

	return 0;
}
