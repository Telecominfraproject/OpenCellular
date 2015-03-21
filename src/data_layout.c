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
 * data_layout.c - Code to manage the layout of data in the boot device.
 *
 */

#include "data_layout.h"
#include "cbootimage.h"
#include "crypto.h"
#include "set.h"
#include "context.h"
#include "parse.h"
#include <sys/param.h>

typedef struct blk_data_rec
{
	u_int32_t blk_number;
	u_int32_t pages_used; /* pages always used starting from 0. */
	u_int8_t *data;

	/* Pointer to ECC errors? */

	struct blk_data_rec *next;
} block_data;

/* Function prototypes */
static block_data
*new_block(u_int32_t blk_number, u_int32_t block_size);
static block_data
*find_block(u_int32_t blk_number, block_data  *block_list);
static block_data
*add_block(u_int32_t blk_number, block_data **block_list,
			u_int32_t block_size);
static int
erase_block(build_image_context *context, u_int32_t blk_number);

static int
write_page(build_image_context *context,
			u_int32_t blk_number,
			u_int32_t page_number,
			u_int8_t *data);

static void
insert_padding(u_int8_t *data, u_int32_t length);

static void
write_padding(u_int8_t *data, u_int32_t length);

static int write_bct(build_image_context *context,
		u_int32_t block,
		u_int32_t bct_slot);

static void
set_bl_data(build_image_context *context,
			u_int32_t instance,
			u_int32_t start_blk,
			u_int32_t start_page,
			u_int32_t length);

static int write_image(build_image_context *context, file_type image_type);

static void find_new_bct_blk(build_image_context *context);
static int finish_update(build_image_context *context);

u_int32_t
iceil_log2(u_int32_t a, u_int32_t b)
{
	return (a + (1 << b) - 1) >> b;
}

/* Returns the smallest power of 2 >= a */
u_int32_t
ceil_log2(u_int32_t a)
{
	u_int32_t result;

	result = log2(a);
	if ((1UL << result) < a)
		result++;

	return result;
}

static block_data *new_block(u_int32_t blk_number, u_int32_t block_size)
{
	block_data *new_block = malloc(sizeof(block_data));
	if (new_block == NULL)
		return NULL;

	new_block->blk_number = blk_number;
	new_block->pages_used = 0;
	new_block->data = malloc(block_size);
	if (new_block->data == NULL) {
		free(new_block);
		return NULL;
	}
	new_block->next = NULL;

	memset(new_block->data, 0, block_size);

	return new_block;
}

block_data *new_block_list(void)
{
	return NULL;
}

void destroy_block_list(block_data *block_list)
{
	block_data *next;

	while (block_list) {
		next = block_list->next;
		free(block_list->data);
		free(block_list);
		block_list = next;
	}
}

static block_data *find_block(u_int32_t blk_number, block_data  *block_list)
{
	while (block_list) {
		if (block_list->blk_number == blk_number)
			return block_list;

		block_list = block_list->next;
	}

	return NULL;
}

/* Returns pointer to block after adding it to block_list, if needed. */
static block_data *add_block(u_int32_t blk_number,
		block_data **block_list,
		u_int32_t block_size)
{
	block_data *block = find_block(blk_number,*block_list);
	block_data *parent;

	if (block == NULL) {
		block = new_block(blk_number, block_size);
		if (block == NULL)
			return block;

		/* Insert block into the list */
		if ((*block_list == NULL) ||
			(blk_number < (*block_list)->blk_number)) {
			block->next = *block_list;
			*block_list = block;
		} else {
			/* Search for the correct place to insert the block. */
			parent = *block_list;
			while (parent->next != NULL &&
				parent->next->blk_number < blk_number) {
				parent = parent->next;
			}

			block->next = parent->next;
			parent->next = block;
		}
	}

	return block;
}

static int
erase_block(build_image_context *context, u_int32_t blk_number)
{
	block_data   *block;

	assert(context != NULL);

	block = add_block(blk_number, &(context->memory), context->block_size);

	if (block == NULL)
		return -ENOMEM;
	if (block->data == NULL)
		return -ENOMEM;

	memset(block->data, 0, context->block_size);
	block->pages_used = 0;
	return 0;
}

