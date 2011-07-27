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
#include "data_layout.h"
#include "set.h"

static int
query_alloc(build_image_context *context,
        nvbct_lib_id size_id,
        u_int8_t **dst)
{
        u_int32_t size;

        /* Note: 3rd argument not used in this particular query. */
        if (context->bctlib.get_value(size_id, &size, context->bct) != 0)
                return -ENODATA;

        *dst = malloc(size);

        if (*dst == NULL)
                return -ENOMEM;

        memset(*dst, 0, size);

        return 0;
}

void
cleanup_context(build_image_context *context)
{
        destroy_block_list(context->memory);
        destroy_addon_list(context->addon_tbl.addon_item_list);
        free(context->bct);
}

int
init_context(build_image_context *context)
{
        int e = 0;

        /* Set defaults */
        context->memory = new_block_list();
        context->next_bct_blk = 0; /* Default to block 0 */

        /* Allocate space for the bct.
         * Note that this is different from the old code which pointed directly
         * into a memory image.
         */
        e = query_alloc(context, nvbct_lib_id_bct_size, &(context->bct));
        if (e != 0)
                goto fail;

        context_set_value(context, token_page_size, 2048);
        context_set_value(context, token_redundancy, 1);
        context_set_value(context, token_version, 1);
        context_set_value(context, token_bct_copy, 2);

        return 0;

 fail:
        cleanup_context(context);

        return e;
}
