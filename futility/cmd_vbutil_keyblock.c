/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot key block utility
 */

#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cryptolib.h"
#include "futility.h"
#include "host_common.h"
#include "util_misc.h"
#include "vboot_common.h"

/* Command line options */
enum {
	OPT_MODE_PACK = 1000,
	OPT_MODE_UNPACK,
	OPT_DATAPUBKEY,
	OPT_SIGNPUBKEY,
	OPT_SIGNPRIVATE,
	OPT_SIGNPRIVATE_PEM,
	OPT_PEM_ALGORITHM,
	OPT_EXTERNAL_SIGNER,
	OPT_FLAGS,
};

static const struct option long_opts[] = {
	{"pack", 1, 0, OPT_MODE_PACK},
	{"unpack", 1, 0, OPT_MODE_UNPACK},
	{"datapubkey", 1, 0, OPT_DATAPUBKEY},
	{"signpubkey", 1, 0, OPT_SIGNPUBKEY},
	{"signprivate", 1, 0, OPT_SIGNPRIVATE},
	{"signprivate_pem", 1, 0, OPT_SIGNPRIVATE_PEM},
	{"pem_algorithm", 1, 0, OPT_PEM_ALGORITHM},
	{"externalsigner", 1, 0, OPT_EXTERNAL_SIGNER},
	{"flags", 1, 0, OPT_FLAGS},
	{NULL, 0, 0, 0}
};

static const char usage[] =
	"\n"
	"Usage:  " MYNAME " %s <--pack|--unpack> <file> [OPTIONS]\n"
	"\n"
	"For '--pack <file>', required OPTIONS are:\n"
	"  --datapubkey <file>         Data public key in .vbpubk format\n"
	"\n"
	"Optional OPTIONS are:\n"
	"  --signprivate <file>"
	"        Signing private key in .vbprivk format.\n"
	"OR\n"
	"  --signprivate_pem <file>\n"
	"  --pem_algorithm <algo>\n"
	"        Signing private key in .pem format and algorithm id.\n"
	"(If one of the above arguments is not specified, the keyblock will\n"
	"not be signed.)\n"
	"\n"
	"  --flags <number>            Specifies allowed use conditions.\n"
	"  --externalsigner \"cmd\""
	"        Use an external program cmd to calculate the signatures.\n"
	"\n"
	"For '--unpack <file>', optional OPTIONS are:\n"
	"  --signpubkey <file>"
	"        Signing public key in .vbpubk format. This is required to\n"
	"                                verify a signed keyblock.\n"
	"  --datapubkey <file>"
	"        Write the data public key to this file.\n\n";

static void print_help(const char *progname)
{
	printf(usage, progname);
}

/* Pack a .keyblock */
static int Pack(const char *outfile, const char *datapubkey,
		const char *signprivate,
		const char *signprivate_pem, uint64_t pem_algorithm,
		uint64_t flags, const char *external_signer)
{
	VbPublicKey *data_key;
	VbPrivateKey *signing_key = NULL;
	VbKeyBlockHeader *block;

	if (!outfile) {
		fprintf(stderr,
			"vbutil_keyblock: Must specify output filename.\n");
		return 1;
	}
	if (!datapubkey) {
		fprintf(stderr,
			"vbutil_keyblock: Must specify data public key.\n");
		return 1;
	}

	data_key = PublicKeyRead(datapubkey);
	if (!data_key) {
		fprintf(stderr, "vbutil_keyblock: Error reading data key.\n");
		return 1;
	}

	if (signprivate_pem) {
		if (pem_algorithm >= kNumAlgorithms) {
			fprintf(stderr,
				"vbutil_keyblock: Invalid --pem_algorithm %"
				PRIu64 "\n", pem_algorithm);
			return 1;
		}
		if (external_signer) {
			/* External signing uses the PEM file directly. */
			block = KeyBlockCreate_external(data_key,
							signprivate_pem,
							pem_algorithm, flags,
							external_signer);
		} else {
			signing_key =
			    PrivateKeyReadPem(signprivate_pem, pem_algorithm);
			if (!signing_key) {
				fprintf(stderr, "vbutil_keyblock:"
					" Error reading signing key.\n");
				return 1;
			}
			block = KeyBlockCreate(data_key, signing_key, flags);
		}
	} else {
		if (signprivate) {
			signing_key = PrivateKeyRead(signprivate);
			if (!signing_key) {
				fprintf(stderr, "vbutil_keyblock:"
					" Error reading signing key.\n");
				return 1;
			}
		}
		block = KeyBlockCreate(data_key, signing_key, flags);
	}

	free(data_key);
	if (signing_key)
		free(signing_key);

	if (0 != KeyBlockWrite(outfile, block)) {
		fprintf(stderr, "vbutil_keyblock: Error writing key block.\n");
		return 1;
	}
	free(block);
	return 0;
}

