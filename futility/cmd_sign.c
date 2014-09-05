/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bmpblk_header.h"
#include "fmap.h"
#include "futility.h"
#include "gbb_header.h"
#include "host_common.h"
#include "traversal.h"
#include "util_misc.h"
#include "vboot_common.h"

/* Local values for cb_area_s._flags */
enum callback_flags {
	AREA_IS_VALID =     0x00000001,
};

/* Local structure for args, etc. */
struct local_data_s {
	VbPrivateKey *signprivate;
	VbKeyBlockHeader *keyblock;
	VbPublicKey *kernel_subkey;
	VbPrivateKey *devsignprivate;
	VbKeyBlockHeader *devkeyblock;
	uint32_t version;
	uint32_t flags;
	int flags_specified;
	char *loemdir;
	char *loemid;
} option = {
	.version = 1,
};


int futil_cb_sign_bogus(struct futil_traverse_state_s *state)
{
	fprintf(stderr, "Don't know how to sign %s\n", state->name);
	return 1;
}

int futil_cb_sign_notyet(struct futil_traverse_state_s *state)
{
	fprintf(stderr, "Signing %s is not yet implemented\n", state->name);
	return 1;
}

/*
 * This handles FW_MAIN_A and FW_MAIN_B while processing a BIOS image.
 *
 * The data in state->my_area is just the RW firmware blob, so there's nothing
 * useful to show about it. We'll just mark it as present so when we encounter
 * corresponding VBLOCK area, we'll have this to verify.
 */
int futil_cb_sign_fw_main(struct futil_traverse_state_s *state)
{
	state->my_area->_flags |= AREA_IS_VALID;
	return 0;
}


int futil_cb_sign_fw_preamble(struct futil_traverse_state_s *state)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)state->my_area->buf;
	uint32_t len = state->my_area->len;

	/* We don't (yet) handle standalone VBLOCKs */
	if (state->component == CB_FW_PREAMBLE)
		return futil_cb_sign_notyet(state);

	/*
	 * If we have a valid keyblock and fw_preamble, then we can use them to
	 * determine the size of the firmware body. Otherwise, we'll have to
	 * just sign the whole region.
	 */
	if (VBOOT_SUCCESS != KeyBlockVerify(key_block, len, NULL, 1)) {
		fprintf(stderr, "Warning: %s keyblock is invalid. "
			"Signing the entire FW FMAP region...\n",
		       state->name);
		goto whatever;
	}

	RSAPublicKey *rsa = PublicKeyToRSA(&key_block->data_key);
	if (!rsa) {
		fprintf(stderr, "Warning: %s public key is invalid. "
			"Signing the entire FW FMAP region...\n",
			state->name);
		goto whatever;
	}
	uint32_t more = key_block->key_block_size;
	VbFirmwarePreambleHeader *preamble =
		(VbFirmwarePreambleHeader *)(state->my_area->buf + more);
	uint32_t fw_size = preamble->body_signature.data_size;
	struct cb_area_s *fw_body_area = 0;

	switch (state->component) {
	case CB_FMAP_VBLOCK_A:
		fw_body_area = &state->cb_area[CB_FMAP_FW_MAIN_A];
		/* Preserve the flags if they're not specified */
		if (!option.flags_specified)
			option.flags = preamble->flags;
		break;
	case CB_FMAP_VBLOCK_B:
		fw_body_area = &state->cb_area[CB_FMAP_FW_MAIN_B];
		break;
	default:
		DIE;
	}

	if (fw_size > fw_body_area->len) {
		fprintf(stderr,
			"%s says the firmware is larger than we have\n",
		       state->name);
		return 1;
	}

	/* Update the firmware size */
	fw_body_area->len = fw_size;

whatever:
	state->my_area->_flags |= AREA_IS_VALID;

	return 0;
}

int futil_cb_sign_begin(struct futil_traverse_state_s *state)
{
	if (state->in_type == FILE_TYPE_UNKNOWN) {
		fprintf(stderr, "Unable to determine type of %s\n",
			state->in_filename);
		return 1;
	}

	return 0;
}

