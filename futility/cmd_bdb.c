/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Common boot flow utility
 */

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2common.h"
#include "bdb.h"
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
	OPT_DATA,
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
	{"data", 1, 0, OPT_DATA},
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
	uint8_t *bdb, *data, *new_bdb;
	uint32_t bdb_size, data_size;
	struct bdb_header *bdb_header;
	struct bdb_data *data_header;
	struct bdb_hash *new_hash;
	int rv = -1;

	bdb = read_file(bdb_filename, &bdb_size);
	data = read_file(data_filename, &data_size);
	if (!bdb || !data) {
		fprintf(stderr, "Unable to load BDB or data\n");
		goto exit;
	}

	/* Create a copy of BDB */
	new_bdb = calloc(1, bdb_size + sizeof(*new_hash));
	if (!new_bdb) {
		fprintf(stderr, "Unable to allocate memory\n");
		goto exit;
	}
	/* Copy up to the end of hashes. This implicitly clears the data
	 * sig because it's not copied. */
	memcpy(new_bdb, bdb, vb2_offset_of(bdb, bdb_get_data_sig(bdb)));

	/* Update new BDB header */
	bdb_header = (struct bdb_header *)bdb_get_header(new_bdb);
	bdb_header->bdb_size += sizeof(*new_hash);

	data_header = (struct bdb_data *)bdb_get_data(new_bdb);

	/* Update new hash. We're overwriting the data signature, which
	 * is already invalid anyway. */
	new_hash = (struct bdb_hash *)((uint8_t *)data_header
			+ data_header->signed_size);
	new_hash->size = data_size;
	new_hash->type = type;
	new_hash->load_address = load_address;
	new_hash->partition = partition;
	new_hash->offset = offset;
	if (bdb_sha256(new_hash->digest, data, data_size)) {
		fprintf(stderr, "Unable to calculate hash\n");
		goto exit;
	}

	/* Update data header */
	data_header->num_hashes++;
	data_header->signed_size += sizeof(*new_hash);

	rv = write_file(bdb_filename, bdb_header, bdb_header->bdb_size);
	if (rv) {
		fprintf(stderr, "Unable to write BDB\n");
		goto exit;
	}

	fprintf(stderr, "Hash is added to BDB successfully. Resign required\n");

exit:
	free(bdb);
	free(data);
	free(new_bdb);

	return rv;
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

static int install_bdbkey(uint8_t **bdb, const struct bdb_key *new_key)
{
	struct bdb_header *header;
	const struct bdb_key *key;
	uint8_t *p, *q;
	uint8_t *new_bdb;
	size_t new_size;
	size_t l;

	header = (struct bdb_header *)bdb_get_header(*bdb);
	key = bdb_get_bdbkey(*bdb);
	new_size = bdb_size_of(*bdb) + new_key->struct_size - key->struct_size;
	new_bdb = calloc(1, new_size);
	if (!new_bdb) {
		fprintf(stderr, "Unable to allocate memory\n");
		return -1;
	}

	/* copy BDB header */
	p = *bdb;
	q = new_bdb;
	l = header->struct_size;
	memcpy(q, p, l);

	/* copy new BDB key */
	p += l;
	q += l;
	memcpy(q, new_key, new_key->struct_size);

	/* copy the rest */
	p += key->struct_size;
	q += new_key->struct_size;
	l = bdb_size_of(*bdb) - vb2_offset_of(*bdb, p);
	memcpy(q, p, l);

	/* update size */
	header = (struct bdb_header *)bdb_get_header(new_bdb);
	header->bdb_size = new_size;

	free(*bdb);
	*bdb = new_bdb;

	return 0;
}

static int install_datakey(uint8_t **bdb, const struct bdb_key *new_key)
{
	struct bdb_header *header;
	struct bdb_key *key;
	uint8_t *p, *q;
	uint8_t *new_bdb;
	size_t new_size;
	uint32_t l;

	key = (struct bdb_key *)bdb_get_datakey(*bdb);
	new_size = bdb_size_of(*bdb) + new_key->struct_size - key->struct_size;
	new_bdb = calloc(1, new_size);
	if (!new_bdb) {
		fprintf(stderr, "Unable to allocate memory\n");
		return -1;
	}

	/* copy the stuff up to datakey */
	p = *bdb;
	q = new_bdb;
	l = bdb_offset_of_datakey(*bdb);
	memcpy(q, p, l);

	/* copy new data key */
	p += l;
	q += l;
	memcpy(q, new_key, new_key->struct_size);

	/* copy the rest */
	p += key->struct_size;
	q += new_key->struct_size;
	l = bdb_size_of(*bdb) - vb2_offset_of(*bdb, p);
	memcpy(q, p, l);

	/* update size */
	header = (struct bdb_header *)bdb_get_header(new_bdb);
	header->bdb_size = new_size;
	header->signed_size = header->oem_area_0_size + new_key->struct_size;

	free(*bdb);
	*bdb = new_bdb;

	return 0;
}
/**
 * Resign a BDB using new keys
 *
 * It first installs given public keys to the BDB, then, runs verification.
 * If verification fails due to an invalid signature, it tries to 'fix' it
 * by resigning it using a given private key, then runs verification again.
 * Whether a key is required or not depends on which signature is invalid.
 * If a private key is required but not provided, it returns an error.
 */
