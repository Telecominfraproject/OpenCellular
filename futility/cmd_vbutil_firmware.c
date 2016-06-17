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
#include "cryptolib.h"
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
static int Vblock(const char *outfile, const char *keyblock_file,
		  const char *signprivate, uint64_t version,
		  const char *fv_file, const char *kernelkey_file,
		  uint32_t preamble_flags)
{

	VbPrivateKey *signing_key;
	VbPublicKey *kernel_subkey;
	VbKeyBlockHeader *key_block;
	uint64_t key_block_size;
	uint8_t *fv_data;
	uint64_t fv_size;
	FILE *f;
	uint64_t i;

	if (!outfile) {
		VbExError("Must specify output filename\n");
		return 1;
	}
	if (!keyblock_file || !signprivate || !kernelkey_file) {
		VbExError("Must specify all keys\n");
		return 1;
	}
	if (!fv_file) {
		VbExError("Must specify firmware volume\n");
		return 1;
	}

	/* Read the key block and keys */
	key_block =
	    (VbKeyBlockHeader *) ReadFile(keyblock_file, &key_block_size);
	if (!key_block) {
		VbExError("Error reading key block.\n");
		return 1;
	}

	signing_key = PrivateKeyRead(signprivate);
	if (!signing_key) {
		VbExError("Error reading signing key.\n");
		return 1;
	}
	struct vb2_private_key *signing_key2 =
		vb2_read_private_key(signprivate);
	if (!signing_key2) {
		VbExError("Error reading signing key.\n");
		return 1;
	}

	kernel_subkey = PublicKeyRead(kernelkey_file);
	if (!kernel_subkey) {
		VbExError("Error reading kernel subkey.\n");
		return 1;
	}

	/* Read and sign the firmware volume */
	fv_data = ReadFile(fv_file, &fv_size);
	if (!fv_data)
		return 1;
	if (!fv_size) {
		VbExError("Empty firmware volume file\n");
		return 1;
	}
	struct vb2_signature *body_sig =
		vb2_calculate_signature(fv_data, fv_size, signing_key2);
	if (!body_sig) {
		VbExError("Error calculating body signature\n");
		return 1;
	}
	free(fv_data);

	/* Create preamble */
	struct vb2_fw_preamble *preamble =
		vb2_create_fw_preamble(version,
				       (struct vb2_packed_key *)kernel_subkey,
				       body_sig, signing_key2, preamble_flags);
	if (!preamble) {
		VbExError("Error creating preamble.\n");
		return 1;
	}

	/* Write the output file */
	f = fopen(outfile, "wb");
	if (!f) {
		VbExError("Can't open output file %s\n", outfile);
		return 1;
	}
	i = ((1 != fwrite(key_block, key_block_size, 1, f)) ||
	     (1 != fwrite(preamble, preamble->preamble_size, 1, f)));
	fclose(f);
	if (i) {
		VbExError("Can't write output file %s\n", outfile);
		unlink(outfile);
		return 1;
	}

	/* Success */
	return 0;
}

static int Verify(const char *infile, const char *signpubkey,
		  const char *fv_file, const char *kernelkey_file)
{
	uint8_t workbuf[VB2_WORKBUF_RECOMMENDED_SIZE];
	struct vb2_workbuf wb;
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	uint32_t now = 0;

	if (!infile || !signpubkey || !fv_file) {
		VbExError("Must specify filename, signpubkey, and fv\n");
		return 1;
	}

	/* Read public signing key */
	uint8_t *pubkbuf;
	uint32_t pubklen;
	struct vb2_public_key sign_key;
	if (VB2_SUCCESS != vb2_read_file(signpubkey, &pubkbuf, &pubklen)) {
		fprintf(stderr, "Error reading signpubkey.\n");
		return 1;
	}
	if (VB2_SUCCESS != vb2_unpack_key(&sign_key, pubkbuf, pubklen)) {
		fprintf(stderr, "Error unpacking signpubkey.\n");
		return 1;
	}

	/* Read blob */
	uint8_t *blob;
	uint32_t blob_size;
	if (VB2_SUCCESS != vb2_read_file(infile, &blob, &blob_size)) {
		VbExError("Error reading input file\n");
		return 1;
	}

	/* Read firmware volume */
	uint8_t *fv_data;
	uint32_t fv_size;
	if (VB2_SUCCESS != vb2_read_file(fv_file, &fv_data, &fv_size)) {
		VbExError("Error reading firmware volume\n");
		return 1;
	}

	/* Verify key block */
	struct vb2_keyblock *keyblock = (struct vb2_keyblock *)blob;
	if (VB2_SUCCESS !=
	    vb2_verify_keyblock(keyblock, blob_size, &sign_key, &wb)) {
		VbExError("Error verifying key block.\n");
		return 1;
	}
	free(pubkbuf);

	now += keyblock->keyblock_size;

	printf("Key block:\n");
	printf("  Size:                %d\n", keyblock->keyblock_size);
	printf("  Flags:               %d (ignored)\n",
	       keyblock->keyblock_flags);

	struct vb2_packed_key *packed_key = &keyblock->data_key;
	printf("  Data key algorithm:  %d %s\n", packed_key->algorithm,
	       vb1_crypto_name(packed_key->algorithm));
	printf("  Data key version:    %d\n", packed_key->key_version);
	printf("  Data key sha1sum:    %s\n",
	       packed_key_sha1_string(packed_key));

	struct vb2_public_key data_key;
	if (VB2_SUCCESS !=
	    vb2_unpack_key(&data_key, (const uint8_t *)&keyblock->data_key,
			   keyblock->data_key.key_offset +
			   keyblock->data_key.key_size)) {
		fprintf(stderr, "Error parsing data key.\n");
		return 1;
	}

	/* Verify preamble */
	struct vb2_fw_preamble *pre2 = (struct vb2_fw_preamble *)(blob + now);
	if (VB2_SUCCESS !=
	    vb2_verify_fw_preamble(pre2, blob_size - now, &data_key, &wb)) {
		VbExError("Error2 verifying preamble.\n");
		return 1;
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
	       vb1_crypto_name(kernel_subkey->algorithm));
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
		return 1;
	}

	if (kernelkey_file) {
		if (0 != PublicKeyWrite(kernelkey_file,
					(struct VbPublicKey *)kernel_subkey)) {
			VbExError("Unable to write kernel subkey\n");
			return 1;
		}
	}

	return 0;
}

static int do_vbutil_firmware(int argc, char *argv[])
{

	char *filename = NULL;
	char *key_block_file = NULL;
	char *signpubkey = NULL;
	char *signprivate = NULL;
	uint64_t version = 0;
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
		return Vblock(filename, key_block_file, signprivate, version,
			      fv_file, kernelkey_file, preamble_flags);
	case OPT_MODE_VERIFY:
		return Verify(filename, signpubkey, fv_file, kernelkey_file);
	default:
		fprintf(stderr, "Must specify a mode.\n");
		print_help(argc, argv);
		return 1;
	}
}

DECLARE_FUTIL_COMMAND(vbutil_firmware, do_vbutil_firmware, VBOOT_VERSION_1_0,
		      "Verified boot firmware utility");