static int write_new_preamble(struct cb_area_s *vblock,
			      struct cb_area_s *fw_body,
			      VbPrivateKey *signkey,
			      VbKeyBlockHeader *keyblock)
{
	VbSignature *body_sig;
	VbFirmwarePreambleHeader *preamble;

	body_sig = CalculateSignature(fw_body->buf, fw_body->len, signkey);
	if (!body_sig) {
		fprintf(stderr, "Error calculating body signature\n");
		return 1;
	}

	preamble = CreateFirmwarePreamble(option.version,
					  option.kernel_subkey,
					  body_sig,
					  signkey,
					  option.flags);
	if (!preamble) {
		fprintf(stderr, "Error creating firmware preamble.\n");
		free(body_sig);
		return 1;
	}

	/* Write the new keyblock */
	uint32_t more = keyblock->key_block_size;
	memcpy(vblock->buf, keyblock, more);
	/* and the new preamble */
	memcpy(vblock->buf + more, preamble, preamble->preamble_size);

	free(preamble);
	free(body_sig);

	return 0;
}

static int write_loem(const char *ab, struct cb_area_s *vblock)
{
	char filename[PATH_MAX];
	int n;
	n = snprintf(filename, sizeof(filename), "%s/vblock_%s.%s",
		     option.loemdir ? option.loemdir : ".",
		     ab, option.loemid);
	if (n >= sizeof(filename)) {
		fprintf(stderr, "LOEM args produce bogus filename\n");
		return 1;
	}

	FILE *fp = fopen(filename, "w");
	if (!fp) {
		fprintf(stderr, "Can't open %s for writing: %s\n",
			filename, strerror(errno));
		return 1;
	}

	if (1 != fwrite(vblock->buf, vblock->len, 1, fp)) {
		fprintf(stderr, "Can't write to %s: %s\n",
			filename, strerror(errno));
		fclose(fp);
		return 1;
	}
	if (fclose(fp)) {
		fprintf(stderr, "Failed closing loem output: %s\n",
			strerror(errno));
		return 1;
	}

	return 0;
}

int futil_cb_sign_end(struct futil_traverse_state_s *state)
{
	struct cb_area_s *vblock_a = &state->cb_area[CB_FMAP_VBLOCK_A];
	struct cb_area_s *vblock_b = &state->cb_area[CB_FMAP_VBLOCK_B];
	struct cb_area_s *fw_a = &state->cb_area[CB_FMAP_FW_MAIN_A];
	struct cb_area_s *fw_b = &state->cb_area[CB_FMAP_FW_MAIN_B];
	int retval = 0;

	if (state->errors ||
	    !(vblock_a->_flags & AREA_IS_VALID) ||
	    !(vblock_b->_flags & AREA_IS_VALID) ||
	    !(fw_a->_flags & AREA_IS_VALID) ||
	    !(fw_b->_flags & AREA_IS_VALID)) {
		fprintf(stderr, "Something's wrong. Not changing anything\n");
		return 1;
	}

	/* Do A & B differ ? */
	if (fw_a->len != fw_b->len ||
	    memcmp(fw_a->buf, fw_b->buf, fw_a->len)) {
		/* Yes, must use DEV keys for A */
		if (!option.devsignprivate || !option.devkeyblock) {
			fprintf(stderr,
				"FW A & B differ. DEV keys are required.\n");
			return 1;
		}
		retval |= write_new_preamble(vblock_a, fw_a,
					     option.devsignprivate,
					     option.devkeyblock);
	} else {
		retval |= write_new_preamble(vblock_a, fw_a,
					     option.signprivate,
					     option.keyblock);
	}

	/* FW B is always normal keys */
	retval |= write_new_preamble(vblock_b, fw_b,
				     option.signprivate,
				     option.keyblock);




	if (option.loemid) {
		retval |= write_loem("A", vblock_a);
		retval |= write_loem("B", vblock_b);
	}

	return retval;
}

static const char usage[] = "\n"
	"Usage:  " MYNAME " %s [OPTIONS] FILE [OUTFILE]\n"
	"\n"
	"[Re]Sign the specified BIOS image\n"
	"\n"
	"Required OPTIONS:\n"
	"  -s|--signprivate FILE.vbprivk    The private firmware data key\n"
	"  -b|--keyblock    FILE.keyblock   The keyblock containing the\n"
	"                                     public firmware data key\n"
	"  -k|--kernelkey   FILE.vbpubk     The public kernel subkey\n"
	"\n"
	"These are required if the A and B firmware differ:\n"
	"  -S|--devsign     FILE.vbprivk    The DEV private firmware data key\n"
	"  -B|--devkeyblock FILE.keyblock   The keyblock containing the\n"
	"                                     DEV public firmware data key\n"
	"\n"
	"Optional OPTIONS:\n"
	"  -v|--version     NUM             The firmware version number"
	" (default %d)\n"
	"  -f|--flags       NUM             The preamble flags value"
	" (default is\n"
	"                                     unchanged, or 0 if unknown)\n"
	"  -d|--loemdir     DIR             Local OEM output vblock directory\n"
	"  -l|--loemid      STRING          Local OEM vblock suffix\n"
	"\n";

