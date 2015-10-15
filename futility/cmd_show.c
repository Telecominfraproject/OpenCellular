/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define OPENSSL_NO_SHA
#include <openssl/rsa.h>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "file_type.h"
#include "file_type_bios.h"
#include "fmap.h"
#include "futility.h"
#include "futility_options.h"
#include "host_common.h"
#include "util_misc.h"
#include "vb1_helper.h"
#include "vboot_common.h"
#include "2api.h"
#include "host_key2.h"

/* Options */
struct show_option_s show_option = {
	.padding = 65536,
	.type = FILE_TYPE_UNKNOWN,
};

void show_pubkey(VbPublicKey *pubkey, const char *sp)
{
	printf("%sVboot API:           1.0\n", sp);
	printf("%sAlgorithm:           %" PRIu64 " %s\n", sp, pubkey->algorithm,
	       (pubkey->algorithm < kNumAlgorithms ?
		algo_strings[pubkey->algorithm] : "(invalid)"));
	printf("%sKey Version:         %" PRIu64 "\n", sp, pubkey->key_version);
	printf("%sKey sha1sum:         ", sp);
	PrintPubKeySha1Sum(pubkey);
	printf("\n");
}

static void show_keyblock(VbKeyBlockHeader *key_block, const char *name,
			  int sign_key, int good_sig)
{
	if (name)
		printf("Key block:               %s\n", name);
	else
		printf("Key block:\n");
	printf("  Signature:             %s\n",
	       sign_key ? (good_sig ? "valid" : "invalid") : "ignored");
	printf("  Size:                  0x%" PRIx64 "\n",
	       key_block->key_block_size);
	printf("  Flags:                 %" PRIu64 " ",
	       key_block->key_block_flags);
	if (key_block->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_0)
		printf(" !DEV");
	if (key_block->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_1)
		printf(" DEV");
	if (key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_0)
		printf(" !REC");
	if (key_block->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_1)
		printf(" REC");
	printf("\n");

	VbPublicKey *data_key = &key_block->data_key;
	printf("  Data key algorithm:    %" PRIu64 " %s\n", data_key->algorithm,
	       (data_key->algorithm < kNumAlgorithms
		? algo_strings[data_key->algorithm]
		: "(invalid)"));
	printf("  Data key version:      %" PRIu64 "\n", data_key->key_version);
	printf("  Data key sha1sum:      ");
	PrintPubKeySha1Sum(data_key);
	printf("\n");
}

int ft_show_pubkey(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	VbPublicKey *pubkey = (VbPublicKey *)buf;

	if (!PublicKeyLooksOkay(pubkey, len)) {
		printf("%s looks bogus\n", name);
		return 1;
	}

	printf("Public Key file:       %s\n", name);
	show_pubkey(pubkey, "  ");

	return 0;
}

int ft_show_privkey(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	VbPrivateKey key;
	const unsigned char *start;
	int alg_okay;

	key.algorithm = *(typeof(key.algorithm) *)buf;
	start = buf + sizeof(key.algorithm);
	if (len <= sizeof(key.algorithm)) {
		printf("%s looks bogus\n", name);
		return 1;
	}
	len -= sizeof(key.algorithm);
	key.rsa_private_key = d2i_RSAPrivateKey(NULL, &start, len);

	printf("Private Key file:      %s\n", name);
	printf("  Vboot API:           1.0\n");
	alg_okay = key.algorithm < kNumAlgorithms;
	printf("  Algorithm:           %" PRIu64 " %s\n", key.algorithm,
	       alg_okay ? algo_strings[key.algorithm] : "(unknown)");
	printf("  Key sha1sum:         ");
	if (key.rsa_private_key) {
		PrintPrivKeySha1Sum(&key);
		RSA_free(key.rsa_private_key);
	} else {
		printf("<error>");
	}
	printf("\n");

	return 0;
}

