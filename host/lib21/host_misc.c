/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "2sha.h"
#include "vb2_common.h"
#include "host_common.h"
#include "host_misc2.h"

int vb2_read_file(const char *filename, uint8_t **data_ptr, uint32_t *size_ptr)
{
	FILE *f;
	uint8_t *buf;
	long size;

	*data_ptr = NULL;
	*size_ptr = 0;

	f = fopen(filename, "rb");
	if (!f) {
		VB2_DEBUG("Unable to open file %s\n", filename);
		return VB2_ERROR_READ_FILE_OPEN;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);

	if (size < 0 || size > UINT32_MAX) {
		fclose(f);
		return VB2_ERROR_READ_FILE_SIZE;
	}

	buf = malloc(size);
	if (!buf) {
		fclose(f);
		return VB2_ERROR_READ_FILE_ALLOC;
	}

	if(1 != fread(buf, size, 1, f)) {
		VB2_DEBUG("Unable to read file %s\n", filename);
		fclose(f);
		free(buf);
		return VB2_ERROR_READ_FILE_DATA;
	}

	fclose(f);

	*data_ptr = buf;
	*size_ptr = size;
	return VB2_SUCCESS;
}

int vb2_write_file(const char *filename, const void *buf, uint32_t size)
{
	FILE *f = fopen(filename, "wb");

	if (!f) {
		VB2_DEBUG("Unable to open file %s\n", filename);
		return VB2_ERROR_WRITE_FILE_OPEN;
	}

	if (1 != fwrite(buf, size, 1, f)) {
		VB2_DEBUG("Unable to write to file %s\n", filename);
		fclose(f);
		unlink(filename);  /* Delete any partial file */
		return VB2_ERROR_WRITE_FILE_DATA;
	}

	fclose(f);
	return VB2_SUCCESS;
}

int vb2_write_object(const char *filename, const void *buf)
{
	const struct vb2_struct_common *cptr = buf;

	return vb2_write_file(filename, buf, cptr->total_size);
}

uint32_t vb2_desc_size(const char *desc)
{
	if (!desc || !*desc)
		return 0;

	return roundup32(strlen(desc) + 1);
}

static const char *onedigit(const char *str, uint8_t *vptr)
{
	uint8_t val = 0;
	char c;

	for (; (c = *str++) && !isxdigit(c);)
		;
	if (!c)
		return 0;

	if (c >= '0' && c <= '9')
		val = c - '0';
	else if (c >= 'A' && c <= 'F')
		val = 10 + c - 'A';
	else if (c >= 'a' && c <= 'f')
		val = 10 + c - 'a';

	*vptr = val;
	return str;
}

static const char *onebyte(const char *str, uint8_t *vptr)
{
	uint8_t val;
	uint8_t digit;

	str = onedigit(str, &digit);
	if (!str)
		return 0;
	val = digit << 4;

	str = onedigit(str, &digit);
	if (!str)
		return 0;
	val |= digit;

	*vptr = val;
	return str;
}

int vb2_str_to_id(const char *str, struct vb2_id *id)
{
	uint8_t val = 0;
	int i;

	if (!str)
		return VB2_ERROR_STR_TO_ID;

	memset(id, 0, sizeof(*id));

	for (i = 0; i < VB2_ID_NUM_BYTES; i++) {

		str = onebyte(str, &val);
		if (!str)
			break;
		id->raw[i] = val;
	}

	/* If we get at least one valid byte, that's good enough. */
	return i ? VB2_SUCCESS : VB2_ERROR_STR_TO_ID;
}