static int do_resign(const char *bdb_filename,
		     const char *bdbkey_pri_filename,
		     const char *bdbkey_pub_filename,
		     uint32_t bdbkey_version,
		     const char *datakey_pri_filename,
		     const char *datakey_pub_filename,
		     uint32_t datakey_version)
{
	uint8_t *bdb = NULL;
	struct rsa_st *bdbkey_pri = NULL;
	struct rsa_st *datakey_pri = NULL;
	uint32_t bdb_size;
	int resigned = 0;
	int rv = -1;

	if (!bdb_filename) {
		fprintf(stderr, "BDB file must be specified\n");
		goto exit;
	}

	bdb = read_file(bdb_filename, &bdb_size);
	if (!bdb) {
		fprintf(stderr, "Unable to read %s\n", bdb_filename);
		goto exit;
	}

	if (bdbkey_pub_filename) {
		struct bdb_key *key = bdb_create_key(bdbkey_pub_filename,
						     bdbkey_version, NULL);
		if (!key) {
			fprintf(stderr, "Unable to read BDB key\n");
			goto exit;
		}
		if (install_bdbkey(&bdb, key)) {
			fprintf(stderr, "Unable to install new BDB key\n");
			goto exit;
		}
	}

	if (datakey_pub_filename) {
		struct bdb_key *key = bdb_create_key(datakey_pub_filename,
						     datakey_version, NULL);
		if (!key) {
			fprintf(stderr, "Unable to read data key\n");
			goto exit;
		}
		if (install_datakey(&bdb, key)) {
			fprintf(stderr, "Unable to install new data key\n");
			goto exit;
		}
	}

	/* Check validity for the new bdb key */
	rv = bdb_verify(bdb, bdb_size_of(bdb), NULL);
	if (rv == BDB_ERROR_HEADER_SIG) {
		/* This is expected failure if we installed a new BDB key.
		 * Let's resign to fix it. */
		resigned = 1;
		fprintf(stderr, "Data key signature is invalid. Need to resign "
			"the key.\n");
		if (!bdbkey_pri_filename) {
			fprintf(stderr, "Private BDB key is required but not "
				"provided.\n");
			goto exit;
		}
		bdbkey_pri = read_pem(bdbkey_pri_filename);
		rv = bdb_sign_datakey(&bdb, bdbkey_pri);
		if (rv) {
			fprintf(stderr, "Failed to resign data key: %d\n", rv);
			goto exit;
		}
		fprintf(stderr, "Data key is resigned.\n");
	} else {
		fprintf(stderr, "Resigning data key is not required.\n");
	}

	/* Check validity for the new data key */
	rv = bdb_verify(bdb, bdb_size_of(bdb), NULL);
	switch (rv) {
	case BDB_ERROR_DATA_SIG:
	case BDB_ERROR_DATA_CHECK_SIG:
		/* This is expected failure if we installed a new data key
		 * or sig is corrupted, which happens when a new hash is added
		 * by 'add' sub-command. Let's resign the data */
		resigned = 1;
		fprintf(stderr,
			"Data signature is invalid. Need to resign data.\n");
		if (!datakey_pri_filename) {
			fprintf(stderr, "Private data key is required but not "
				"provided.\n");
			goto exit;
		}
			datakey_pri = read_pem(datakey_pri_filename);
			rv = bdb_sign_data(&bdb, datakey_pri);
			if (rv) {
			fprintf(stderr, "Failed to resign hashes: %d\n", rv);
				goto exit;
			}
		fprintf(stderr, "Data is resigned.\n");
		break;
	case BDB_GOOD_OTHER_THAN_KEY:
	case BDB_SUCCESS:
		fprintf(stderr, "Resigning the data is not required.\n");
		break;
	default:
		fprintf(stderr, "Verifying BDB failed unexpectedly: %d\n", rv);
			goto exit;
	}

	if (!resigned)
		goto exit;

	/* Check validity one last time */
	rv = bdb_verify(bdb, bdb_size_of(bdb), NULL);
	if (rv && rv != BDB_GOOD_OTHER_THAN_KEY) {
		/* This is not expected. We installed new keys and resigned
		 * BDB but it's still invalid. */
		fprintf(stderr, "BDB is resigned but it's invalid: %d\n", rv);
		goto exit;
	}

	rv = write_file(bdb_filename, bdb, bdb_size_of(bdb));
	if (rv) {
		fprintf(stderr, "Unable to write BDB.\n");
		goto exit;
	}

	fprintf(stderr, "Successfully resigned BDB.\n");

exit:
	free(bdb);
	RSA_free(bdbkey_pri);
	RSA_free(datakey_pri);

	return rv;
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
		case OPT_DATA:
			data_filename = optarg;
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
		return do_resign(bdb_filename, bdbkey_pri_filename,
				 bdbkey_pub_filename, bdbkey_version,
				 datakey_pri_filename, datakey_pub_filename,
				 datakey_version);
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