int ft_show_keyblock(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	VbKeyBlockHeader *block = (VbKeyBlockHeader *)buf;
	VbPublicKey *sign_key = show_option.k;
	int good_sig = 0;
	int retval = 0;

	/* Check the hash only first */
	if (0 != KeyBlockVerify(block, len, NULL, 1)) {
		printf("%s is invalid\n", name);
		return 1;
	}

	/* Check the signature if we have one */
	if (sign_key && VBOOT_SUCCESS ==
	    KeyBlockVerify(block, len, sign_key, 0))
		good_sig = 1;

	if (show_option.strict && (!sign_key || !good_sig))
		retval = 1;

	show_keyblock(block, name, !!sign_key, good_sig);

	return retval;
}

int ft_show_fw_preamble(const char *name, uint8_t *buf, uint32_t len,
			void *data)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)buf;
	struct bios_state_s *state = (struct bios_state_s *)data;
	VbPublicKey *sign_key = show_option.k;
	uint8_t *fv_data = show_option.fv;
	uint64_t fv_size = show_option.fv_size;
	struct bios_area_s *fw_body_area = 0;
	int good_sig = 0;
	int retval = 0;

	/* Check the hash... */
	if (VBOOT_SUCCESS != KeyBlockVerify(key_block, len, NULL, 1)) {
		printf("%s keyblock component is invalid\n", name);
		return 1;
	}

	/*
	 * If we're being invoked while poking through a BIOS, we should
	 * be given the keys and data to verify as part of the state. If we
	 * have no state, then we're just looking at a standalone fw_preamble,
	 * so we'll have to get any keys or data from options.
	 */
	if (state) {
		if (!sign_key && state->rootkey.is_valid)
			/* BIOS should have a rootkey in the GBB */
			sign_key = (VbPublicKey *)state->rootkey.buf;
		/* Identify the firmware body for this VBLOCK */
		enum bios_component body_c = state->c == BIOS_FMAP_VBLOCK_A
			? BIOS_FMAP_FW_MAIN_A
			: BIOS_FMAP_FW_MAIN_B;
		fw_body_area = &state->area[body_c];
	}

	/* If we have a key, check the signature too */
	if (sign_key && VBOOT_SUCCESS ==
	    KeyBlockVerify(key_block, len, sign_key, 0))
		good_sig = 1;

	show_keyblock(key_block, name, !!sign_key, good_sig);

	if (show_option.strict && (!sign_key || !good_sig))
		retval = 1;

	RSAPublicKey *rsa = PublicKeyToRSA(&key_block->data_key);
	if (!rsa) {
		fprintf(stderr, "Error parsing data key in %s\n", name);
		return 1;
	}
	uint32_t more = key_block->key_block_size;
	VbFirmwarePreambleHeader *preamble =
		(VbFirmwarePreambleHeader *)(buf + more);

	if (VBOOT_SUCCESS != VerifyFirmwarePreamble(preamble,
						    len - more, rsa)) {
		printf("%s is invalid\n", name);
		return 1;
	}

	uint32_t flags = VbGetFirmwarePreambleFlags(preamble);
	printf("Firmware Preamble:\n");
	printf("  Size:                  %" PRIu64 "\n",
	       preamble->preamble_size);
	printf("  Header version:        %" PRIu32 ".%" PRIu32 "\n",
	       preamble->header_version_major, preamble->header_version_minor);
	printf("  Firmware version:      %" PRIu64 "\n",
	       preamble->firmware_version);
	VbPublicKey *kernel_subkey = &preamble->kernel_subkey;
	printf("  Kernel key algorithm:  %" PRIu64 " %s\n",
	       kernel_subkey->algorithm,
	       (kernel_subkey->algorithm < kNumAlgorithms ?
		algo_strings[kernel_subkey->algorithm] : "(invalid)"));
	if (kernel_subkey->algorithm >= kNumAlgorithms)
		retval = 1;
	printf("  Kernel key version:    %" PRIu64 "\n",
	       kernel_subkey->key_version);
	printf("  Kernel key sha1sum:    ");
	PrintPubKeySha1Sum(kernel_subkey);
	printf("\n");
	printf("  Firmware body size:    %" PRIu64 "\n",
	       preamble->body_signature.data_size);
	printf("  Preamble flags:        %" PRIu32 "\n", flags);


	if (flags & VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL) {
		printf("Preamble requests USE_RO_NORMAL;"
		       " skipping body verification.\n");
		goto done;
	}

	/* We'll need to get the firmware body from somewhere... */
	if (fw_body_area && fw_body_area->is_valid) {
		fv_data = fw_body_area->buf;
		fv_size = fw_body_area->len;
	}

	if (!fv_data) {
		printf("No firmware body available to verify.\n");
		if (show_option.strict)
			return 1;
		return 0;
	}

	if (VBOOT_SUCCESS !=
	    VerifyData(fv_data, fv_size, &preamble->body_signature, rsa)) {
		fprintf(stderr, "Error verifying firmware body.\n");
		return 1;
	}

