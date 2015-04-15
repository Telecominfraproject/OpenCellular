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

/* Description and functions to handle each file type */
struct futil_file_type_s {
	/* Short name for this type */
	const char *name;
	/* Human-readable description */
	const char *desc;
	/* Functions to identify, display, and sign this type of file. */
	enum futil_file_type (*recognize)(uint8_t *buf, uint32_t len);
	int (*show)(const char *name, uint8_t *buf, uint32_t len, void *data);
	int (*sign)(const char *name, uint8_t *buf, uint32_t len, void *data);
};

/* Populate a list of file types and operator functions. */
static const struct futil_file_type_s futil_file_types[] = {
	{"unknown",       "not something we know about", 0, 0, 0},
#define R_(x) x
#define S_(x) x
#define NONE 0
#define FILE_TYPE(A, B, C, D, E, F) {B, C, D, E, F},
#include "file_type.inc"
#undef FILE_TYPE
#undef NONE
#undef S_
#undef R_
};

const char * const futil_file_type_name(enum futil_file_type type)
{
	return futil_file_types[type].name;
}

const char * const futil_file_type_desc(enum futil_file_type type)
{
	return futil_file_types[type].desc;
}

/* Name to enum. Returns true on success. */
int futil_str_to_file_type(const char *str, enum futil_file_type *type)
{
	int i;
	for (i = 0; i < NUM_FILE_TYPES; i++)
		if (!strcasecmp(str, futil_file_types[i].name)) {
			*type = i;
			return 1;
		}

	*type = FILE_TYPE_UNKNOWN;
	return 0;
}

/* Print the list of type names and exit with the given value. */
void print_file_types_and_exit(int retval)
{
	int i;
	printf("\nValid file types are:\n\n");
	for (i = 0; i < NUM_FILE_TYPES; i++)
		printf("  %-20s%s\n", futil_file_types[i].name,
		       futil_file_types[i].desc);
	printf("\n");

	exit(retval);
}

/* Try to figure out what we're looking at */
enum futil_file_type futil_file_type_buf(uint8_t *buf, uint32_t len)
{
	enum futil_file_type type;
	int i;

	for (i = 0; i < NUM_FILE_TYPES; i++) {
		if (futil_file_types[i].recognize) {
			type = futil_file_types[i].recognize(buf, len);
			if (type != FILE_TYPE_UNKNOWN)
				return type;
		}
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

int futil_file_type_show(enum futil_file_type type,
			 const char *filename,
			 uint8_t *buf, uint32_t len)
{
	if (futil_file_types[type].show)
		return futil_file_types[type].show(filename, buf, len, 0);

	fprintf(stderr, "Don't know how to show %s (type %s)\n",
		filename, futil_file_type_name(type));
	return 1;
}

int futil_file_type_sign(enum futil_file_type type,
			 const char *filename,
			 uint8_t *buf, uint32_t len)
{
	if (futil_file_types[type].sign)
		return futil_file_types[type].sign(filename, buf, len, 0);

	fprintf(stderr, "Don't know how to sign %s (type %s)\n",
		filename, futil_file_type_name(type));
	return 1;
}

