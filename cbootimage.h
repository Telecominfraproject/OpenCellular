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
 * cbootimage.h - Definitions for the cbootimage code.
 */

#ifndef INCLUDED_BUILDIMAGE_H
#define INCLUDED_BUILDIMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#define NVBOOT_AES_BLOCK_SIZE_LOG2 4
#define MAX_BUFFER 200
#define MAX_STR_LEN	20
#define MAX_BOOTLOADER_SIZE (16 * 1024 * 1024)
#define NVBOOT_BOOTDATA_VERSION(a, b) ((((a)&0xffff) << 16) | ((b)&0xffff))
#define NVBOOT_BAD_BLOCK_TABLE_SIZE 4096
#define NV_MAX(a, b) (((a) > (b)) ? (a) : (b))

/*
 * Enumerations
 */

typedef enum
{
	file_type_bl = 0,
	file_type_bct,
} file_type;

/*
 * The main context data structure of cbootimage tool
 */
typedef struct build_image_context_rec
{
	FILE *config_file;
	char *image_filename;
	FILE *raw_file;
	u_int32_t block_size;
	u_int32_t block_size_log2;
	u_int32_t page_size;
	u_int32_t page_size_log2;
	u_int32_t pages_per_blk;
	u_int32_t partition_size;
	u_int32_t redundancy;
	u_int32_t version;
	u_int32_t bct_copy;
	/* Allocation data. */
	struct blk_data_rec *memory; /* Representation of memory */
	/* block number for the BCT block */
	u_int32_t next_bct_blk;

	char *newbl_filename;
	u_int32_t newbl_load_addr;
	u_int32_t newbl_entry_point;
	u_int32_t newbl_attr;
	u_int8_t generate_bct;
	u_int8_t *bct;

	char *bct_filename;
	u_int32_t last_bl_blk;
	u_int32_t bct_size; /* The BCT file size */
	u_int32_t boot_data_version; /* The boot data version of BCT */
	u_int8_t bct_init; /* The flag for the memory allocation of bct */
} build_image_context;

/* Function prototypes */

int write_image_file(build_image_context *context);

/* Global data */
extern int enable_debug;

#endif /* #ifndef INCLUDED_BUILDIMAGE_H */