static int
write_page(build_image_context *context,
	u_int32_t blk_number,
	u_int32_t page_number,
	u_int8_t *data)
{
	block_data *block;
	u_int8_t *page_ptr;

	assert(context);

	block = add_block(blk_number, &(context->memory), context->block_size);

	if (block == NULL)
		return -ENOMEM;
	if (block->data == NULL)
		return -ENOMEM;
	assert(((page_number + 1) * context->page_size)
			<= context->block_size);

	if (block->pages_used != page_number) {
		printf("Warning: Writing page in block out of order.\n");
		printf("     block=%d  page=%d\n", blk_number, page_number);
	}

	page_ptr = block->data + (page_number * context->page_size);
	memcpy(page_ptr, data, context->page_size);
	if (block->pages_used < (page_number+1))
		block->pages_used = page_number+1;
	return 0;
}

static void
insert_padding(u_int8_t *data, u_int32_t length)
{
	u_int32_t aes_blks;
	u_int32_t remaining;

	aes_blks = iceil_log2(length, NVBOOT_AES_BLOCK_SIZE_LOG2);
	remaining = (aes_blks << NVBOOT_AES_BLOCK_SIZE_LOG2) - length;

	write_padding(data + length, remaining);
}

static void
write_padding(u_int8_t *p, u_int32_t remaining)
{
	u_int8_t value = 0x80;

	while (remaining) {
		*p++ = value;
		remaining--;
		value = 0x00;
	}
}

static int
write_bct(build_image_context *context,
	u_int32_t block,
	u_int32_t bct_slot)
{
	u_int32_t pagesremaining;
	u_int32_t page;
	u_int32_t pages_per_bct;
	u_int8_t *buffer;
	u_int8_t *data;
	int err = 0;

	assert(context);

	pages_per_bct = iceil_log2(context->bct_size, context->page_size_log2);
	pagesremaining = pages_per_bct;
	page = bct_slot * pages_per_bct;

	/* Create a local copy of the BCT data */
	buffer = malloc(pages_per_bct * context->page_size);
	if (buffer == NULL)
		return -ENOMEM;
	memset(buffer, 0, pages_per_bct * context->page_size);

	memcpy(buffer, context->bct, context->bct_size);

	insert_padding(buffer, context->bct_size);

	/* Encrypt and compute hash */
	err = sign_bct(context, buffer);
	if (err != 0)
		goto fail;

	/* Write the BCT data to the storage device, picking up ECC errors */
	data = buffer;
	while (pagesremaining > 0) {
		err = write_page(context, block, page, data);
		if (err != 0)
			goto fail;
		page++;
		pagesremaining--;
		data += context->page_size;
	}
fail:
	/* Cleanup */
	free(buffer);
	return err;
}

#define SET_BL_FIELD(instance, field, value)   \
do {                                           \
    g_soc_config->setbl_param(instance,        \
        token_bl_##field,                      \
        &(value),                              \
        context->bct);                         \
} while (0);

#define GET_BL_FIELD(instance, field, ptr)     \
g_soc_config->getbl_param(instance,            \
        token_bl_##field,                      \
        ptr,                                   \
        context->bct);

#define COPY_BL_FIELD(from, to, field)         \
do {                                           \
    u_int32_t v;                               \
    GET_BL_FIELD(from, field, &v);             \
    SET_BL_FIELD(to,   field,  v);             \
} while (0);

#define SET_MTS_FIELD(instance, field, value)   \
do {                                            \
    g_soc_config->set_mts_info(context,         \
    instance,                                   \
    token_mts_info_##field,                     \
    value);                                     \
} while (0);

#define GET_MTS_FIELD(instance, field, ptr)     \
g_soc_config->get_mts_info(context,             \
        instance,                               \
        token_mts_info_##field,                 \
        ptr);

#define COPY_MTS_FIELD(from, to, field)         \
do {                                            \
    u_int32_t v;                                \
    GET_MTS_FIELD(from, field, &v);             \
    SET_MTS_FIELD(to,   field,  v);             \
} while (0);

