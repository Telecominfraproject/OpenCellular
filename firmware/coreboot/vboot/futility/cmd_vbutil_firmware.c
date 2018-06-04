/* Copyright 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot firmware utility
 */

#include <getopt.h>
#include <inttypes.h>		/* For PRIu64 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2rsa.h"
#include "futility.h"
#include "host_common.h"
#include "host_key2.h"
#include "kernel_blob.h"
#include "util_misc.h"
#include "vboot_common.h"
#include "vb1_helper.h"
#include "vb2_common.h"

/* Command line options */
enum {
	OPT_MODE_VBLOCK = 1000,
	OPT_MODE_VERIFY,
	OPT_KEYBLOCK,
	OPT_SIGNPUBKEY,
	OPT_SIGNPRIVATE,
	OPT_VERSION,
	OPT_FV,
	OPT_KERNELKEY,
	OPT_FLAGS,
	OPT_HELP,
};

static const struct option long_opts[] = {
	{"vblock", 1, 0, OPT_MODE_VBLOCK},
	{"verify", 1, 0, OPT_MODE_VERIFY},
	{"keyblock", 1, 0, OPT_KEYBLOCK},
	{"signpubkey", 1, 0, OPT_SIGNPUBKEY},
	{"signprivate", 1, 0, OPT_SIGNPRIVATE},
	{"version", 1, 0, OPT_VERSION},
	{"fv", 1, 0, OPT_FV},
	{"kernelkey", 1, 0, OPT_KERNELKEY},
	{"flags", 1, 0, OPT_FLAGS},
	{"help", 0, 0, OPT_HELP},
	{NULL, 0, 0, 0}
};

/* Print help and return error */
static void print_help(int argc, char *argv[])
{
	printf("\nUsage:  " MYNAME " %s <--vblock|--verify> <file> [OPTIONS]\n"
	       "\n"
	       "For '--vblock <file>', required OPTIONS are:\n"
	       "\n"
	       "  --keyblock <file>           Key block in .keyblock format\n"
	       "  --signprivate <file>"
	       "        Signing private key in .vbprivk format\n"
	       "  --version <number>          Firmware version\n"
	       "  --fv <file>                 Firmware volume to sign\n"
	       "  --kernelkey <file>          Kernel subkey in .vbpubk format\n"
	       "\n"
	       "optional OPTIONS are:\n"
	       "  --flags <number>            Preamble flags (defaults to 0)\n"
	       "\n"
	       "For '--verify <file>', required OPTIONS are:\n"
	       "\n"
	       "  --signpubkey <file>"
	       "         Signing public key in .vbpubk format\n"
	       "  --fv <file>                 Firmware volume to verify\n"
	       "\n"
	       "For '--verify <file>', optional OPTIONS are:\n"
	       "  --kernelkey <file>"
	       "          Write the kernel subkey to this file\n\n",
	       argv[0]);
}

