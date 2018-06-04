/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Create a BDB
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bdb.h"
#include "host.h"

/* Parameters for creating a BDB hash entry */
struct create_hash {
	/* File containing data */
	const char *filename;

	/* Type of data; enum bdb_data_type */
	uint8_t type;

	/* Address in RAM to load data.  -1 means use default. */
	uint64_t load_address;

	/* Partition number containing data, or -1 to use the same partition as
	 * the BDB. */
	uint8_t partition;

	/*
	 * Offset of data from start of partition.
	 *
	 * TODO: if -1, append after BDB.  But need to know how big the BDB
	 * is, and need to round up offset to 32-bit boundary.
	 */
	 uint64_t offset;
};

/* Parameters for a key */
struct create_key {
	/* Description */
	const char *description;

	/* Key version (not meaningful for BDB key) */
	uint32_t key_version;

	/* Public key filename (.keyb) */
	const char *public_filename;

	/* Private key filename (.pem) */
	const char *private_filename;
};

struct create_params_2 {
	/* Destination filename */
	const char *filename;

	/* Partition to contain the BDB */
	uint8_t partition;

	/* OEM area files.  NULL means there is no data for that area. */
	const char *oem_area_0_filename;
	const char *oem_area_1_filename;

	/* BDB key and subkey */
	struct create_key bdbkey;
	struct create_key subkey;
};

/*****************************************************************************/
/* FILL THIS IN WITH YOUR SOURCE DATA */

/*
 * Creation parameters.  Hash and num_hashes will be filled in automatically
 * by create().
 */
struct bdb_create_params p = {
	.bdb_load_address = 0x11223344,
	.header_sig_description = "The header sig",
	.data_sig_description = "The data sig",
	.data_description = "Test BDB data",
	.data_version = 3,
};

/* Additional parameters */
struct create_params_2 p2 = {
	.filename = "build/bdb.bin",
	.partition = 1,
	.oem_area_0_filename = "testdata/oem0.bin",
	.oem_area_1_filename = "testdata/oem1.bin",
	.bdbkey = {
		.description = "Test BDB key",
		.key_version = 3,
		.public_filename = "testkeys/bdbkey.keyb",
		.private_filename = "testkeys/bdbkey.pem",
	},
	.subkey = {
		.description = "Test Subkey",
		.key_version = 4,
		.public_filename = "testkeys/subkey.keyb",
		.private_filename = "testkeys/subkey.pem",
	},
};

/* List of hash entries, terminated by one with a NULL filename */
struct create_hash hash_entries[] = {
	{
		.filename = "testdata/sp-rw.bin",
		.type = BDB_DATA_SP_RW,
		.load_address = -1,
		.partition = -1,
		.offset = 0x10000,
	},
	{
		.filename = "testdata/ap-rw.bin",
		.type = BDB_DATA_AP_RW,
		.load_address = 0x200000,
		.partition = -1,
		.offset = 0x28000,
	},
	{
		.filename = NULL
	},
};

/*****************************************************************************/

int create(void)
{
	struct bdb_hash *hash;
	struct bdb_header *h;
	int i;

	/* Count the number of hash entries */
	for (p.num_hashes = 0; hash_entries[p.num_hashes].filename;
	     p.num_hashes++)
		;
	printf("Found %d hash entries\n", p.num_hashes);

	/* Calculate hashes */
	p.hash = hash = calloc(sizeof(struct bdb_hash), p.num_hashes);
	for (i = 0; i < p.num_hashes; i++, hash++) {
		const struct create_hash *he = hash_entries + i;

		/* Read file and calculate size and hash */
		uint8_t *buf = read_file(he->filename, &hash->size);
		if (!buf)
			return 1;
		if (bdb_sha256(hash->digest, buf, hash->size)) {
			fprintf(stderr, "Unable to calculate hash\n");
			return 1;
		}
		free(buf);

		hash->type = he->type;
		hash->load_address = he->load_address;

		hash->partition = he->partition == -1 ? p2.partition :
			he->partition;

		hash->offset = he->offset;
	}

	/* Read OEM data */
	if (p2.oem_area_0_filename) {
		p.oem_area_0 = read_file(p2.oem_area_0_filename,
					 &p.oem_area_0_size);
		if (!p.oem_area_0)
			return 1;

		if (p.oem_area_0_size & 3) {
			fprintf(stderr,
				"OEM area 0 size isn't 32-bit aligned\n");
			return 1;
		}
	}

	if (p2.oem_area_1_filename) {
		p.oem_area_1 = read_file(p2.oem_area_1_filename,
					 &p.oem_area_1_size);
		if (!p.oem_area_1)
			return 1;

		if (p.oem_area_1_size & 3) {
			fprintf(stderr,
				"OEM area 1 size isn't 32-bit aligned\n");
			return 1;
		}
	}

	/* Load keys */
	p.bdbkey = bdb_create_key(p2.bdbkey.public_filename,
				  p2.bdbkey.key_version,
				  p2.bdbkey.description);
	p.subkey = bdb_create_key(p2.subkey.public_filename,
				  p2.subkey.key_version,
				  p2.subkey.description);
	p.private_bdbkey = read_pem(p2.bdbkey.private_filename);
	p.private_subkey = read_pem(p2.subkey.private_filename);
	if (!p.bdbkey || !p.subkey || !p.private_bdbkey || !p.private_subkey) {
		fprintf(stderr, "Unable to load keys\n");
		return 1;
	}

	/* Create the BDB */
	h = bdb_create(&p);
	if (!h) {
		fprintf(stderr, "Unable to create BDB\n");
		return 1;
	}

	/* Write it */
	if (write_file(p2.filename, h, h->bdb_size))
		return 1;

	/* Free keys and buffers */
	free(p.bdbkey);
	free(p.subkey);
	RSA_free(p.private_bdbkey);
	RSA_free(p.private_subkey);
	free(h);
	free(p.hash);

	return 0;
}

/*****************************************************************************/

int main(void)
{
	return create();
}
