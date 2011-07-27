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
 * data_layout.c - Code to manage the layout of data in the boot device.
 *
 */

#include "data_layout.h"
#include "cbootimage.h"
#include "crypto.h"
#include "set.h"
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

static int write_bootloaders(build_image_context *context);

static u_int32_t find_new_bct_blk(build_image_context *context);
static int finish_update(build_image_context *context);
static void init_bad_block_table(build_image_context *context);

static u_int32_t
iceil_log2(u_int32_t a, u_int32_t b)
{
	return (a + (1 << b) - 1) >> b;
}

/* Returns the smallest power of 2 >= a */
static u_int32_t
ceil_log2(u_int32_t a)
{
	u_int32_t result;

	result = log2(a);
	if ((1UL << result) < a)
		result++;

	return result;
}

static void init_bad_block_table(build_image_context *context)
{
	u_int32_t bytes_per_entry;
	nvboot_badblock_table *table;
	nvboot_config_table *bct;

	bct = (nvboot_config_table *)(context->bct);

	assert(context != NULL);
	assert(bct != NULL);

	table = &(bct->badblock_table);

	bytes_per_entry = ICEIL(context->partition_size,
				NVBOOT_BAD_BLOCK_TABLE_SIZE);
	table->block_size_log2 = context->block_size_log2;
	table->virtual_blk_size_log2 = NV_MAX(ceil_log2(bytes_per_entry),
					table->block_size_log2);
	table->entries_used = iceil_log2(context->partition_size,
					table->virtual_blk_size_log2);
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
	u_int32_t bct_size;
	u_int32_t pagesremaining;
	u_int32_t page;
	u_int32_t pages_per_bct;
	u_int8_t *buffer;
	u_int8_t *data;
	int e = 0;

	assert(context);

	/* Note: 3rd argument not used in this particular query. */
	(void)context->bctlib.get_value(nvbct_lib_id_bct_size,
			&bct_size, context->bct);

	pages_per_bct = iceil_log2(bct_size, context->page_size_log2);
	pagesremaining = pages_per_bct;
	page = bct_slot * pages_per_bct;

	/* Create a local copy of the BCT data */
	buffer = malloc(pages_per_bct * context->page_size);
	if (buffer == NULL)
		return -ENOMEM;
	memset(buffer, 0, pages_per_bct * context->page_size);

	memcpy(buffer, context->bct, bct_size);

	insert_padding(buffer, bct_size);

	/* Encrypt and compute hash */
	e = sign_bct(context, buffer);
	if (e != 0)
		goto fail;

	/* Write the BCT data to the storage device, picking up ECC errors */
	data = buffer;
	while (pagesremaining > 0) {
		e = write_page(context, block, page, data);
		if (e != 0)
			goto fail;
		page++;
		pagesremaining--;
		data += context->page_size;
	}
fail:
	/* Cleanup */
	free(buffer);
	return e;
}

#define SET_BL_FIELD(instance, field, value)                    \
do {                                                        \
	(void)context->bctlib.setbl_param(instance,              \
		nvbct_lib_id_bl_##field, \
			&(value),              \
			context->bct);         \
} while (0);

#define GET_BL_FIELD(instance, field, ptr)                    \
(void)context->bctlib.getbl_param(instance,                \
		nvbct_lib_id_bl_##field,   \
			ptr,                     \
			context->bct);