#define SET_FIELD(is_bl, instance, field, value)\
do {                                            \
    if (is_bl) {                                \
        SET_BL_FIELD(instance, field,value);    \
    }                                           \
    else {                                      \
        SET_MTS_FIELD(instance, field, value);  \
    }                                           \
} while (0);

#define GET_FIELD(is_bl, instance, field, ptr)  \
do {                                            \
    if (is_bl) {                                \
        GET_BL_FIELD(instance, field, ptr);     \
    }                                           \
    else {                                      \
        GET_MTS_FIELD(instance, field, ptr);    \
    }                                           \
} while (0);

#define COPY_FIELD(is_bl, from, to, field)      \
do {                                            \
    if (is_bl) {                                \
        COPY_BL_FIELD(from, to, field);         \
    }                                           \
    else {                                      \
        COPY_MTS_FIELD(from, to, field);        \
    }                                           \
} while (0);

static void
set_bl_data(build_image_context *context,
		u_int32_t instance,
		u_int32_t start_blk,
		u_int32_t start_page,
		u_int32_t length)
{
	assert(context);

	SET_BL_FIELD(instance, version, context->version);
	SET_BL_FIELD(instance, start_blk, start_blk);
	SET_BL_FIELD(instance, start_page, start_page);
	SET_BL_FIELD(instance, length, length);
	SET_BL_FIELD(instance, load_addr, context->newbl_load_addr);
	SET_BL_FIELD(instance, entry_point, context->newbl_entry_point);
	SET_BL_FIELD(instance, attribute, context->newbl_attr);
}

static void
set_mts_data(build_image_context *context,
		u_int32_t instance,
		u_int32_t start_blk,
		u_int32_t start_page,
		u_int32_t length)
{
	assert(context);

	SET_MTS_FIELD(instance, version, context->version);
	SET_MTS_FIELD(instance, start_blk, start_blk);
	SET_MTS_FIELD(instance, start_page, start_page);
	SET_MTS_FIELD(instance, length, length);
	SET_MTS_FIELD(instance, load_addr, context->mts_load_addr);
	SET_MTS_FIELD(instance, entry_point, context->mts_entry_point);
	SET_MTS_FIELD(instance, attribute, context->mts_attr);
}

#define SET_DATA(is_bl, context, instance, start_blk, start_page, length) \
do {                                                                      \
    if (is_bl)                                                            \
        set_bl_data(context, instance, start_blk, start_page, length);    \
    else                                                                  \
        set_mts_data(context, instance, start_blk, start_page, length);   \
} while (0);

/*
 * Load the image then update it with the information from config file.
 * In the interest of expediency, all image's allocated from bottom to top
 * start at page 0 of a block, and all image's allocated from top to bottom
 * end at the end of a block.
 *
 * @param context      The main context pointer
 * @param image_type   The image type. Can be file_type_bl or file_type_mts
 *                     only right now
 * @return 0 for success
 */
