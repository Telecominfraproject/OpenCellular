/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common boot flow utility
 */

#include <getopt.h>
#include <inttypes.h>		/* For PRIu64 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bdb_struct.h"
#include "futility.h"
#include "host.h"

static long int version;

/* Command line options */
enum {
	/* mode options */
	OPT_MODE_NONE,
	OPT_MODE_ADD,
	OPT_MODE_CREATE,
	OPT_MODE_RESIGN,
	OPT_MODE_VERIFY,
	/* file options */
	OPT_BDBKEY_PRI,
	OPT_BDBKEY_PUB,
	OPT_DATAKEY_PRI,
	OPT_DATAKEY_PUB,
	/* key version */
	OPT_BDBKEY_VERSION,
	OPT_DATAKEY_VERSION,
	/* integer options */
	OPT_OFFSET,
	OPT_PARTITION,
	OPT_TYPE,
	OPT_LOAD_ADDRESS,
	OPT_VERSION,
	OPT_HELP,
};

static const struct option long_opts[] = {
	{"add", 1, 0, OPT_MODE_ADD},
	{"create", 1, 0, OPT_MODE_CREATE},
	{"resign", 1, 0, OPT_MODE_RESIGN},
	{"verify", 1, 0, OPT_MODE_VERIFY},
	{"bdbkey_pri", 1, 0, OPT_BDBKEY_PRI},
	{"bdbkey_pub", 1, 0, OPT_BDBKEY_PUB},
	{"datakey_pri", 1, 0, OPT_DATAKEY_PRI},
	{"datakey_pub", 1, 0, OPT_DATAKEY_PUB},
	{"bdbkey_version", 1, 0, OPT_BDBKEY_VERSION},
	{"datakey_version", 1, 0, OPT_DATAKEY_VERSION},
	{"offset", 1, 0, OPT_OFFSET},
	{"partition", 1, 0, OPT_PARTITION},
	{"type", 1, 0, OPT_TYPE},
	{"load_address", 1, 0, OPT_LOAD_ADDRESS},
	{"version", 1, 0, OPT_VERSION},
	{"help", 0, 0, OPT_HELP},
	{NULL, 0, 0, 0}
};

/**
 * Add hash entry to BDB
 *
 * This adds a hash entry to a BDB. It does not change the signature. Hence,
 * the produced BDB needs to be resigned using the resign sub-command.
 */
static int do_add(const char *bdb_filename, const char *data_filename,
		  uint64_t offset, uint8_t partition,
		  uint8_t type, uint64_t load_address)
{
	fprintf(stderr, "'add' command is not implemented\n");
	return -1;
}

/**
 * Create a new BDB
 *
 * This creates a new BDB using a pair of BDB keys and a pair of data keys.
 * A private data key is needed even with no hash entries.
 */
static int do_create(const char *bdb_filename,
		     const char *bdbkey_pri_filename,
		     const char *bdbkey_pub_filename,
		     uint32_t bdbkey_version,
		     const char *datakey_pri_filename,
		     const char *datakey_pub_filename,
		     uint32_t datakey_version)
{
	struct bdb_key *bdbkey;
	struct bdb_key *datakey;
	struct rsa_st *bdbkey_pri;
	struct rsa_st *datakey_pri;
	struct bdb_create_params params;
	struct bdb_header *header;
	int rv = -1;

	/* Check arguments */
	if (!bdb_filename || !bdbkey_pri_filename || !bdbkey_pub_filename
			|| !datakey_pri_filename || !datakey_pub_filename) {
		fprintf(stderr, "Missing filenames\n");
		goto exit;
	}

	/* Load keys */
	bdbkey = bdb_create_key(bdbkey_pub_filename, bdbkey_version, NULL);
	bdbkey_pri = read_pem(bdbkey_pri_filename);
	datakey = bdb_create_key(datakey_pub_filename, datakey_version, NULL);
	datakey_pri = read_pem(datakey_pri_filename);
	if (!bdbkey || !bdbkey_pri || !datakey || !datakey_pri) {
		fprintf(stderr, "Unable to load keys\n");
		goto exit;
	}

	memset(&params, 0, sizeof(params));
	params.bdb_load_address = -1;
	params.bdbkey = bdbkey;
	params.datakey = datakey;
	params.private_bdbkey = bdbkey_pri;
	params.private_datakey = datakey_pri;
	params.num_hashes = 0;

	header = bdb_create(&params);
	if (!header) {
		fprintf(stderr, "Unable to create BDB\n");
		goto exit;
	}

	rv = write_file(bdb_filename, header, header->bdb_size);
	if (rv) {
		fprintf(stderr, "Unable to write BDB\n");
		goto exit;
	}

	fprintf(stderr, "BDB is created successfully\n");

exit:
	/* Free keys and buffers */
	free(bdbkey);
	free(datakey);
	RSA_free(bdbkey_pri);
	RSA_free(datakey_pri);

	return rv;
}

/**
 * Resign a BDB using new keys
 *
 * This resigns data key with a new pair of BDB keys. Optionally, it resigns
 * hash entries with a new pair of data keys if they're provided.
 *
 * @return
 */
