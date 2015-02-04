/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */

#include <stdio.h>
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

int vb2_str_to_guid(const char *str, struct vb2_guid *guid)
{
	uint32_t time_low;
	uint16_t time_mid;
	uint16_t time_high_and_version;
	unsigned int chunk[11];

	if (!str ||
	    11 != sscanf(str,
			 "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
			 chunk+0,
			 chunk+1,
			 chunk+2,
			 chunk+3,
			 chunk+4,
			 chunk+5,
			 chunk+6,
			 chunk+7,
			 chunk+8,
			 chunk+9,
			 chunk+10)) {
		return VB2_ERROR_STR_TO_GUID;
	}

	time_low = chunk[0] & 0xffffffff;
	time_mid = chunk[1] & 0xffff;
	time_high_and_version = chunk[2] & 0xffff;

	guid->uuid.time_low = htole32(time_low);
	guid->uuid.time_mid = htole16(time_mid);
	guid->uuid.time_high_and_version = htole16(time_high_and_version);

	guid->uuid.clock_seq_high_and_reserved = chunk[3] & 0xff;
	guid->uuid.clock_seq_low = chunk[4] & 0xff;
	guid->uuid.node[0] = chunk[5] & 0xff;
	guid->uuid.node[1] = chunk[6] & 0xff;
	guid->uuid.node[2] = chunk[7] & 0xff;
	guid->uuid.node[3] = chunk[8] & 0xff;
	guid->uuid.node[4] = chunk[9] & 0xff;
	guid->uuid.node[5] = chunk[10] & 0xff;

	return VB2_SUCCESS;
}

int vb2_guid_to_str(const struct vb2_guid *guid,
		    char *buf, unsigned int buflen)
{
	int n;

	if (!buf || buflen < VB2_GUID_MIN_STRLEN)
		return VB2_ERROR_GUID_TO_STR;

	n = snprintf(buf, buflen,
		     "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		     le32toh(guid->uuid.time_low),
		     le16toh(guid->uuid.time_mid),
		     le16toh(guid->uuid.time_high_and_version),
		     guid->uuid.clock_seq_high_and_reserved,
		     guid->uuid.clock_seq_low,
		     guid->uuid.node[0], guid->uuid.node[1],
		     guid->uuid.node[2], guid->uuid.node[3],
		     guid->uuid.node[4], guid->uuid.node[5]);

	if (n != VB2_GUID_MIN_STRLEN - 1)
		return VB2_ERROR_GUID_TO_STR;

	return VB2_SUCCESS;
}