static int Unpack(const char *infile, const char *datapubkey,
		  const char *signpubkey)
{
	VbPublicKey *data_key;
	VbPublicKey *sign_key = NULL;
	VbKeyBlockHeader *block;

	if (!infile) {
		fprintf(stderr, "vbutil_keyblock: Must specify filename\n");
		return 1;
	}

	block = KeyBlockRead(infile);
	if (!block) {
		fprintf(stderr, "vbutil_keyblock: Error reading key block.\n");
		return 1;
	}

	/* If the block is signed, then verify it with the signing public key,
	 * since KeyBlockRead() only verified the hash. */
	if (block->key_block_signature.sig_size && signpubkey) {
		sign_key = PublicKeyRead(signpubkey);
		if (!sign_key) {
			fprintf(stderr,
				"vbutil_keyblock: Error reading signpubkey.\n");
			return 1;
		}
		if (0 !=
		    KeyBlockVerify(block, block->key_block_size, sign_key, 0)) {
			fprintf(stderr, "vbutil_keyblock:"
				" Error verifying key block.\n");
			return 1;
		}
		free(sign_key);
	}

	printf("Key block file:       %s\n", infile);
	printf("Signature             %s\n", sign_key ? "valid" : "ignored");
	printf("Flags:                %" PRIu64 " ", block->key_block_flags);
	if (block->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_0)
		printf(" !DEV");
	if (block->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_1)
		printf(" DEV");
	if (block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_0)
		printf(" !REC");
	if (block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_1)
		printf(" REC");
	printf("\n");

	data_key = &block->data_key;
	printf("Data key algorithm:   %" PRIu64 " %s\n", data_key->algorithm,
	       (data_key->algorithm < kNumAlgorithms ?
		algo_strings[data_key->algorithm] : "(invalid)"));
	printf("Data key version:     %" PRIu64 "\n", data_key->key_version);
	printf("Data key sha1sum:     ");
	PrintPubKeySha1Sum(data_key);
	printf("\n");

	if (datapubkey) {
		if (0 != PublicKeyWrite(datapubkey, data_key)) {
			fprintf(stderr, "vbutil_keyblock:"
				" unable to write public key\n");
			return 1;
		}
	}

	free(block);
	return 0;
}

static int do_vbutil_keyblock(int argc, char *argv[])
{

	char *filename = NULL;
	char *datapubkey = NULL;
	char *signpubkey = NULL;
	char *signprivate = NULL;
	char *signprivate_pem = NULL;
	char *external_signer = NULL;
	uint64_t flags = 0;
	uint64_t pem_algorithm = 0;
	int is_pem_algorithm = 0;
	int mode = 0;
	int parse_error = 0;
	char *e;
	int i;

	while ((i = getopt_long(argc, argv, "", long_opts, NULL)) != -1) {
		switch (i) {
		case '?':
			/* Unhandled option */
			printf("Unknown option\n");
			parse_error = 1;
			break;

		case OPT_MODE_PACK:
		case OPT_MODE_UNPACK:
			mode = i;
			filename = optarg;
			break;

		case OPT_DATAPUBKEY:
			datapubkey = optarg;
			break;

		case OPT_SIGNPUBKEY:
			signpubkey = optarg;
			break;

		case OPT_SIGNPRIVATE:
			signprivate = optarg;
			break;

		case OPT_SIGNPRIVATE_PEM:
			signprivate_pem = optarg;
			break;

		case OPT_PEM_ALGORITHM:
			pem_algorithm = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --pem_algorithm\n");
				parse_error = 1;
			} else {
				is_pem_algorithm = 1;
			}
			break;

		case OPT_EXTERNAL_SIGNER:
			external_signer = optarg;
			break;

		case OPT_FLAGS:
			flags = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --flags\n");
				parse_error = 1;
			}
			break;
		}
	}

	/* Check if the right combination of options was provided. */
	if (signprivate && signprivate_pem) {
		fprintf(stderr,
			"Only one of --signprivate or --signprivate_pem must"
			" be specified\n");
		parse_error = 1;
	}

	if (signprivate_pem && !is_pem_algorithm) {
		fprintf(stderr, "--pem_algorithm must be used with"
			" --signprivate_pem\n");
		parse_error = 1;
	}

	if (external_signer && !signprivate_pem) {
		fprintf(stderr,
			"--externalsigner must be used with --signprivate_pem"
			"\n");
		parse_error = 1;
	}

	if (parse_error) {
		print_help(argv[0]);
		return 1;
	}

	switch (mode) {
	case OPT_MODE_PACK:
		return Pack(filename, datapubkey, signprivate,
			    signprivate_pem, pem_algorithm,
			    flags, external_signer);
	case OPT_MODE_UNPACK:
		return Unpack(filename, datapubkey, signpubkey);
	default:
		printf("Must specify a mode.\n");
		print_help(argv[0]);
		return 1;
	}
}

DECLARE_FUTIL_COMMAND(vbutil_keyblock, do_vbutil_keyblock,
		      VBOOT_VERSION_1_0,
		      "Creates, signs, and verifies a keyblock",
		      print_help);