static void help_and_quit(const char *prog)
{
	fprintf(stderr, usage, prog, option.version);
	exit(1);
}

static const struct option long_opts[] = {
	/* name    hasarg *flag  val */
	{"signprivate", 1, NULL, 's'},
	{"keyblock",    1, NULL, 'b'},
	{"kernelkey",   1, NULL, 'k'},
	{"devsign",     1, NULL, 'S'},
	{"devkeyblock", 1, NULL, 'B'},
	{"version",     1, NULL, 'v'},
	{"flags",       1, NULL, 'f'},
	{"loemdir",     1, NULL, 'd'},
	{"loemid",      1, NULL, 'l'},
	{NULL,          0, NULL, 0},
};
static char *short_opts = ":s:b:k:S:B:v:f:d:l:";

static int do_sign(int argc, char *argv[])
{
	char *infile = 0;
	char *outfile = 0;
	int fd, i;
	int errorcnt = 0;
	struct futil_traverse_state_s state;
	char *e = 0;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, short_opts, long_opts, 0)) != -1) {
		switch (i) {
		case 's':
			option.signprivate = PrivateKeyRead(optarg);
			if (!option.signprivate) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'b':
			option.keyblock = KeyBlockRead(optarg);
			if (!option.keyblock) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'k':
			option.kernel_subkey = PublicKeyRead(optarg);
			if (!option.kernel_subkey) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'S':
			option.devsignprivate = PrivateKeyRead(optarg);
			if (!option.devsignprivate) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'B':
			option.devkeyblock = KeyBlockRead(optarg);
			if (!option.devkeyblock) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'v':
			option.version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --version \"%s\"\n", optarg);
				errorcnt++;
			}
			break;

		case 'f':
			option.flags_specified = 1;
			option.flags = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --flags \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case 'd':
			option.loemdir = optarg;
			break;
		case 'l':
			option.loemid = optarg;
			break;

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
		default:
			DIE;
		}
	}

	if (!option.signprivate) {
		fprintf(stderr,
			"Missing required private firmware data key\n");
		errorcnt++;
	}

	if (!option.keyblock) {
		fprintf(stderr,
			"Missing required keyblock\n");
		errorcnt++;
	}

	if (!option.kernel_subkey) {
		fprintf(stderr,
			"Missing required kernel subkey\n");
		errorcnt++;
	}

	if (errorcnt)
		help_and_quit(argv[0]);

	switch (argc - optind) {
	case 2:
		infile = argv[optind++];
		outfile = argv[optind++];
		copy_file_or_die(infile, outfile);
		break;
	case 1:
		/* Stomping right on it. Errors will leave it garbled. */
		/* TODO: Use a tempfile (mkstemp) for normal files. */
		infile = argv[optind++];
		outfile = infile;
		break;
	case 0:
		fprintf(stderr, "ERROR: missing input filename\n");
		help_and_quit(argv[0]);
		break;
	default:
		fprintf(stderr, "ERROR: too many arguments left over\n");
		help_and_quit(argv[0]);
	}


	fd = open(outfile, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Can't open %s: %s\n",
			outfile, strerror(errno));
		return 1;
	}

	memset(&state, 0, sizeof(state));
	state.in_filename = outfile ? outfile : "<none>";
	state.op = FUTIL_OP_SIGN;

	errorcnt += futil_traverse(fd, &state, 1);

	if (close(fd)) {
		errorcnt++;
		fprintf(stderr, "Error when closing %s: %s\n",
			outfile, strerror(errno));
	}

	if (option.signprivate)
		free(option.signprivate);
	if (option.keyblock)
		free(option.keyblock);
	if (option.kernel_subkey)
		free(option.kernel_subkey);

	return !!errorcnt;
}

DECLARE_FUTIL_COMMAND(sign, do_sign, "[Re]Sign a BIOS image");