done:
	/* Can't trust the BIOS unless everything is signed (in which case
	 * we've already returned), but standalone files are okay. */
	if (state || (sign_key && good_sig)) {
		if (!(flags & VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL))
			printf("Body verification succeeded.\n");
		if (state)
			state->area[state->c].is_valid = 1;
	} else {
		printf("Seems legit, but the signature is unverified.\n");
		if (show_option.strict)
			retval = 1;
	}

	return retval;
}

int ft_show_kernel_preamble(const char *name, uint8_t *buf, uint32_t len,
			    void *data)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)buf;
	VbPublicKey *sign_key = show_option.k;
	uint8_t *kernel_blob = 0;
	uint64_t kernel_size = 0;
	int good_sig = 0;
	int retval = 0;
	uint64_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;
	uint32_t flags = 0;

	/* Check the hash... */
	if (VBOOT_SUCCESS != KeyBlockVerify(key_block, len, NULL, 1)) {
		printf("%s keyblock component is invalid\n", name);
		return 1;
	}

	/* If we have a key, check the signature too */
	if (sign_key && VBOOT_SUCCESS ==
	    KeyBlockVerify(key_block, len, sign_key, 0))
		good_sig = 1;

	printf("Kernel partition:        %s\n", name);
	show_keyblock(key_block, NULL, !!sign_key, good_sig);

	if (show_option.strict && (!sign_key || !good_sig))
		retval = 1;

	RSAPublicKey *rsa = PublicKeyToRSA(&key_block->data_key);
	if (!rsa) {
		fprintf(stderr, "Error parsing data key in %s\n", name);
		return 1;
	}
	uint32_t more = key_block->key_block_size;
	VbKernelPreambleHeader *preamble =
		(VbKernelPreambleHeader *)(buf + more);

	if (VBOOT_SUCCESS != VerifyKernelPreamble(preamble,
						    len - more, rsa)) {
		printf("%s is invalid\n", name);
		return 1;
	}

	printf("Kernel Preamble:\n");
	printf("  Size:                  0x%" PRIx64 "\n",
	       preamble->preamble_size);
	printf("  Header version:        %" PRIu32 ".%" PRIu32 "\n",
	       preamble->header_version_major,
	       preamble->header_version_minor);
	printf("  Kernel version:        %" PRIu64 "\n",
	       preamble->kernel_version);
	printf("  Body load address:     0x%" PRIx64 "\n",
	       preamble->body_load_address);
	printf("  Body size:             0x%" PRIx64 "\n",
	       preamble->body_signature.data_size);
	printf("  Bootloader address:    0x%" PRIx64 "\n",
	       preamble->bootloader_address);
	printf("  Bootloader size:       0x%" PRIx64 "\n",
	       preamble->bootloader_size);

	if (VbGetKernelVmlinuzHeader(preamble,
				     &vmlinuz_header_address,
				     &vmlinuz_header_size)
	    != VBOOT_SUCCESS) {
		fprintf(stderr, "Unable to retrieve Vmlinuz Header!");
		return 1;
	}
	if (vmlinuz_header_size) {
		printf("  Vmlinuz_header address:    0x%" PRIx64 "\n",
		       vmlinuz_header_address);
		printf("  Vmlinuz header size:       0x%" PRIx64 "\n",
		       vmlinuz_header_size);
	}

	if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS)
		flags = preamble->flags;
	printf("  Flags:                 0x%" PRIx32 "\n", flags);

	/* Verify kernel body */
	if (show_option.fv) {
		/* It's in a separate file, which we've already read in */
		kernel_blob = show_option.fv;
		kernel_size = show_option.fv_size;
	} else if (len > show_option.padding) {
		/* It should be at an offset within the input file. */
		kernel_blob = buf + show_option.padding;
		kernel_size = len - show_option.padding;
	}

	if (!kernel_blob) {
		/* TODO: Is this always a failure? The preamble is okay. */
		fprintf(stderr, "No kernel blob available to verify.\n");
		return 1;
	}

	if (0 != VerifyData(kernel_blob, kernel_size,
			    &preamble->body_signature, rsa)) {
		fprintf(stderr, "Error verifying kernel body.\n");
		return 1;
	}

	printf("Body verification succeeded.\n");

	printf("Config:\n%s\n", kernel_blob + KernelCmdLineOffset(preamble));

	return retval;
}

