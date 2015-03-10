/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "file_type.h"
#include "futility.h"
#include "gbb_header.h"

/* Human-readable strings */
static const struct {
	const char * const name;
	const char * const desc;
} type_strings[] = {
#define FILE_TYPE(A, B, C) {B, C},
#include "file_type.inc"
#undef FILE_TYPE
};

const char * const futil_file_type_name(enum futil_file_type type)
{
	if ((int) type < 0 || type >= NUM_FILE_TYPES)
		type = FILE_TYPE_UNKNOWN;

	return type_strings[type].name;
}

const char * const futil_file_type_desc(enum futil_file_type type)
{
	if ((int) type < 0 || type >= NUM_FILE_TYPES)
		type = FILE_TYPE_UNKNOWN;

	return type_strings[type].desc;
}

/* Name to enum. Returns true on success. */
int futil_file_str_to_type(const char *str, enum futil_file_type *tptr)
{
	int i;
	for (i = 0; i < NUM_FILE_TYPES; i++)
		if (!strcasecmp(str, type_strings[i].name)) {
			if (tptr)
				*tptr = i;
			return 1;
		}

	if (tptr)
		*tptr = FILE_TYPE_UNKNOWN;
	return 0;
}

/* Print the list of type names and exit with the given value. */
void print_file_types_and_exit(int retval)
{
	int i;
	printf("\nValid file types are:\n\n");
	for (i = 0; i < NUM_FILE_TYPES; i++)
		printf("  %-20s%s\n", type_strings[i].name,
		       type_strings[i].desc);
	printf("\n");

	exit(retval);
}

/* Try these in order so we recognize the larger objects first */
enum futil_file_type (*recognizers[])(uint8_t *buf, uint32_t len) = {
	&recognize_gpt,
	&recognize_bios_image,
	&recognize_gbb,
	&recognize_vblock1,
	&recognize_vb1_key,
	&recognize_vb2_key,
	&recognize_pem,
};

int futil_str_to_file_type(const char *str, enum futil_file_type *type)
{
	int i;
	for (i = 0; i < NUM_FILE_TYPES; i++)
		if (!strcasecmp(str, type_strings[i].name))
			break;
	if (i < NUM_FILE_TYPES) {
		*type = i;
		return 1;
	}

	return 0;
}


/* Try to figure out what we're looking at */
enum futil_file_type futil_file_type_buf(uint8_t *buf, uint32_t len)
{
	enum futil_file_type type;
	int i;

	for (i = 0; i < ARRAY_SIZE(recognizers); i++) {
		type = recognizers[i](buf, len);
		if (type != FILE_TYPE_UNKNOWN)
			return type;
	}

	return FILE_TYPE_UNKNOWN;
}

enum futil_file_err futil_file_type(const char *filename,
				    enum futil_file_type *type)
{
	int ifd;
	uint8_t *buf;
	uint32_t buf_len;
	struct stat sb;
	enum futil_file_err err = FILE_ERR_NONE;

	*type = FILE_TYPE_UNKNOWN;

	ifd = open(filename, O_RDONLY);
	if (ifd < 0) {
		fprintf(stderr, "Can't open %s: %s\n",
			filename, strerror(errno));
		return FILE_ERR_OPEN;
	}

	if (0 != fstat(ifd, &sb)) {
		fprintf(stderr, "Can't stat input file: %s\n",
			strerror(errno));
		return FILE_ERR_STAT;
	}

	if (S_ISREG(sb.st_mode) || S_ISBLK(sb.st_mode)) {
		err = futil_map_file(ifd, MAP_RO, &buf, &buf_len);
		if (err) {
			close(ifd);
			return err;
		}

		*type = futil_file_type_buf(buf, buf_len);

		err = futil_unmap_file(ifd, MAP_RO, buf, buf_len);
		if (err) {
			close(ifd);
			return err;
		}
	} else if (S_ISDIR(sb.st_mode)) {
		err = FILE_ERR_DIR;
	} else if (S_ISCHR(sb.st_mode)) {
		err = FILE_ERR_CHR;
	} else if (S_ISFIFO(sb.st_mode)) {
		err = FILE_ERR_FIFO;
	} else if (S_ISSOCK(sb.st_mode)) {
		err = FILE_ERR_SOCK;
	}

	if (close(ifd)) {
		fprintf(stderr, "Error when closing %s: %s\n",
			filename, strerror(errno));
		return FILE_ERR_CLOSE;
	}

	return err;
}
