/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "futility.h"

enum {
	OPT_HELP = 1000,
	OPT_OFFSET,
};

static const struct option long_opts[] = {
	{"help", 0, 0, OPT_HELP},
	{"offset", 1, 0, OPT_OFFSET},
	{NULL, 0, NULL, 0},
};

static void print_help(int argc, char *argv[])
{
	printf("\nUsage:  " MYNAME " %s FILE [OPTIONS]\n", argv[0]);
	printf("\nOptions:\n");
	printf(" --offset <offset>          Offset of cache within FILE\n");
	printf("\n");
}

struct mrc_cache {
	uint32_t signature;
	uint32_t size;
	uint32_t checksum;
	uint32_t version;
	uint8_t  data[0];
} __attribute__((packed));

#define MRC_DATA_SIGNATURE	(('M'<<0)|('R'<<8)|('C'<<16)|('D'<<24))

unsigned long compute_ip_checksum(void *addr, unsigned long length)
{
	uint8_t *ptr;
	volatile union {
		uint8_t  byte[2];
		uint16_t word;
	} value;
	unsigned long sum;
	unsigned long i;
	/* In the most straight forward way possible,
	 * compute an ip style checksum.
	 */
	sum = 0;
	ptr = addr;
	for(i = 0; i < length; i++) {
		unsigned long v;
		v = ptr[i];
		if (i & 1) {
			v <<= 8;
		}
		/* Add the new value */
		sum += v;
		/* Wrap around the carry */
		if (sum > 0xFFFF) {
			sum = (sum + (sum >> 16)) & 0xFFFF;
		}
	}
	value.byte[0] = sum & 0xff;
	value.byte[1] = (sum >> 8) & 0xff;
	return (~value.word) & 0xFFFF;
}

static int verify_checksum(struct mrc_cache *cache, unsigned long cache_len)
{
	uint32_t checksum;
	if (cache->signature != MRC_DATA_SIGNATURE) {
		fprintf(stderr, "MRC signature mismatch\n");
		return 1;
	}

	fprintf(stderr, "MRC signature match.. successful\n");

	if (cache->size > cache_len) {
		fprintf(stderr, "MRC cache size overflow\n");
		return 1;
	}

	checksum = compute_ip_checksum((void *)&cache->data[0], cache->size);

	if (checksum != cache->checksum) {
		fprintf(stderr, "MRC checksum mismatch\n");
		return 1;
	}

	fprintf(stderr, "MRC checksum.. verified!\n");
	return 0;
}

static int do_validate_rec_mrc(int argc, char *argv[])
{
	char *infile = NULL;
	int parse_error = 0;
	int fd, i, ret = 1;
	uint32_t file_size;
	uint8_t *buff;
	uint32_t offset = 0;
	char *e;

	while (((i = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) &&
	       !parse_error) {
		switch (i) {
		case OPT_HELP:
			print_help(argc, argv);
			return 0;
		case OPT_OFFSET:
			offset = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --offset\n");
				parse_error = 1;
			}
			break;
		default:
		case '?':
			parse_error = 1;
			break;
		}
	}

	if (parse_error) {
		print_help(argc, argv);
		return 1;
	}

	if ((argc - optind) < 1) {
		fprintf(stderr, "You must specify an input FILE!\n");
		print_help(argc, argv);
		return 1;
	} else if ((argc - optind) != 1) {
		fprintf(stderr, "Unexpected arguments!\n");
		print_help(argc, argv);
		return 1;
	}

	infile = argv[optind++];

	fd = open(infile, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s:%s\n", infile, strerror(errno));
		return 1;
	}

	if (futil_map_file(fd, MAP_RO, &buff, &file_size) != FILE_ERR_NONE) {
		fprintf(stderr, "Cannot map file %s\n", infile);
		close(fd);
		return 1;
	}

	if (file_size > offset)
		ret = verify_checksum((struct mrc_cache *)(buff + offset),
				      file_size - offset);
	else
		fprintf(stderr, "Offset greater than file size: offset=0x%x, "
			"file size=0x%x\n", offset, file_size);

	if (futil_unmap_file(fd, MAP_RO, buff, file_size) != FILE_ERR_NONE) {
		fprintf(stderr, "Failed to unmap file %s\n", infile);
		ret = 1;
	}

	close(fd);
	return ret;
}

DECLARE_FUTIL_COMMAND(validate_rec_mrc, do_validate_rec_mrc, VBOOT_VERSION_ALL,
		      "Validates content of Recovery MRC cache");