enum no_short_opts {
	OPT_PADDING = 1000,
	OPT_TYPE,
	OPT_PUBKEY,
	OPT_HELP,
};

static const char usage[] = "\n"
	"Usage:  " MYNAME " %s [OPTIONS] FILE [...]\n"
	"\n"
	"Where FILE could be\n"
	"\n"
	"  a keyblock (.keyblock)\n"
	"  a firmware preamble signature (VBLOCK_A/B)\n"
	"  a firmware image (bios.bin)\n"
	"  a kernel partition (/dev/sda2, /dev/mmcblk0p2)\n"
	"  keys in various formats (.vbpubk, .vbprivk, .pem)\n"
	"  several other file types related to verified boot\n"
	"\n"
	"Options:\n"
	"  -t                               Just show the type of each file\n"
	"  --type           TYPE            Override the detected file type\n"
	"                                     Use \"--type help\" for a list\n"
	"Type-specific options:\n"
	"  -k|--publickey   FILE.vbpubk     Public key in vb1 format\n"
	"  --pubkey         FILE.vpubk2     Public key in vb2 format\n"
	"  -f|--fv          FILE            Verify this payload (FW_MAIN_A/B)\n"
	"  --pad            NUM             Kernel vblock padding size\n"
	"  --strict                         "
	"Fail unless all signatures are valid\n"
	"\n";

static void print_help(int argc, char *argv[])
{
	if (!strcmp(argv[0], "verify"))
		printf("\nUsage:  " MYNAME " %s [OPTIONS] FILE [...]\n\n"
		       "This is just an alias for\n\n"
		       "  " MYNAME " show --strict\n\n",
		       argv[0]);

	printf(usage, "show");
}

static const struct option long_opts[] = {
	/* name    hasarg *flag val */
	{"publickey",   1, 0, 'k'},
	{"fv",          1, 0, 'f'},
	{"pad",         1, NULL, OPT_PADDING},
	{"type",        1, NULL, OPT_TYPE},
	{"strict",      0, &show_option.strict, 1},
	{"pubkey",      1, NULL, OPT_PUBKEY},
	{"help",        0, NULL, OPT_HELP},
	{NULL, 0, NULL, 0},
};
static char *short_opts = ":f:k:t";