/* Create a firmware .vblock */
static int do_vblock(const char *outfile, const char *keyblock_file,
		     const char *signprivate, uint32_t version,
		     const char *fv_file, const char *kernelkey_file,
		     uint32_t preamble_flags)
{
	struct vb2_keyblock *keyblock = NULL;
	struct vb2_private_key *signing_key = NULL;
	struct vb2_packed_key *kernel_subkey = NULL;
	struct vb2_signature *body_sig = NULL;
	struct vb2_fw_preamble *preamble = NULL;
	uint8_t *fv_data = NULL;
	int retval = 1;

	if (!outfile) {
		VbExError("Must specify output filename\n");
		goto vblock_cleanup;
	}
	if (!keyblock_file || !signprivate || !kernelkey_file) {
		VbExError("Must specify all keys\n");
		goto vblock_cleanup;
	}
	if (!fv_file) {
		VbExError("Must specify firmware volume\n");
		goto vblock_cleanup;
	}

	/* Read the key block and keys */
	keyblock = vb2_read_keyblock(keyblock_file);
	if (!keyblock) {
		VbExError("Error reading key block.\n");
		goto vblock_cleanup;
	}

	signing_key = vb2_read_private_key(signprivate);
	if (!signing_key) {
		VbExError("Error reading signing key.\n");
		goto vblock_cleanup;
	}

	kernel_subkey = vb2_read_packed_key(kernelkey_file);
	if (!kernel_subkey) {
		VbExError("Error reading kernel subkey.\n");
		goto vblock_cleanup;
	}

	/* Read and sign the firmware volume */
	uint32_t fv_size;
	if (VB2_SUCCESS != vb2_read_file(fv_file, &fv_data, &fv_size))
		goto vblock_cleanup;
	if (!fv_size) {
		VbExError("Empty firmware volume file\n");
		goto vblock_cleanup;
	}
	body_sig = vb2_calculate_signature(fv_data, fv_size, signing_key);
	if (!body_sig) {
		VbExError("Error calculating body signature\n");
		goto vblock_cleanup;
	}

	/* Create preamble */
	preamble = vb2_create_fw_preamble(version, kernel_subkey, body_sig,
					  signing_key, preamble_flags);
	if (!preamble) {
		VbExError("Error creating preamble.\n");
		goto vblock_cleanup;
	}

	/* Write the output file */
	FILE *f = fopen(outfile, "wb");
	if (!f) {
		VbExError("Can't open output file %s\n", outfile);
		goto vblock_cleanup;
	}
	int i = ((1 != fwrite(keyblock, keyblock->keyblock_size, 1, f)) ||
		 (1 != fwrite(preamble, preamble->preamble_size, 1, f)));
	fclose(f);
	if (i) {
		VbExError("Can't write output file %s\n", outfile);
		unlink(outfile);
		goto vblock_cleanup;
	}

	/* Success */
	retval = 0;

vblock_cleanup:
	if (keyblock)
		free(keyblock);
	if (signing_key)
		free(signing_key);
	if (kernel_subkey)
		free(kernel_subkey);
	if (fv_data)
		free(fv_data);
	if (body_sig)
		free(body_sig);
	if (preamble)
		free(preamble);

	return retval;
}

static int do_verify(const char *infile, const char *signpubkey,
		     const char *fv_file, const char *kernelkey_file)
{
	uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE];
	struct vb2_workbuf wb;
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	uint32_t now = 0;

	uint8_t *pubkbuf = NULL;
	uint8_t *blob = NULL;
	uint8_t *fv_data = NULL;
	int retval = 1;

	if (!infile || !signpubkey || !fv_file) {
		VbExError("Must specify filename, signpubkey, and fv\n");
		goto verify_cleanup;
	}

	/* Read public signing key */
	uint32_t pubklen;
	struct vb2_public_key sign_key;
	if (VB2_SUCCESS != vb2_read_file(signpubkey, &pubkbuf, &pubklen)) {
		fprintf(stderr, "Error reading signpubkey.\n");
		goto verify_cleanup;
	}
	if (VB2_SUCCESS != vb2_unpack_key_buffer(&sign_key, pubkbuf, pubklen)) {
		fprintf(stderr, "Error unpacking signpubkey.\n");
		goto verify_cleanup;
	}

	/* Read blob */
	uint32_t blob_size;
	if (VB2_SUCCESS != vb2_read_file(infile, &blob, &blob_size)) {
		VbExError("Error reading input file\n");
		goto verify_cleanup;
	}

	/* Read firmware volume */
	uint32_t fv_size;
	if (VB2_SUCCESS != vb2_read_file(fv_file, &fv_data, &fv_size)) {
		VbExError("Error reading firmware volume\n");
		goto verify_cleanup;
	}

	/* Verify key block */
	struct vb2_keyblock *keyblock = (struct vb2_keyblock *)blob;
	if (VB2_SUCCESS !=
	    vb2_verify_keyblock(keyblock, blob_size, &sign_key, &wb)) {
		VbExError("Error verifying key block.\n");
		goto verify_cleanup;
	}

	now += keyblock->keyblock_size;

	printf("Key block:\n");
	printf("  Size:                %d\n", keyblock->keyblock_size);
	printf("  Flags:               %d (ignored)\n",
	       keyblock->keyblock_flags);

	struct vb2_packed_key *packed_key = &keyblock->data_key;
	printf("  Data key algorithm:  %d %s\n", packed_key->algorithm,
	       vb2_get_crypto_algorithm_name(packed_key->algorithm));
	printf("  Data key version:    %d\n", packed_key->key_version);
	printf("  Data key sha1sum:    %s\n",
	       packed_key_sha1_string(packed_key));

	struct vb2_public_key data_key;
	if (VB2_SUCCESS !=
	    vb2_unpack_key(&data_key, &keyblock->data_key)) {
		fprintf(stderr, "Error parsing data key.\n");
		goto verify_cleanup;
	}

	/* Verify preamble */
	struct vb2_fw_preamble *pre2 = (struct vb2_fw_preamble *)(blob + now);
	if (VB2_SUCCESS !=
	    vb2_verify_fw_preamble(pre2, blob_size - now, &data_key, &wb)) {
		VbExError("Error2 verifying preamble.\n");
		goto verify_cleanup;
	}
	now += pre2->preamble_size;

	uint32_t flags = pre2->flags;
	if (pre2->header_version_minor < 1)
		flags = 0;  /* Old 2.0 structure didn't have flags */

	printf("Preamble:\n");
	printf("  Size:                  %d\n", pre2->preamble_size);
	printf("  Header version:        %d.%d\n",
	       pre2->header_version_major, pre2->header_version_minor);
	printf("  Firmware version:      %d\n", pre2->firmware_version);

	struct vb2_packed_key *kernel_subkey = &pre2->kernel_subkey;
	printf("  Kernel key algorithm:  %d %s\n", kernel_subkey->algorithm,
	       vb2_get_crypto_algorithm_name(kernel_subkey->algorithm));
	printf("  Kernel key version:    %d\n", kernel_subkey->key_version);
	printf("  Kernel key sha1sum:    %s\n",
	       packed_key_sha1_string(kernel_subkey));
	printf("  Firmware body size:    %d\n", pre2->body_signature.data_size);
	printf("  Preamble flags:        %d\n", flags);

	/* TODO: verify body size same as signature size */

	/* Verify body */
	if (flags & VB2_FIRMWARE_PREAMBLE_USE_RO_NORMAL) {
		printf("Preamble requests USE_RO_NORMAL;"
		       " skipping body verification.\n");
	} else if (VB2_SUCCESS ==
		   vb2_verify_data(fv_data, fv_size, &pre2->body_signature,
				   &data_key, &wb)) {
		printf("Body verification succeeded.\n");
	} else {
		VbExError("Error verifying firmware body.\n");
		goto verify_cleanup;
	}

	if (kernelkey_file &&
	    VB2_SUCCESS != vb2_write_packed_key(kernelkey_file,
						kernel_subkey)) {
		VbExError("Unable to write kernel subkey\n");
		goto verify_cleanup;
	}

	/* Success */
	retval = 0;

