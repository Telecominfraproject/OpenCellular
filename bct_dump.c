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
#include "nvbctlib.h"
#include "data_layout.h"
#include "context.h"

#include <string.h>

int enable_debug = 0;

typedef struct {
	nvbct_lib_id id;
	char const * message;
} value_data;

static value_data const	values[] = {
	{nvbct_lib_id_boot_data_version,   "Version................: 0x%08x\n"},
	{nvbct_lib_id_block_size_log2,     "Block size (log2)......: %d\n"},
	{nvbct_lib_id_page_size_log2,      "Page size (log2).......: %d\n"},
	{nvbct_lib_id_partition_size,      "Parition size..........: 0x%08x\n"},
	{nvbct_lib_id_bootloader_used,     "Bootloader used........: %d\n"},
	{nvbct_lib_id_bootloaders_max,     "Bootloaders max........: %d\n"},
	{nvbct_lib_id_bct_size,            "BCT size...............: %d\n"},
	{nvbct_lib_id_hash_size,           "Hash size..............: %d\n"},
	{nvbct_lib_id_crypto_offset,       "Crypto offset..........: %d\n"},
	{nvbct_lib_id_crypto_length,       "Crypto length..........: %d\n"},
	{nvbct_lib_id_max_bct_search_blks, "Max BCT search blocks..: %d\n"},
};

static value_data const	bl_values[] = {
	{nvbct_lib_id_bl_version,     "    Version.......: 0x%08x\n"},
	{nvbct_lib_id_bl_start_blk,   "    Start block...: %d\n"},
	{nvbct_lib_id_bl_start_page,  "    Start page....: %d\n"},
	{nvbct_lib_id_bl_length,      "    Length........: %d\n"},
	{nvbct_lib_id_bl_load_addr,   "    Load address..: 0x%08x\n"},
	{nvbct_lib_id_bl_entry_point, "    Entry point...: 0x%08x\n"},
	{nvbct_lib_id_bl_attribute,   "    Attributes....: 0x%08x\n"},
};

static void
usage(void)
{
	printf("Usage: bct_dump bctfile\n");
	printf("    bctfile       BCT filename to read and display\n");
}

int
main(int argc, char *argv[])
{
	int e;
	build_image_context context;
	u_int32_t bootloaders_used;
	u_int32_t data;
	int i;
	int j;

	if (argc != 2)
		usage();

	memset(&context, 0, sizeof(build_image_context));

	context.bct_filename = argv[1];

	/* Set up the Nvbctlib function pointers. */
	nvbct_lib_get_fns(&(context.bctlib));

	e = init_context(&context);
	if (e != 0) {
		printf("context initialization failed.  Aborting.\n");
		return e;
	}

	read_bct_file(&context);

	for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
		e = context.bctlib.get_value(values[i].id, &data, context.bct);
		printf(values[i].message, e == 0 ? data : -1);
	}

	e = context.bctlib.get_value(nvbct_lib_id_bootloader_used,
				     &bootloaders_used,
				     context.bct);

	for (i = 0; (e == 0) && (i < bootloaders_used); ++i) {
		printf("Bootloader[%d]\n", i);

		for (j = 0; j < sizeof(bl_values) / sizeof(bl_values[0]); ++j) {
			e = context.bctlib.getbl_param(i,
						       bl_values[j].id,
						       &data,
						       context.bct);
			printf(bl_values[j].message, e == 0 ? data : -1);
		}
	}

	/* Clean up memory. */
	cleanup_context(&context);

	return e;
}