static int do_resign(void)
{
	fprintf(stderr, "'resign' command is not implemented\n");
	return -1;
}

static int do_verify(void)
{
	fprintf(stderr, "'verify' command is not implemented\n");
	return -1;
}

/* Print help and return error */
static void print_help(int argc, char *argv[])
{
	printf("\nUsage: " MYNAME " %s <--create|--add|--resign|--verify>\n"
	       "\n"
	       "For '--add <bdb_file> [OPTIONS]', required OPTIONS are:\n"
	       "  --data <file>               Data to be added\n"
	       "  --offset <offset>           Offset\n"
	       "  --partition <number>        Partition number\n"
	       "  --type <number>             Data type\n"
	       "  --load_address <number>     Data load address\n"
	       "\n"
	       "For '--create <bdb_file> [OPTIONS]', required OPTIONS are:\n"
	       "  --bdbkey_pri <file>         BDB key in .pem format\n"
	       "  --bdbkey_pub <file>         BDB key in .keyb format\n"
	       "  --datakey_pri <file>        Data key in .pem format\n"
	       "  --datakey_pub <file>        Data key in .keyb format\n"
	       "\n"
	       "For '--resign <bdb_file> [OPTIONS]', optional OPTIONS are:\n"
	       "  --bdbkey_pri <file>         New BDB key in .pem format\n"
	       "  --bdbkey_pub <file>         New BDB key in .keyb format\n"
	       "  --datakey_pri <file>        New data key in .pem format\n"
	       "  --datakey_pub <file>        New data key in .keyb format\n"
	       "\n"
	       "For '--verify <bdb_file> [OPTIONS]', optional OPTIONS are:\n"
	       "  --key_digest <file>         BDB key digest\n"
	       "\n",
	       argv[0]);
}

static int do_bdb(int argc, char *argv[])
{
	int mode = 0;
	const char *bdb_filename = NULL;
	const char *bdbkey_pri_filename = NULL;
	const char *bdbkey_pub_filename = NULL;
	const char *datakey_pri_filename = NULL;
	const char *datakey_pub_filename = NULL;
	const char *data_filename = NULL;
	uint32_t bdbkey_version = 0;
	uint32_t datakey_version = 0;
	uint64_t offset = 0;
	uint8_t partition = 0;
	uint8_t type = 0;
	uint64_t load_address = -1;
	int parse_error = 0;
	char *e;
	int i;

	while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
		switch (i) {
		case '?':
			/* Unhandled option */
			fprintf(stderr, "Unknown option or missing value\n");
			parse_error = 1;
			break;
		case OPT_HELP:
			print_help(argc, argv);
			return !!parse_error;
		case OPT_MODE_CREATE:
			mode = i;
			bdb_filename = optarg;
			break;
		case OPT_MODE_ADD:
			mode = i;
			bdb_filename = optarg;
			break;
		case OPT_MODE_RESIGN:
			mode = i;
			bdb_filename = optarg;
			break;
		case OPT_BDBKEY_PRI:
			bdbkey_pri_filename = optarg;
			break;
		case OPT_BDBKEY_PUB:
			bdbkey_pub_filename = optarg;
			break;
		case OPT_DATAKEY_PRI:
			datakey_pri_filename = optarg;
			break;
		case OPT_DATAKEY_PUB:
			datakey_pub_filename = optarg;
			break;
		case OPT_BDBKEY_VERSION:
			bdbkey_version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --bdbkey_version\n");
				parse_error = 1;
			}
			break;
		case OPT_DATAKEY_VERSION:
			datakey_version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --datakey_version\n");
				parse_error = 1;
			}
			break;
		case OPT_OFFSET:
			offset = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --offset\n");
				parse_error = 1;
			}
			break;
		case OPT_PARTITION:
			partition = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --partition\n");
				parse_error = 1;
			}
			break;
		case OPT_TYPE:
			type = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --type\n");
				parse_error = 1;
			}
			break;
		case OPT_LOAD_ADDRESS:
			load_address = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --load_address\n");
				parse_error = 1;
			}
			break;
		case OPT_VERSION:
			version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --version\n");
				parse_error = 1;
			}
			break;
		}
	}

	if (parse_error) {
		print_help(argc, argv);
		return 1;
	}

	switch (mode) {
	case OPT_MODE_ADD:
		return do_add(bdb_filename, data_filename,
			      offset, partition, type, load_address);
	case OPT_MODE_CREATE:
		return do_create(bdb_filename, bdbkey_pri_filename,
				 bdbkey_pub_filename, bdbkey_version,
				 datakey_pri_filename, datakey_pub_filename,
				 datakey_version);
	case OPT_MODE_RESIGN:
		return do_resign();
	case OPT_MODE_VERIFY:
		return do_verify();
	case OPT_MODE_NONE:
	default:
		fprintf(stderr, "Must specify a mode.\n");
		print_help(argc, argv);
		return 1;
	}
}

DECLARE_FUTIL_COMMAND(bdb, do_bdb, VBOOT_VERSION_1_0,
		      "Common boot flow utility");