verify_cleanup:
	if (pubkbuf)
		free(pubkbuf);
	if (blob)
		free(blob);
	if (fv_data)
		free(fv_data);

	return retval;
}

static int do_vbutil_firmware(int argc, char *argv[])
{

	char *filename = NULL;
	char *key_block_file = NULL;
	char *signpubkey = NULL;
	char *signprivate = NULL;
	uint32_t version = 0;
	char *fv_file = NULL;
	char *kernelkey_file = NULL;
	uint32_t preamble_flags = 0;
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
		case OPT_HELP:
			print_help(argc, argv);
			return !!parse_error;

		case OPT_MODE_VBLOCK:
		case OPT_MODE_VERIFY:
			mode = i;
			filename = optarg;
			break;

		case OPT_KEYBLOCK:
			key_block_file = optarg;
			break;

		case OPT_SIGNPUBKEY:
			signpubkey = optarg;
			break;

		case OPT_SIGNPRIVATE:
			signprivate = optarg;
			break;

		case OPT_FV:
			fv_file = optarg;
			break;

		case OPT_KERNELKEY:
			kernelkey_file = optarg;
			break;

		case OPT_VERSION:
			version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				printf("Invalid --version\n");
				parse_error = 1;
			}
			break;

		case OPT_FLAGS:
			preamble_flags = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				printf("Invalid --flags\n");
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
	case OPT_MODE_VBLOCK:
		return do_vblock(filename, key_block_file, signprivate, version,
				 fv_file, kernelkey_file, preamble_flags);
	case OPT_MODE_VERIFY:
		return do_verify(filename, signpubkey, fv_file, kernelkey_file);
	default:
		fprintf(stderr, "Must specify a mode.\n");
		print_help(argc, argv);
		return 1;
	}
}

DECLARE_FUTIL_COMMAND(vbutil_firmware, do_vbutil_firmware, VBOOT_VERSION_1_0,
		      "Verified boot firmware utility");
