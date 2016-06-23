/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot key utility
 */

#include <getopt.h>
#include <inttypes.h>		/* For PRIu64 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "cryptolib.h"
#include "futility.h"
#include "host_common.h"
#include "host_key2.h"
#include "util_misc.h"
#include "vb1_helper.h"
#include "vb2_common.h"
#include "vboot_common.h"

/* Command line options */
enum {
	OPT_INKEY = 1000,
	OPT_KEY_VERSION,
	OPT_ALGORITHM,
	OPT_MODE_PACK,
	OPT_MODE_UNPACK,
	OPT_COPYTO,
	OPT_HELP,
};

static const struct option long_opts[] = {
	{"key", 1, 0, OPT_INKEY},
	{"version", 1, 0, OPT_KEY_VERSION},
	{"algorithm", 1, 0, OPT_ALGORITHM},
	{"pack", 1, 0, OPT_MODE_PACK},
	{"unpack", 1, 0, OPT_MODE_UNPACK},
	{"copyto", 1, 0, OPT_COPYTO},
	{"help", 0, 0, OPT_HELP},
	{NULL, 0, 0, 0}
};

static void print_help(int argc, char *argv[])
{
	int i;

	printf("\n"
	       "Usage:  " MYNAME " %s --pack <outfile> [PARAMETERS]\n"
	       "\n"
	       "  Required parameters:\n"
	       "    --key <infile>              RSA key file (.keyb or .pem)\n"
	       "    --version <number>          Key version number "
	       "(required for .keyb,\n"
	       "                                  ignored for .pem)\n"
	       "    --algorithm <number>        "
	       "Signing algorithm to use with key:\n", argv[0]);

	for (i = 0; i < kNumAlgorithms; i++) {
		printf("                                  %d = (%s)\n",
		       i, vb1_crypto_name(i));
	}

	printf("\nOR\n\n"
	       "Usage:  " MYNAME " %s --unpack <infile>\n"
	       "\n"
	       "  Optional parameters:\n"
	       "    --copyto <file>             "
	       "Write a copy of the key to this file.\n\n", argv[0]);
}

/* Pack a .keyb file into a .vbpubk, or a .pem into a .vbprivk */
static int do_pack(const char *infile, const char *outfile, uint32_t algorithm,
		   uint32_t version)
{
	if (!infile || !outfile) {
		fprintf(stderr, "vbutil_key: Must specify --in and --out\n");
		return 1;
	}

	struct vb2_packed_key *pubkey =
		vb2_read_packed_keyb(infile, algorithm, version);
	if (pubkey) {
		if (0 != vb2_write_packed_key(outfile, pubkey)) {
			fprintf(stderr, "vbutil_key: Error writing key.\n");
			return 1;
		}
		free(pubkey);
		return 0;
	}

	struct vb2_private_key *privkey =
		vb2_read_private_key_pem(infile, algorithm);
	if (privkey) {
		if (VB2_SUCCESS != vb2_write_private_key(outfile, privkey)) {
			fprintf(stderr, "vbutil_key: Error writing key.\n");
			return 1;
		}
		free(privkey);
		return 0;
	}

	VbExError("Unable to parse either .keyb or .pem from %s\n", infile);
	return 1;
}

/* Unpack a .vbpubk or .vbprivk */
static int do_unpack(const char *infile, const char *outfile)
{
	struct vb2_packed_key *pubkey;

	if (!infile) {
		fprintf(stderr, "Need file to unpack\n");
		return 1;
	}

	pubkey = vb2_read_packed_key(infile);
	if (pubkey) {
		printf("Public Key file:   %s\n", infile);
		printf("Algorithm:         %u %s\n", pubkey->algorithm,
		       vb1_crypto_name(pubkey->algorithm));
		printf("Key Version:       %u\n", pubkey->key_version);
		printf("Key sha1sum:       %s\n",
		       packed_key_sha1_string(pubkey));
		if (outfile &&
		    VB2_SUCCESS != vb2_write_packed_key(outfile, pubkey)) {
			fprintf(stderr, "butil_key: Error writing key copy\n");
			free(pubkey);
			return 1;
		}
		free(pubkey);
		return 0;
	}

	struct vb2_private_key *privkey = vb2_read_private_key(infile);
	if (privkey) {
		printf("Private Key file:  %s\n", infile);

		enum vb2_crypto_algorithm alg =
			vb2_get_crypto_algorithm(privkey->hash_alg,
						 privkey->sig_alg);
		printf("Algorithm:         %u %s\n", alg, vb1_crypto_name(alg));
		if (outfile &&
		    VB2_SUCCESS != vb2_write_private_key(outfile, privkey)) {
			fprintf(stderr,"vbutil_key: Error writing key copy\n");
			free(privkey);
			return 1;
		}
		free(privkey);
		return 0;
	}

	VbExError("Unable to parse either .vbpubk or vbprivk from %s\n",
		  infile);
	return 1;
}

static int do_vbutil_key(int argc, char *argv[])
{

	char *infile = NULL;
	char *outfile = NULL;
	int mode = 0;
	int parse_error = 0;
	uint32_t version = 1;
	uint32_t algorithm = VB2_ALG_COUNT;
	char *e;
	int i;

	while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
		switch (i) {
		case '?':
			/* Unhandled option */
			VbExError("Unknown option\n");
			parse_error = 1;
			break;
		case OPT_HELP:
			print_help(argc, argv);
			return !!parse_error;

		case OPT_INKEY:
			infile = optarg;
			break;

		case OPT_KEY_VERSION:
			version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				VbExError("Invalid --version\n");
				parse_error = 1;
			}
			break;

		case OPT_ALGORITHM:
			algorithm = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				VbExError("Invalid --algorithm\n");
				parse_error = 1;
			}
			break;

		case OPT_MODE_PACK:
			mode = i;
			outfile = optarg;
			break;

		case OPT_MODE_UNPACK:
			mode = i;
			infile = optarg;
			break;

		case OPT_COPYTO:
			outfile = optarg;
			break;
		}
	}

	if (parse_error) {
		print_help(argc, argv);
		return 1;
	}

	switch (mode) {
	case OPT_MODE_PACK:
		return do_pack(infile, outfile, algorithm, version);
	case OPT_MODE_UNPACK:
		return do_unpack(infile, outfile);
	default:
		printf("Must specify a mode.\n");
		print_help(argc, argv);
		return 1;
	}
}

DECLARE_FUTIL_COMMAND(vbutil_key, do_vbutil_key, VBOOT_VERSION_1_0,
		      "Wraps RSA keys with vboot headers");