static int
write_image(build_image_context *context, file_type image_type)
{
	u_int32_t i, j;
	u_int32_t image_instance;
	u_int32_t image_move_count = 0;
	u_int32_t image_move_remaining;
	u_int32_t current_blk;
	u_int32_t current_page;
	u_int32_t pages_in_image;
	u_int32_t image_used;
	u_int8_t  *image_storage; /* Holds the image after reading */
	u_int8_t  *buffer;	/* Holds the image for writing */
	u_int8_t  *src;	/* Scans through the image during writing */
	u_int32_t  image_actual_size; /* In bytes */
	u_int32_t  pagesremaining;
	u_int32_t  virtual_blk;
	u_int32_t  pages_per_blk;
	u_int32_t  image_version;
	u_int8_t  *hash_buffer;
	u_int32_t  hash_size;
	u_int32_t  image_max;
	parse_token token;
	int err = 0, is_bl;

	assert(context);

	/* Only support bootloader and mts image right now */
	if (image_type == file_type_bl) {
		is_bl = 1;
	}
	else if (image_type == file_type_mts) {
		is_bl = 0;
	}
	else {
		printf("Not supported image type!\n");
		return -EINVAL;
	}

	pages_per_blk = 1 << (context->block_size_log2
			- context->page_size_log2);

	g_soc_config->get_value(token_hash_size,
			&hash_size, context->bct);
	token = is_bl ? token_bootloaders_max : token_mts_max;
	g_soc_config->get_value(token, &image_max, context->bct);

	hash_buffer = calloc(1, hash_size);
	if (hash_buffer == NULL)
		return -ENOMEM;

	if (enable_debug) {
		printf("writing %s\n", is_bl ? "bootloader" : "mts image");
		printf("  redundancy = %d\n", context->redundancy);
	}

	/* Make room for the image in the BCT. */

	/* Determine how many to move.
	 * Note that this code will count Mts[0] only if there is already
	 * a mts in the device.
	 */
	GET_FIELD(is_bl, 0, version, &image_version);
	token = is_bl ? token_bootloader_used : token_mts_used;
	g_soc_config->get_value(token, &image_used, context->bct);
	for (image_instance = 0; image_instance < image_used; image_instance++) {
		u_int32_t tmp;
		GET_FIELD(is_bl, image_instance, version, &tmp);
		if (tmp == image_version)
			image_move_count++;
	}

	/* Adjust the move count, if needed, to avoid overflowing the mts table.
	 * This can happen due to too much redundancy.
	 */
	image_move_count = MIN(image_move_count, image_max - context->redundancy);

	/* Move the mts entries down. */
	image_move_remaining = image_move_count;
	while (image_move_remaining > 0) {
		u_int32_t  inst_from = image_move_remaining - 1;
		u_int32_t  inst_to   =
			image_move_remaining + context->redundancy - 1;

		COPY_FIELD(is_bl, inst_from, inst_to, version);
		COPY_FIELD(is_bl, inst_from, inst_to, start_blk);
		COPY_FIELD(is_bl, inst_from, inst_to, start_page);
		COPY_FIELD(is_bl, inst_from, inst_to, length);
		COPY_FIELD(is_bl, inst_from, inst_to, load_addr);
		COPY_FIELD(is_bl, inst_from, inst_to, entry_point);
		COPY_FIELD(is_bl, inst_from, inst_to, attribute);

		if (is_bl) {
			g_soc_config->getbl_param(inst_from,
				token_bl_crypto_hash,
				(u_int32_t*)hash_buffer,
				context->bct);
			g_soc_config->setbl_param(inst_to,
				token_bl_crypto_hash,
				(u_int32_t*)hash_buffer,
				context->bct);
		}

		image_move_remaining--;
	}

	/* Read the image into memory. */
	if (read_from_image(
		is_bl ? context->newbl_filename : context->mts_filename,
		0,
		is_bl ? MAX_BOOTLOADER_SIZE : MAX_MTS_SIZE,
		&image_storage,
		&image_actual_size,
		image_type) == 1) {
		printf("Error reading image %s.\n",
			is_bl ? context->newbl_filename : context->mts_filename);
		exit(1);
	}

	pages_in_image = iceil_log2(image_actual_size, context->page_size_log2);

	current_blk = context->next_bct_blk;
	current_page  = 0;
	for (image_instance = 0; image_instance < context->redundancy;
					image_instance++) {

		pagesremaining = pages_in_image;
		/* Advance to the next block if needed. */
		if (current_page > 0) {
			current_blk++;
			current_page = 0;
		}

		virtual_blk = 0;

		while (pagesremaining > 0) {
			/* Update the bad block table with relative
			  * bad blocks.
			  */
			if (virtual_blk == 0) {
				SET_DATA(is_bl,
					context,
					image_instance,
					current_blk,
					current_page,
					image_actual_size);
			}

			if (pagesremaining > pages_per_blk) {
				current_blk++;
				virtual_blk++;
				pagesremaining -= pages_per_blk;
			} else {
				current_page = pagesremaining;
				pagesremaining = 0;
			}
		}
	}

	/* Scan forwards to write each copy. */
	for (image_instance = 0; image_instance < context->redundancy;
					image_instance++) {

		/* Create a local copy of the BCT data */
		buffer = malloc(pages_in_image * context->page_size);
		if (buffer == NULL)
			return -ENOMEM;

		memset(buffer, 0, pages_in_image * context->page_size);
		memcpy(buffer, image_storage, image_actual_size);

		insert_padding(buffer, image_actual_size);

		pagesremaining = pages_in_image;

		if (is_bl) {
			GET_BL_FIELD(image_instance, start_blk, &current_blk);
			GET_BL_FIELD(image_instance, start_page,  &current_page);

			/* Encrypt and compute hash */
			sign_data_block(buffer,
					image_actual_size,
					hash_buffer);
			g_soc_config->setbl_param(image_instance,
					token_bl_crypto_hash,
					(u_int32_t*)hash_buffer,
					context->bct);
		}

		GET_FIELD(is_bl, image_instance, start_blk, &current_blk);
		GET_FIELD(is_bl, image_instance, start_page,  &current_page);

		/* Write the BCT data to the storage device,
		 * picking up ECC errors
		 */
		src = buffer;

		/* Write pages as we go. */
		virtual_blk = 0;
		while (pagesremaining) {
			if (current_page == 0) {
				/* Erase the block before writing into it. */
				erase_block(context, current_blk);
			}

			err = write_page(context,
				current_blk, current_page, src);
			if (err != 0)
				goto fail;
			pagesremaining--;
			src += context->page_size;
			current_page++;
			if (current_page >= pages_per_blk) {
				current_page = 0;
				current_blk++;
				virtual_blk++;
			}
			context->last_blk = current_blk;
		}
		context->next_bct_blk = context->last_blk + 1;
		free(buffer);
	}

	image_used = context->redundancy + image_move_count;
	token = is_bl ? token_bootloader_used : token_mts_used;
	g_soc_config->set_value(token, &image_used, context->bct);

	if (enable_debug) {
		for (i = 0; i < image_max; i++) {
			u_int32_t version;
			u_int32_t start_blk;
			u_int32_t start_page;
			u_int32_t length;
			u_int32_t load_addr;
			u_int32_t entry_point;

			GET_FIELD(is_bl, i, version,     &version);
			GET_FIELD(is_bl, i, start_blk,  &start_blk);
			GET_FIELD(is_bl, i, start_page,   &start_page);
			GET_FIELD(is_bl, i, length,      &length);
			GET_FIELD(is_bl, i, load_addr, &load_addr);
			GET_FIELD(is_bl, i, entry_point,  &entry_point);

			printf("%s%s[%d]: %d %04d %04d %04d 0x%08x 0x%08x\n",
				i < image_used ? "  " : "**",
				is_bl ? "BL" : "MTS",
				i,
				version,
				start_blk,
				start_page,
				length,
				load_addr,
				entry_point);
			if (is_bl) {
				g_soc_config->getbl_param(i,
					token_bl_crypto_hash,
					(u_int32_t*)hash_buffer,
					context->bct);
				for (j = 0; j < hash_size / 4; j++) {
					printf("%08x",
						*((u_int32_t*)(hash_buffer + 4*j)));
				}
				printf("\n");
			}
		}
	}
	free(image_storage);
	free(hash_buffer);
	return 0;

fail:
	/* Cleanup. */
	free(buffer);
	free(image_storage);
	free(hash_buffer);
	printf("Write image failed, error: %d.\n", err);
	return err;
}

