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
 * cbootimage.h - Definitions for the cbootimage code.
 */

#ifndef INCLUDED_BUILDIMAGE_H
#define INCLUDED_BUILDIMAGE_H

#include "nvboot_bct.h"
#include "nvbctlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#define NVBOOT_AES_BLOCK_SIZE_LOG2 4

#define KEY_LENGTH (128/8)

#define MAX_BUFFER 200
#define MAX_STR_LEN	20
#define MAX_BOOTLOADER_SIZE (16 * 1024 * 1024)

/*
 * Enumerations
 */

typedef enum
{
	file_type_bl = 0,
	file_type_bct,
	file_type_addon
} file_type;
/*
 * Structures
 */
typedef struct item_rec
{
	u_int8_t unique_name[4];
	u_int32_t Location;
	u_int32_t size;
	u_int32_t attribute;
	u_int32_t reserve[4];
	u_int32_t item_checksum[4];
}item;

typedef struct addon_item_rec
{
	struct item_rec item;
	char  addon_filename[MAX_BUFFER];
	int item_index;
	struct addon_item_rec *next;
}addon_item;

typedef struct table_rec
{
	u_int8_t magic_id[8];
	u_int32_t table_checksum[4];
	u_int32_t table_size;

}table;

typedef struct addon_table_rec
{
	struct table_rec table;
	u_int8_t addon_item_no;
	struct addon_item_rec *addon_item_list;
}addon_table;

typedef struct build_image_context_rec
{
	FILE *config_file;
	char *image_filename;
	FILE *raw_file;
	nvbct_lib_fns bctlib;
	u_int32_t block_size;
	u_int32_t block_size_log2;
	u_int32_t page_size;
	u_int32_t page_size_log2;
	u_int32_t pages_per_blk;
	u_int32_t partition_size;
	u_int32_t redundancy;
	u_int32_t version;
	/* Allocation data. */
	struct blk_data_rec *memory; /* Representation of memory */
	/* block number for the (first) journal block */
	u_int32_t journal_blk;

	char *newbl_filename;
	u_int32_t newbl_load_addr;
	u_int32_t newbl_entry_point;
	u_int32_t newbl_attr;
	u_int8_t *bct;

	struct addon_table_rec addon_tbl;
	char *bct_filename;
	u_int32_t last_bl_blk;
	u_int32_t addon_tbl_blk;
} build_image_context;

/* Function prototypes */

int write_image_file(build_image_context *context);

/* Global data */
extern int enable_debug;

/* Useful macros */

#define GET_VALUE(id, ptr)                                    \
    (void)context->bctlib.get_value(nvbct_lib_id_##id,           \
                                   ptr,                       \
                                   context->bct);

#define SET_VALUE(id, value)                                  \
    (void)context->bctlib.set_value(nvbct_lib_id_##id,           \
                                   value,                     \
                                   context->bct);

#endif /* #ifndef INCLUDED_BUILDIMAGE_H */