#define COPY_BL_FIELD(from, to, field)                        \
do {                                                      \
	u_int32_t v;                                              \
	GET_BL_FIELD(from, field, &v);                        \
	SET_BL_FIELD(to,   field,  v);                        \
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


/*
 * In the interest of expediency, all BL's allocated from bottom to top start
 * at page 0 of a block, and all BL's allocated from top to bottom end at
 * the end of a block.
 */
/* TODO: Check for partition overflow */
/* TODO: Refactor this code! */
static int
write_bootloaders(build_image_context *context)
{
	u_int32_t i;
	u_int32_t j;
	u_int32_t bl_instance;
	u_int32_t bl_move_count = 0;
	u_int32_t bl_move_remaining;
	u_int32_t current_blk;
	u_int32_t current_page;
	u_int32_t  pages_in_bl;
	u_int8_t  *bl_storage; /* Holds the Bl after reading */
	u_int8_t  *buffer;	/* Holds the Bl for writing */
	u_int8_t  *src;	/* Scans through the Bl during writing */
	u_int32_t  bl_length; /* In bytes */
	u_int32_t  bl_actual_size; /* In bytes */
	u_int32_t  pagesremaining;
	u_int32_t  virtual_blk;
	u_int32_t  pages_per_blk;
	u_int32_t  min_offset;
	u_int32_t  max_offset;
	u_int32_t  bl_0_version;
	u_int32_t  bl_used;
	u_int8_t  *hash_buffer;
	u_int32_t  hash_size;
	u_int32_t  bootloaders_max;
	file_type bl_filetype = file_type_bl;
	int e = 0;

	assert(context);

	pages_per_blk = 1 << (context->block_size_log2
			- context->page_size_log2);

	GET_VALUE(hash_size, &hash_size);
	GET_VALUE(bootloaders_max, &bootloaders_max);

	hash_buffer = malloc(hash_size);
	if (hash_buffer == NULL)
		return -ENOMEM;

	if (enable_debug) {
		printf("write_bootloaders()\n");
		printf("  redundancy = %d\n", context->redundancy);
	}

	/* Make room for the Bl(s) in the BCT. */

	/* Determine how many to move.
	 * Note that this code will count Bl[0] only if there is already
	 * a BL in the device.
	 */
	GET_BL_FIELD(0, version, &bl_0_version);
	GET_VALUE(bootloader_used, &bl_used);
	for (bl_instance = 0; bl_instance < bl_used; bl_instance++) {
		u_int32_t bl_version;
		GET_BL_FIELD(bl_instance, version, &bl_version);
		if (bl_version == bl_0_version)
			bl_move_count++;
	}

	/* Adjust the move count, if needed, to avoid overflowing the BL table.
	 * This can happen due to too much redundancy.
	 */
	bl_move_count = MIN(bl_move_count,
			bootloaders_max - context->redundancy);

	/* Move the Bl entries down. */
	bl_move_remaining = bl_move_count;
	while (bl_move_remaining > 0) {
		u_int32_t  inst_from = bl_move_remaining - 1;
		u_int32_t  inst_to   =
			bl_move_remaining + context->redundancy - 1;

		COPY_BL_FIELD(inst_from, inst_to, version);
		COPY_BL_FIELD(inst_from, inst_to, start_blk);
		COPY_BL_FIELD(inst_from, inst_to, start_page);
		COPY_BL_FIELD(inst_from, inst_to, length);
		COPY_BL_FIELD(inst_from, inst_to, load_addr);
		COPY_BL_FIELD(inst_from, inst_to, entry_point);
		COPY_BL_FIELD(inst_from, inst_to, attribute);

		(void)context->bctlib.getbl_param(inst_from,
			nvbct_lib_id_bl_crypto_hash,
			(u_int32_t*)hash_buffer,
			context->bct);
		(void)context->bctlib.setbl_param(inst_to,
			nvbct_lib_id_bl_crypto_hash,
			(u_int32_t*)hash_buffer,
			context->bct);
		bl_move_remaining--;
	}

	/* Read the BL into memory. */
	if (read_from_image(context->newbl_filename,
		context->page_size,
		&bl_storage,
		&bl_length,
		&bl_actual_size,
		bl_filetype) == 1) {
		printf("Error reading Bootloader %s.\n",
			context->newbl_filename);
		exit(1);
	}

	pages_in_bl = iceil_log2(bl_length, context->page_size_log2);

	current_blk = context->next_bct_blk;
	current_page  = 0;
	for (bl_instance = 0; bl_instance < context->redundancy;
					bl_instance++) {

		pagesremaining = pages_in_bl;
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
			min_offset = virtual_blk * context->block_size;
			max_offset = min_offset + context->block_size;

			if (virtual_blk == 0) {
				set_bl_data(context,
					bl_instance,
					current_blk,
					current_page,
					bl_actual_size);
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
	for (bl_instance = 0; bl_instance < context->redundancy;
					bl_instance++) {

		/* Create a local copy of the BCT data */
		buffer = malloc(pages_in_bl * context->page_size);
		if (buffer == NULL)
			return -ENOMEM;

		memset(buffer, 0, pages_in_bl * context->page_size);
		memcpy(buffer, bl_storage, bl_actual_size);
		insert_padding(buffer, bl_actual_size);

		pagesremaining = pages_in_bl;

		GET_BL_FIELD(bl_instance, start_blk, &current_blk);
		GET_BL_FIELD(bl_instance, start_page,  &current_page);

		/* Encrypt and compute hash */
		sign_data_block(buffer,
				bl_actual_size,
				hash_buffer);
		(void)context->bctlib.setbl_param(bl_instance,
				nvbct_lib_id_bl_crypto_hash,
				(u_int32_t*)hash_buffer,
				context->bct);

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

			e = write_page(context,
				current_blk, current_page, src);
			if (e != 0)
				goto fail;
			pagesremaining--;
			src += context->page_size;
			current_page++;
			if (current_page >= pages_per_blk) {
				current_page = 0;
				current_blk++;
				virtual_blk++;
			}
			context->last_bl_blk = current_blk;
		}
		free(buffer);
	}

	(void)context->bctlib.set_value(nvbct_lib_id_bootloader_used,
			context->redundancy + bl_move_count,
			context->bct);

	if (enable_debug) {
		GET_VALUE(bootloader_used, &bl_used);

		for (i = 0; i < bootloaders_max; i++) {
			u_int32_t version;
			u_int32_t start_blk;
			u_int32_t start_page;
			u_int32_t length;
			u_int32_t load_addr;
			u_int32_t entry_point;

			GET_BL_FIELD(i, version,     &version);
			GET_BL_FIELD(i, start_blk,  &start_blk);
			GET_BL_FIELD(i, start_page,   &start_page);
			GET_BL_FIELD(i, length,      &length);
			GET_BL_FIELD(i, load_addr, &load_addr);
			GET_BL_FIELD(i, entry_point,  &entry_point);

			printf("%sBL[%d]: %d %04d %04d %04d 0x%08x 0x%08x k=",
				i < bl_used ? "  " : "**",
				i,
				version,
				start_blk,
				start_page,
				length,
				load_addr,
				entry_point);

			(void)context->bctlib.getbl_param(i,
				nvbct_lib_id_bl_crypto_hash,
				(u_int32_t*)hash_buffer,
				context->bct);
			for (j = 0; j < hash_size / 4; j++) {
				printf("%08x",
					*((u_int32_t*)(hash_buffer + 4*j)));
			}
			printf("\n");
		}
	}
	free(bl_storage);
	free(hash_buffer);
	return 0;
fail:
	/* Cleanup. */
	free(buffer);
	free(bl_storage);
	free(hash_buffer);
	printf("Write bootloader failed, error: %d.\n", e);
	return e;
}

int
update_addon_item(build_image_context *context)
{
	u_int8_t *aoi_storage;
	u_int8_t *buffer=NULL;
	u_int8_t *src=NULL;
	u_int32_t aoi_length;
	u_int32_t aoi_actual_size;
	u_int32_t pages_count;
	u_int32_t pagesremaining;
	u_int32_t current_blk;
	u_int32_t current_page;
	u_int32_t pages_per_blk;
	u_int32_t table_length;
	u_int32_t hash_size;
	int i;
	u_int8_t magicid[8] = "ChromeOs";
	struct addon_item_rec *current_item;
	file_type aoi_filetype = file_type_addon;
	int e = 0;

	/* Read the Addon item into memory. */
	GET_VALUE(hash_size, &hash_size);
	pages_per_blk = 1 << (context->block_size_log2
		- context->page_size_log2);
	current_blk = context->last_bl_blk;

	/* Get the addon table block number */
	current_blk++;
	context->addon_tbl_blk = current_blk;

	/* write the addon item */
	current_item = context->addon_tbl.addon_item_list;
	for(i = 0; i < context->addon_tbl.addon_item_no; i++) {

		if (read_from_image(current_item->addon_filename,
				context->page_size,
				&aoi_storage,
				&aoi_length,
				&aoi_actual_size,
				aoi_filetype) == 1) {
			printf("Error reading addon file %s.\n",
			context->addon_tbl.addon_item_list->addon_filename);

			exit(1);
		}
		pages_count = iceil_log2(aoi_length, context->page_size_log2);

		/* Create a local copy of the BCT data */
		buffer = malloc(pages_count * context->page_size);
		if (buffer == NULL)
			return -ENOMEM;

		memset(buffer, 0, pages_count * context->page_size);
		memcpy(buffer, aoi_storage, aoi_actual_size);
		insert_padding(buffer, aoi_actual_size);
		/* Encrypt and compute hash */
		sign_data_block(buffer,
			aoi_actual_size,
			(u_int8_t *)current_item->item.item_checksum);

		pagesremaining = pages_count;
		src = buffer;
		current_blk++;
		current_page = 0;
		current_item->item.Location = current_blk;
		current_item->item.size = aoi_actual_size;
		while (pagesremaining) {
			if (current_page == 0) {
				/* Erase the block before writing into it. */
				e = erase_block(context, current_blk);
				if (e != 0)
					goto fail_on_item;
			}
			e = write_page(context,
				current_blk, current_page, src);
			if (e != 0)
				goto fail_on_item;
			pagesremaining--;
			src += context->page_size;
			current_page++;
			if (current_page >= pages_per_blk) {
				current_page = 0;
				current_blk++;
			}
		}
		current_item = current_item->next;
		free(aoi_storage);
		free(buffer);
	}

	/* write add on table */
	current_blk = context->addon_tbl_blk;
	current_item = context->addon_tbl.addon_item_list;
	current_page = 0;
	table_length = sizeof(struct table_rec) +
		context->addon_tbl.addon_item_no * sizeof(struct item_rec);
	context->addon_tbl.table.table_size= table_length;
	memcpy(context->addon_tbl.table.magic_id, magicid, 8);
	pages_count = iceil_log2(table_length, context->page_size_log2);
	buffer = malloc(pages_count * context->page_size);
	if (buffer == NULL)
		return -ENOMEM;

	memset(buffer, 0, pages_count * context->page_size);
	src = buffer;
	memcpy(src, &context->addon_tbl, sizeof(struct table_rec));
	src += sizeof(struct table_rec);

	for(i = 0; i < context->addon_tbl.addon_item_no; i++) {
		memcpy(src, current_item, sizeof(struct item_rec));
		src += sizeof(struct item_rec);
		current_item = current_item->next;
	}
	insert_padding(buffer, table_length);

	pagesremaining = pages_count;
	src = buffer;

	/* Encrypt and compute hash */
	e = sign_data_block(buffer,
		table_length,
		(u_int8_t *)context->addon_tbl.table.table_checksum);
	if (e != 0)
		goto fail_on_table;
	memcpy(src+sizeof(context->addon_tbl.table.magic_id),
			context->addon_tbl.table.table_checksum,
			hash_size);

	while (pagesremaining) {
		if (current_page == 0) {
			e = erase_block(context, current_blk);
			if (e != 0)
				goto fail_on_table;
		}
		e = write_page(context, current_blk, current_page, src);
		if(e != 0)
			goto fail_on_table;
		pagesremaining--;
		src += context->page_size;
		current_page++;
	}
	free(buffer);
	return 0;

fail_on_item:
	free(aoi_storage);

fail_on_table:
	free(buffer);
	return e;
}

void
update_context(struct build_image_context_rec *context)
{
	(void)context->bctlib.get_value(nvbct_lib_id_partition_size,
			&context->partition_size,
			context->bct);
	(void)context->bctlib.get_value(nvbct_lib_id_page_size_log2,
			&context->page_size_log2,
			context->bct);
	(void)context->bctlib.get_value(nvbct_lib_id_block_size_log2,
			&context->block_size_log2,
			context->bct);

	context->page_size = 1 << context->page_size_log2;
	context->block_size = 1 << context->block_size_log2;
	context->pages_per_blk = 1 << (context->block_size_log2 -
                                           context->page_size_log2);
}
void
read_bct_file(struct build_image_context_rec *context)
{
	u_int8_t  *bct_storage; /* Holds the Bl after reading */
	u_int32_t  bct_length; /* In bytes */
	u_int32_t  bct_actual_size; /* In bytes */
	u_int32_t  bct_size;
	file_type bct_filetype = file_type_bct;

	bct_size = sizeof(nvboot_config_table);
	if (read_from_image(context->bct_filename,
		context->page_size,
		&bct_storage,
		&bct_length,
		&bct_actual_size,
		bct_filetype) == 1) {
		printf("Error reading bct file %s.\n", context->bct_filename);
		exit(1);
	}
	memcpy(context->bct, bct_storage, bct_size);
	free(bct_storage);
	update_context(context);
}

void destroy_addon_list(struct addon_item_rec *addon_list)
{
	struct addon_item_rec *next;

	while (addon_list) {
		next = addon_list->next;
		free(addon_list);
		addon_list = next;
	}
}

static u_int32_t
find_new_bct_blk(build_image_context *context)
{
	u_int32_t current_blk;
	u_int32_t max_bct_search_blks;

	assert(context);

	current_blk = context->next_bct_blk;

	GET_VALUE(max_bct_search_blks, &max_bct_search_blks);

	if (current_blk >= max_bct_search_blks) {
		printf("Error: Unable to locate a journal block.\n");
		exit(1);
	}
	context->next_bct_blk++;
	return current_blk;
}

/*
 * Logic for updating a BCT or BL:
 * - begin_update():
 *   - If the device is blank:
 *     - Identify & erase a journal block.
 *   - If the journal block has gone bad:
 *     - Perform an UpdateBL to move the good bootloader out of the
 *       way, if needed.
 *     - Erase the new journal block
 *     - Write the good BCT to slot 0 of the new journal block.
 *   - If the journal block is full:
 *     - Erase block 0
 *     - Write 0's to BCT slot 0.
 *     - Write the good BCT to slot 1.
 *     - Erase the journal block.
 *     - Write the good BCT to slot 0 of the journal block.
 *   - Erase block 0
 * - If updating the BL, do so here.
 * - finish_update():
 *   - Write the new BCT to the next available of the journal block.
 *   - Write the new BCT to slot 0 of block 0.
 */

/* - begin_update():
 *   - Test the following conditions in this order:
 *     - If the device is blank:
 *       - Identify & erase a journal block.
 *     - If the journal block has gone bad:
 *       - Perform an UpdateBL to move the good bootloader out of the
 *         way, if needed.
 *       - Erase the new journal block
 *       - Write the good BCT to slot 0 of the new journal block.
 *     - If the journal block is full:
 *       - Erase block 0
 *       - Write 0's to BCT slot 0.
 *       - Write the good BCT to slot 1.
 *       - Erase the journal block.
 *       - Write the good BCT to slot 0 of the journal block.
 *   - Erase block 0
 */
int
begin_update(build_image_context *context)
{
	u_int32_t pages_per_bct;
	u_int32_t pages_per_blk;
	u_int32_t bct_size;
	u_int32_t hash_size;
	u_int32_t reserved_size;
	u_int32_t reserved_offset;
	u_int32_t current_bct_blk;
	int e = 0;
	int i;

	assert(context);

	/* Ensure that the BCT block & page data is current. */
	if (enable_debug) {
		u_int32_t block_size_log2;
		u_int32_t page_size_log2;

		GET_VALUE(block_size_log2, &block_size_log2);
		GET_VALUE(page_size_log2,  &page_size_log2);

		printf("begin_update(): bct data: b=%d p=%d\n",
			block_size_log2, page_size_log2);
	}

	SET_VALUE(boot_data_version, NVBOOT_BOOTDATA_VERSION(2, 1));
	GET_VALUE(bct_size,  &bct_size);
	GET_VALUE(hash_size, &hash_size);
	GET_VALUE(reserved_size, &reserved_size);
	GET_VALUE(reserved_offset, &reserved_offset);

	pages_per_bct = iceil_log2(bct_size, context->page_size_log2);
	pages_per_blk = (1 << (context->block_size_log2
		- context->page_size_log2));
	/* Initialize the bad block table field. */
	init_bad_block_table(context);
	/* Fill the reserved data w/the padding pattern. */
	write_padding(context->bct + reserved_offset, reserved_size);

	/* Find the next bct block starting at block 0. */
	for (i = 0; i < context->bct_copy; i++) {
		current_bct_blk = find_new_bct_blk(context);
		e = erase_block(context, current_bct_blk);
		if (e != 0)
			goto fail;
	}
	return 0;
fail:
	printf("Erase block failed, error: %d.\n", e);
	return e;
}


/*
 * - finish_update():
 *   - Write the new BCT to the next available of the journal block.
 *   - Write the new BCT to slot 0 of block 0.
 * For now, ignore end state.
 */
static int
finish_update(build_image_context *context)
{
	u_int32_t current_bct_blk;
	int e = 0;
	int i;

	current_bct_blk = context->next_bct_blk;
	for (i = 0; i < context->bct_copy; i++) {
		e = write_bct(context, --current_bct_blk, 0);
		if (e != 0)
			goto fail;
	}

	return 0;
fail:
	printf("Write BCT failed, error: %d.\n", e);
	return e;
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
	if (write_bootloaders(context) != 0)
		return 1;
	if (finish_update(context) != 0)
		return 1;
	return 0;
}
/*
 * To write the current image:
 * - Loop over all blocks in the block data list:
 *   - Write out the data of real blocks.
 *   - Write out 0's for unused blocks.
 *   - Stop on the last used page of the last used block.
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

			if (fwrite(data, 1, bytes, context->raw_file) != bytes)
				return -1;
		}
	}

	free(empty_blk);
	return 0;
}