static int show_type(char *filename)
{
	enum futil_file_err err;
	enum futil_file_type type;
	err = futil_file_type(filename, &type);
	switch (err) {
	case FILE_ERR_NONE:
		printf("%s:\t%s\n", filename, futil_file_type_name(type));
		/* Only our recognized types return success */
		return 0;
	case FILE_ERR_DIR:
		printf("%s:\t%s\n", filename, "directory");
		break;
	case FILE_ERR_CHR:
		printf("%s:\t%s\n", filename, "character special");
		break;
	case FILE_ERR_FIFO:
		printf("%s:\t%s\n", filename, "FIFO");
		break;
	case FILE_ERR_SOCK:
		printf("%s:\t%s\n", filename, "socket");
		break;
	default:
		break;
	}
	/* Everything else is an error */
	return 1;
}

static int do_show(int argc, char *argv[])
{
	char *infile = 0;
	int ifd, i;
	int errorcnt = 0;
	uint8_t *buf;
	uint32_t len;
	char *e = 0;
	int type_override = 0;
	enum futil_file_type type;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, short_opts, long_opts, 0)) != -1) {
		switch (i) {
		case 'f':
			show_option.fv = ReadFile(optarg,
						  &show_option.fv_size);
			if (!show_option.fv) {
				fprintf(stderr, "Error reading %s: %s\n",
					optarg, strerror(errno));
				errorcnt++;
			}
			break;
		case 'k':
			show_option.k = PublicKeyRead(optarg);
			if (!show_option.k) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 't':
			show_option.t_flag = 1;
			break;
		case OPT_PADDING:
			show_option.padding = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --padding \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_TYPE:
			if (!futil_str_to_file_type(optarg,
						    &show_option.type)) {
				if (!strcasecmp("help", optarg))
					print_file_types_and_exit(errorcnt);
				fprintf(stderr,
					"Invalid --type \"%s\"\n", optarg);
				errorcnt++;
			}
			type_override = 1;
			break;
		case OPT_PUBKEY:
			if (vb2_packed_key_read(&show_option.pkey, optarg)) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_HELP:
			print_help(argc, argv);
			return !!errorcnt;

		case '?':
			if (optopt)
				fprintf(stderr, "Unrecognized option: -%c\n",
					optopt);
			else
				fprintf(stderr, "Unrecognized option\n");
			errorcnt++;
			break;
		case ':':
			fprintf(stderr, "Missing argument to -%c\n", optopt);
			errorcnt++;
			break;
		case 0:				/* handled option */
			break;
		default:
			DIE;
		}
	}

	if (errorcnt) {
		print_help(argc, argv);
		return 1;
	}

	if (argc - optind < 1) {
		fprintf(stderr, "ERROR: missing input filename\n");
		print_help(argc, argv);
		return 1;
	}

	if (show_option.t_flag) {
		for (i = optind; i < argc; i++)
			errorcnt += show_type(argv[i]);
		goto done;
	}

	for (i = optind; i < argc; i++) {
		infile = argv[i];
		ifd = open(infile, O_RDONLY);
		if (ifd < 0) {
			errorcnt++;
			fprintf(stderr, "Can't open %s: %s\n",
				infile, strerror(errno));
			continue;
		}

		if (0 != futil_map_file(ifd, MAP_RO, &buf, &len)) {
			errorcnt++;
			goto boo;
		}

		/* Allow the user to override the type */
		if (type_override)
			type = show_option.type;
		else
			type = futil_file_type_buf(buf, len);

		errorcnt += futil_file_type_show(type, infile, buf, len);

		errorcnt += futil_unmap_file(ifd, MAP_RO, buf, len);
boo:
		if (close(ifd)) {
			errorcnt++;
			fprintf(stderr, "Error when closing %s: %s\n",
				infile, strerror(errno));
		}
	}

done:
	if (show_option.k)
		free(show_option.k);
	if (show_option.fv)
		free(show_option.fv);

	return !!errorcnt;
}

DECLARE_FUTIL_COMMAND(show, do_show, VBOOT_VERSION_ALL,
		      "Display the content of various binary components");

static int do_verify(int argc, char *argv[])
{
	show_option.strict = 1;
	return do_show(argc, argv);
}

DECLARE_FUTIL_COMMAND(verify, do_verify,
		      VBOOT_VERSION_ALL,
		      "Verify the signatures of various binary components");
