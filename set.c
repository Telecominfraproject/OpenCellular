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
 * set.c - State setting support for the cbootimage tool
 */

#include "set.h"
#include "cbootimage.h"
#include "crypto.h"
#include "data_layout.h"

/*
 * Function prototypes
 *
 * ParseXXX() parses XXX in the input
 * SetXXX() sets state based on the parsing results but does not perform
 *      any parsing of its own
 * A ParseXXX() function may call other parse functions and set functions.
 * A SetXXX() function may not call any parseing functions.
 */

int
read_from_image(char	*filename,
		u_int32_t	page_size,
		u_int8_t	**image,
		u_int32_t	*storage_size,
		u_int32_t	*actual_size,
		file_type	f_type)
{
	int result = 0; /* 0 = success, 1 = failure */
	FILE *fp;
	struct stat stats;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		result = 1;
		return result;
	}

	if (stat(filename, &stats) != 0) {
		printf("Error: Unable to query info on bootloader path %s\n",
		           filename);
		result = 1;
		goto cleanup;
	}

	*actual_size  = (u_int32_t)stats.st_size;
	*storage_size =
		(u_int32_t)(ICEIL(stats.st_size, page_size) * page_size);

	if (f_type == file_type_bl) {
		if (stats.st_size > MAX_BOOTLOADER_SIZE) {
			printf("Error: Bootloader file %s is too large.\n",
				filename);
			result = 1;
			goto cleanup;
		}


	/* Workaround for a bug in release 1.0 of the boot rom.
	 * Any BL whose padded size is an integral multiple of page size
	 * has its length extended by 16 bytes to bump it to end on a
	 * partial page.
	 */
		if ((*storage_size - *actual_size) < 16) {
			*actual_size  += 16;
			*storage_size += page_size;
		}
	}
	*image = malloc(*storage_size);
	if (*image == NULL) {
		result = 1;
		goto cleanup;
	}

	memset(*image, 0, *storage_size);

	if (fread(*image, 1, (size_t)stats.st_size, fp) != stats.st_size) {
		result = 1;
		goto cleanup;
	}

cleanup:
	fclose(fp);
	return result;
}


/*
 * set_bootloader(): Processes commands to set a bootloader.
 */
int
set_bootloader(build_image_context	*context,
		char	*filename,
		u_int32_t	load_addr,
		u_int32_t	entry_point)
{
	context->newbl_filename = filename;
	context->newbl_load_addr = load_addr;
	context->newbl_entry_point = entry_point;
	return update_bl(context);
}

#define DEFAULT()                                                     \
	default:                                                      \
		printf("Unexpected token %d at line %d\n",            \
			token, __LINE__);                              \
		return 1

/*
 * context_set_array(): Sets an array value.
 */
int
context_set_array(build_image_context	*context,
		u_int32_t	index,
		parse_token	token,
		u_int32_t	value)
{
	assert(context != NULL);
	assert(context->bct != NULL);

	switch (token) {
	case token_attribute:
		(void)context->bctlib.setbl_param(index,
				nvbct_lib_id_bl_attribute,
				&value,
				context->bct);
		break;

		DEFAULT();
	}
	return 0;
}

/*
 * context_set_value(): General handler for setting values in config files.
 */
int context_set_value(build_image_context *context,
		parse_token token,
		u_int32_t value)
{
	assert(context != NULL);

	switch (token) {
	case token_attribute:
		context->newbl_attr = value;
		break;

	case token_page_size:
		context->page_size = value;
		break;
	case token_redundancy:
		context->redundancy = value;
		break;

	case token_version:
		context->version = value;
		break;

	DEFAULT();
	}

	return 0;
}

int
set_addon_filename(build_image_context	*context,
		char	*filename,
		int index)
{

	struct addon_item_rec **current;
	int i;

	current = &(context->addon_tbl.addon_item_list);

	for(i = 0; i <= index; i++) {
		if (*current == NULL) {
			(*current) = malloc(sizeof(struct addon_item_rec));
			if (*current == NULL)
				return -ENOMEM;
			memset((*current), 0, sizeof(struct addon_item_rec));
			memcpy((*current)->addon_filename,
				filename, MAX_BUFFER);
			(*current)->item_index = index;
			(*current)->next = NULL;
			context->addon_tbl.addon_item_no++;
		} else if ((*current)->item_index == index) {
			memcpy((*current)->addon_filename,
				filename, MAX_BUFFER);
		} else
			current = &((*current)->next);
	}
	return 0;
}

int set_addon_attr(build_image_context	*context,
		u_int32_t file_attr,
		int index)
{
	struct addon_item_rec **current;
	int i;

	current = &(context->addon_tbl.addon_item_list);

	for(i = 0; i <= index; i++) {
		if (*current == NULL) {
			(*current) = malloc(sizeof(struct addon_item_rec));
			if (*current == NULL)
				return -ENOMEM;
			memset((*current), 0, sizeof(struct addon_item_rec));
			(*current)->item.attribute= file_attr;
			(*current)->item_index = index;
			(*current)->next = NULL;
			context->addon_tbl.addon_item_no++;
		} else if ((*current)->item_index == index) {
			(*current)->item.attribute= file_attr;
		} else
			current = &((*current)->next);
	}
	return 0;
}

int set_unique_name(build_image_context *context, char *uname,	int index)
{
	struct addon_item_rec **current;
	int i;

	current = &(context->addon_tbl.addon_item_list);

	for(i = 0; i <= index; i++) {
		if (*current == NULL) {
			(*current) = malloc(sizeof(struct addon_item_rec));
			if (*current == NULL)
				return -ENOMEM;
			memset((*current), 0, sizeof(struct addon_item_rec));
			memcpy((*current)->item.unique_name, uname, 4);
			(*current)->item_index = index;
			(*current)->next = NULL;
			context->addon_tbl.addon_item_no++;
		} else if ((*current)->item_index == index) {
			memcpy((*current)->item.unique_name, uname, 4);
		} else
			current = &((*current)->next);
	}
	return 0;
}

int
set_other_field(build_image_context	*context,
		char	*other_str,
		int	other,
		int	index)
{
	struct addon_item_rec **current;
	int i;

	current = &(context->addon_tbl.addon_item_list);

	for(i = 0; i <= index; i++) {
		if (*current == NULL) {
			(*current) = malloc(sizeof(struct addon_item_rec));
			if (*current == NULL)
				return -ENOMEM;
			memset((*current), 0, sizeof(struct addon_item_rec));
			if (other_str == NULL)
				(*current)->item.reserve[0] = other;
			else
				memcpy((*current)->item.reserve,
					other_str, 16);
			(*current)->item_index = index;
			(*current)->next = NULL;
			context->addon_tbl.addon_item_no++;
		} else if ((*current)->item_index == index) {
			if (other_str == NULL)
				(*current)->item.reserve[0] = other;
			else
				memcpy((*current)->item.reserve,
					other_str, 16);
		} else
			current = &((*current)->next);
	}
	return 0;

}

