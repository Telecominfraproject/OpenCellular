/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "2sha.h"
#include "bdb.h"
#include "bdb_api.h"
#include "host.h"

static f_extend extend = vb2_sha256_extend;

static void help(void)
{
	fprintf(stderr,
		"Usage: bdb_extend -b bdb_file -s bds_file "
		"[-d digest_file] [-m]\n"
		"\n"
		"Extends BDS based on a given BDB. When '-m' is given, a "
		"MVMAP2315's sha256_extend algorithm will be used. When "
		"digest_file is specified, the validity of the BDB key is "
		"checked and the secrets will be derived differently.\n");
}

#define PACK32(str, x)						\
	{							\
		*(x) =   ((uint32_t) *((str) + 3)      )	\
			| ((uint32_t) *((str) + 2) <<  8)       \
			| ((uint32_t) *((str) + 1) << 16)       \
			| ((uint32_t) *((str) + 0) << 24);      \
	}

/**
 * MVMAP2315's implementation of sha256 extend
 *
 * This performs incorrect but still cryptographically secure sha256 extension.
 * This is provided for test purpose only.
 *
 * See vb2_sha256_extend for details on arguments.
 */
static void mvmap2315_sha256_extend(const uint8_t *from, const uint8_t *by,
				    uint8_t *to)
{
	struct vb2_sha256_context dc;
	int i;

	vb2_sha256_init(&dc);
	for (i = 0; i < 8; i++) {
		 PACK32(from, &dc.h[i]);
		 from += 4;
	}
	vb2_sha256_update(&dc, by, VB2_SHA256_BLOCK_SIZE);
	vb2_sha256_finalize(&dc, to);
}

static void dump_secret(const uint8_t *secret, const char *label)
{
	int i;
	printf("%s = {", label);
	for (i = 0; i < BDB_SECRET_SIZE; i++) {
		if (i % 8 == 0)
			printf("\n\t");
		else
			printf(" ");
		printf("0x%02x,", secret[i]);
	}
	printf("\n}\n");
}

static void dump_secrets(struct vba_context *ctx, const uint8_t *wsr)
{
	dump_secret(ctx->secrets->bdb, "bdb");
	dump_secret(ctx->secrets->boot_path, "boot_path");
	dump_secret(ctx->secrets->boot_verified, "boot_verified");
	dump_secret(ctx->secrets->nvm_wp, "nvm_wp");
	dump_secret(ctx->secrets->nvm_rw, "nvm_rw");
	dump_secret(wsr, "wsr");
}

static int derive_secrets(struct vba_context *ctx,
			  const uint8_t *bdb, uint8_t *wsr)
{
	struct bdb_secrets secrets;

	memset(&secrets, 0, sizeof(secrets));

	ctx->secrets = &secrets;
	if (vba_extend_secrets_ro(ctx, bdb, wsr, extend)) {
		fprintf(stderr, "ERROR: Failed to derive secrets\n");
		return -1;
	}

	fprintf(stderr, "LOG: Secrets are derived as follows\n");
	dump_secrets(ctx, wsr);

	return 0;
}

int main(int argc, char *argv[])
{
	struct vba_context ctx;
	uint8_t *bdb, *bds;
	uint8_t *key_digest = NULL;
	uint32_t bdb_size, bds_size, digest_size;
	const char *bdb_file = NULL;
	const char *digest_file = NULL;
	const char *bds_file = NULL;
	int rv;
	int opt;

	while ((opt = getopt(argc, argv, "b:d:hms:")) != -1) {
		switch(opt) {
		case 'b':
			bdb_file = optarg;
			break;
		case 'd':
			digest_file = optarg;
			break;
		case 'h':
			help();
			return 0;
		case 'm':
			extend = mvmap2315_sha256_extend;
			break;
		case 's':
			bds_file = optarg;
			break;
		default:
			help();
			return -1;
		}
	}

	if (!bdb_file || !bds_file) {
		fprintf(stderr, "ERROR: BDB and BDS aren't specified\n\n");
		help();
		return -1;
	}

	/* Read BDB */
	bdb = read_file(bdb_file, &bdb_size);
	if (!bdb) {
		fprintf(stderr, "ERROR: Unable to read %s\n", bdb_file);
		return -1;
	}

	/* Read BDS */
	bds = read_file(bds_file, &bds_size);
	if (!bds) {
		fprintf(stderr, "ERROR: Unable to read %s\n", bds_file);
		return -1;
	}
	if (bds_size != BDB_SECRET_SIZE) {
		fprintf(stderr, "ERROR: Invalid BDS size: %d\n", bds_size);
		return -1;
	}

	/* Read key digest if provided */
	if (digest_file) {
		key_digest = read_file(digest_file, &digest_size);
		if (!key_digest) {
			fprintf(stderr,
				"ERROR: Unable to read %s\n", digest_file);
			return -1;
		}
	}

	/* Verify BDB and set a flag based on the result */
	memset(&ctx, 0, sizeof(ctx));
	rv = bdb_verify(bdb, bdb_size, key_digest);
	if (rv) {
		if (rv != BDB_GOOD_OTHER_THAN_KEY) {
			fprintf(stderr, "ERROR: BDB is invalid: %d\n", rv);
			return -1;
		}
		fprintf(stderr,
			"WARNING: BDB is valid but key digest doesn't match\n");
	} else {
		ctx.flags |= VBA_CONTEXT_FLAG_BDB_KEY_EFUSED;
		fprintf(stderr, "BDB is successfully verified by eFused key\n");
	}

	/* Derive secrets and dump the values */
	return derive_secrets(&ctx, bdb, bds);
}