void
update_context(struct build_image_context_rec *context)
{
	g_soc_config->get_value(token_partition_size,
			&context->partition_size,
			context->bct);
	g_soc_config->get_value(token_page_size_log2,
			&context->page_size_log2,
			context->bct);
	g_soc_config->get_value(token_block_size_log2,
			&context->block_size_log2,
			context->bct);
	g_soc_config->get_value(token_odm_data,
			&context->odm_data,
			context->bct);

	context->page_size = 1 << context->page_size_log2;
	context->block_size = 1 << context->block_size_log2;
	context->pages_per_blk = 1 << (context->block_size_log2 -
                                           context->page_size_log2);
}

/*
 * Allocate and initialize the memory for bct data.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
init_bct(struct build_image_context_rec *context)
{
	/* Allocate space for the bct.	 */
	context->bct = malloc(context->bct_size);

	if (context->bct == NULL)
		return -ENOMEM;

	memset(context->bct, 0, context->bct_size);
	context->bct_init = 1;

	return 0;
}

/*
 * Read the bct data from given file to allocated memory.
 * Assign the global parse interface to corresponding hardware interface
 *   according to the boot data version in bct file.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
read_bct_file(struct build_image_context_rec *context)
{
	u_int8_t  *bct_storage; /* Holds the Bl after reading */
	u_int32_t  bct_actual_size; /* In bytes */
	file_type bct_filetype = file_type_bct;
	int err = 0;

	if (read_from_image(context->bct_filename,
			0,
			NVBOOT_CONFIG_TABLE_SIZE_MAX,
			&bct_storage,
			&bct_actual_size,
			bct_filetype) == 1) {
		printf("Error reading bct file %s.\n", context->bct_filename);
		exit(1);
	}

	context->bct_size = bct_actual_size;
	if (context->bct_init != 1)
		err = init_bct(context);
	if (err != 0) {
		printf("Context initialization failed.  Aborting.\n");
		return err;
	}
	memcpy(context->bct, bct_storage, context->bct_size);
	free(bct_storage);

	if (!data_is_valid_bct(context))
		return ENODATA;

	return err;
}

