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

struct mrc_metadata {
	uint32_t signature;
	uint32_t data_size;
	uint16_t data_checksum;
	uint16_t header_checksum;
	uint32_t version;
} __attribute__((packed));

#define MRC_DATA_SIGNATURE	(('M'<<0)|('R'<<8)|('C'<<16)|('D'<<24))
#define REGF_BLOCK_SHIFT	4
#define REGF_UNALLOCATED_BLOCK	0xffff


unsigned long compute_ip_checksum(const void *addr, unsigned long length)
{
	const uint8_t *ptr;
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

static int verify_mrc_slot(struct mrc_metadata *md, unsigned long slot_len)
{
	uint32_t header_checksum;
	if (md->signature != MRC_DATA_SIGNATURE) {
		fprintf(stderr, "MRC signature mismatch\n");
		return 1;
	}

	fprintf(stderr, "MRC signature match.. successful\n");

	if (md->data_size > slot_len) {
		fprintf(stderr, "MRC cache size overflow\n");
		return 1;
	}

	header_checksum = md->header_checksum;
	md->header_checksum = 0;

	if (header_checksum != compute_ip_checksum(md, sizeof(*md))) {
		fprintf(stderr, "MRC metadata header checksum mismatch\n");
		return 1;
	}

	md->header_checksum = header_checksum;

	fprintf(stderr, "MRC metadata header checksum.. verified!\n");

	if (md->data_checksum != compute_ip_checksum(&md[1], md->data_size)) {
		fprintf(stderr, "MRC data checksum mismatch\n");
		return 1;
	}

	fprintf(stderr, "MRC data checksum.. verified!\n");
	return 0;
}

static int get_mrc_data_slot(uint16_t *mb, uint32_t *data_offset,
			     uint32_t *data_size)
{
	/*
	 * First block offset in metadata block tells the total number of
	 * metadata blocks.
	 * Currently, we expect only 1 metadata block to be present.
	 */
	if (*mb != 1) {
		fprintf(stderr, "Only 1 metadata block is expected. "
			"Actual %x\n", *mb);
		return 1;
	}

	/*
	 * RECOVERY_MRC_CACHE is expected to contain only one slot. Thus, there
	 * should be only one block offset present, indicating size of the MRC
	 * cache slot.
	 */
	mb++;
	*data_offset = (1 << REGF_BLOCK_SHIFT);
	*data_size = (*mb - 1) << REGF_BLOCK_SHIFT;

	mb++;
	if (*mb != REGF_UNALLOCATED_BLOCK) {
		fprintf(stderr, "More than 1 slot in recovery mrc cache.\n");
		return 1;
	}

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
	uint32_t data_offset;
	uint32_t data_size;
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

	if (get_mrc_data_slot((uint16_t *)(buff + offset), &data_offset,
			      &data_size)) {
		fprintf(stderr, "Metadata block error\n");
		futil_unmap_file(fd, MAP_RO, buff, file_size);
		close(fd);
		return 1;
	}
	offset += data_offset;

	if ((file_size > offset) && ((file_size - offset) >= data_size))
		ret = verify_mrc_slot((struct mrc_metadata *)(buff + offset),
				      data_size);
	else
		fprintf(stderr, "Offset or data size greater than file size: "
			"offset=0x%x, file size=0x%x, data_size=0x%x\n",
			offset, file_size, data_size);

	if (futil_unmap_file(fd, MAP_RO, buff, file_size) != FILE_ERR_NONE) {
		fprintf(stderr, "Failed to unmap file %s\n", infile);
		ret = 1;
	}

	close(fd);
	return ret;
}

DECLARE_FUTIL_COMMAND(validate_rec_mrc, do_validate_rec_mrc, VBOOT_VERSION_ALL,
		      "Validates content of Recovery MRC cache");