/*
 * Update the next_bct_blk and make it point to the next
 * new blank block according to bct_copy given.
 *
 * @param context		The main context pointer
 */
static void
find_new_bct_blk(build_image_context *context)
{
	u_int32_t max_bct_search_blks;

	assert(context);

	g_soc_config->get_value(token_max_bct_search_blks,
			&max_bct_search_blks, context->bct);

	if (context->next_bct_blk > max_bct_search_blks) {
		printf("Error: Unable to locate a journal block.\n");
		exit(1);
	}
	context->next_bct_blk++;
}

/*
 * Initialization before bct and bootloader update.
 * Find the new blank block and erase it.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
begin_update(build_image_context *context)
{
	u_int32_t hash_size;
	u_int32_t reserved_size;
	u_int32_t reserved_offset;
	int err = 0;
	int i;

	assert(context);

	/* Ensure that the BCT block & page data is current. */
	if (enable_debug) {
		u_int32_t block_size_log2;
		u_int32_t page_size_log2;

		g_soc_config->get_value(token_block_size_log2,
			&block_size_log2, context->bct);
		g_soc_config->get_value(token_page_size_log2,
			&page_size_log2, context->bct);

		printf("begin_update(): bct data: b=%d p=%d\n",
			block_size_log2, page_size_log2);
	}

	g_soc_config->set_value(token_boot_data_version,
			&(context->boot_data_version), context->bct);
	g_soc_config->get_value(token_hash_size,
			&hash_size, context->bct);
	g_soc_config->get_value(token_reserved_size,
			&reserved_size, context->bct);
	g_soc_config->get_value(token_reserved_offset,
			&reserved_offset, context->bct);
	/* Set the odm data */
	g_soc_config->set_value(token_odm_data,
			&(context->odm_data), context->bct);

	/* Initialize the bad block table field. */
	g_soc_config->init_bad_block_table(context);
	/* Fill the reserved data w/the padding pattern. */
	write_padding(context->bct + reserved_offset, reserved_size);

	/* Create the pad before the BCT starting at block 1 */
	for (i = 0; i < context->pre_bct_pad_blocks; i++) {
		find_new_bct_blk(context);
		err = erase_block(context, i);
		if (err != 0)
			goto fail;
	}
	/* Find the next bct block starting at block pre_bct_pad_blocks. */
	for (i = 0; i < context->bct_copy; i++) {
		find_new_bct_blk(context);
		err = erase_block(context, i + context->pre_bct_pad_blocks);
		if (err != 0)
			goto fail;
	}
	return 0;
fail:
	printf("Erase block failed, error: %d.\n", err);
	return err;
}

/*
 * Write the BCT(s) starting at slot 0 of block context->pre_bct_pad_blocks.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
static int
finish_update(build_image_context *context)
{
	int err = 0;
	int i;

	for (i = 0; i < context->bct_copy; i++) {
		err = write_bct(context, i + context->pre_bct_pad_blocks, 0);
		if (err != 0)
			goto fail;
	}

	return 0;
fail:
	printf("Write BCT failed, error: %d.\n", err);
	return err;
}

/*
 * For now, ignore end state.
 */
int
update_bl(build_image_context *context)
{
	if (enable_debug)
		printf("**update_bl()\n");

	if (begin_update(context) != 0)
		return 1;
	if (write_image(context, file_type_bl) != 0)
		return 1;
	if (finish_update(context) != 0)
		return 1;
	return 0;
}

int
update_mts_image(build_image_context *context)
{
	if (enable_debug)
		printf("**update_mts()\n");

	if (begin_update(context) != 0)
		return 1;
	if (write_image(context, file_type_mts) != 0)
		return 1;
	if (finish_update(context) != 0)
		return 1;
	return 0;
}

/*
 * To write the current image:
 *   Loop over all blocks in the block data list:
 *     Write out the data of real blocks.
 *     Write out 0's for unused blocks.
 *     Stop on the last used page of the last used block.
 *
 * @param context		The main context pointer
 * @return 0 for success
 */
int
write_block_raw(build_image_context *context)
{
	block_data *block_list;
	block_data *block;
	u_int32_t blk_number;
	u_int32_t last_blk;
	u_int32_t pages_to_write;
	u_int8_t *data;
	u_int8_t *empty_blk = NULL;

	assert(context != NULL);
	assert(context->memory);

	block_list = context->memory;

	/* Compute the end of the image. */
	block_list = context->memory;
	while (block_list->next)
		block_list = block_list->next;

	last_blk = block_list->blk_number;

	/* Loop over all the storage from block 0, page 0 to
	 *last_blk, Lastpage
	 */
	for (blk_number = 0; blk_number <= last_blk; blk_number++) {
		block = find_block(blk_number, context->memory);
		if (block)	{
			pages_to_write = (blk_number == last_blk) ?
			block->pages_used :
			context->pages_per_blk;
			data = block->data;
		} else {
			/* Allocate empty_blk if needed. */
			if (empty_blk == NULL) {
				empty_blk = malloc(context->block_size);
				if (!empty_blk)
					return -ENOMEM;
				memset(empty_blk, 0, context->block_size);
			}
			pages_to_write = context->pages_per_blk;
			data = empty_blk;
		}
		/* Write the data */
		{
			size_t bytes = pages_to_write * context->page_size;

			if (fwrite(data, 1, bytes, context->raw_file) != bytes) {
				if (empty_blk) free(empty_blk);
				return -1;
			}
		}
	}

	if (empty_blk) free(empty_blk);
	return 0;
}

int write_data_block(FILE *fp, u_int32_t offset, u_int32_t size, u_int8_t *buffer)
{
	if (fseek(fp, offset, 0))
		return -1;

	return fwrite(buffer, 1, size, fp);
}

int data_is_valid_bct(build_image_context *context)
{
	/* get proper soc_config pointer by polling each supported chip */
	if (if_bct_is_t20_get_soc_config(context, &g_soc_config))
		return 1;
	if (if_bct_is_t30_get_soc_config(context, &g_soc_config))
		return 1;
	if (if_bct_is_t114_get_soc_config(context, &g_soc_config))
		return 1;
	if (if_bct_is_t124_get_soc_config(context, &g_soc_config))
		return 1;
	if (if_bct_is_t132_get_soc_config(context, &g_soc_config))
		return 1;
	if (if_bct_is_t210_get_soc_config(context, &g_soc_config))
		return 1;

	return 0;
}

int get_bct_size_from_image(build_image_context *context)
{
	u_int8_t buffer[NVBOOT_CONFIG_TABLE_SIZE_MAX];
	u_int32_t bct_size = 0;
	FILE *fp;

	fp = fopen(context->input_image_filename, "r");
	if (!fp)
		return ENODATA;

	if (fread(buffer, 1, NVBOOT_CONFIG_TABLE_SIZE_MAX, fp) != NVBOOT_CONFIG_TABLE_SIZE_MAX) {
		fclose(fp);
		return ENODATA;
	}

	context->bct = buffer;
	if (data_is_valid_bct(context) && g_soc_config->get_bct_size)
		bct_size = g_soc_config->get_bct_size();

	fclose(fp);
	context->bct = 0;
	return bct_size;
}
