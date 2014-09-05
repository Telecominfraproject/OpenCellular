/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
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
static struct local_data_s {
	VbPublicKey *k;
	uint8_t *f;
	uint64_t f_size;
} option;

static void show_key(VbPublicKey *pubkey, const char *sp)
{
	printf("%sAlgorithm:           %" PRIu64 " %s\n", sp,pubkey->algorithm,
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
	printf("Key block:               %s\n", name);
	printf("  Size:                  %" PRIu64 "\n",
	       key_block->key_block_size);
	printf("  Signature              %s\n",
	       sign_key ? (good_sig ? "valid" : "invalid" ) : "ignored");
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

int futil_cb_show_key(struct futil_traverse_state_s *state)
{
	VbPublicKey *pubkey = (VbPublicKey *)state->my_area->buf;

	if (!PublicKeyLooksOkay(pubkey, state->my_area->len)) {
		printf("%s looks bogus\n", state->name);
		return 1;
	}

	printf("Public Key file:       %s\n", state->in_filename);
	show_key(pubkey, "  ");

	state->my_area->_flags |= AREA_IS_VALID;
	return 0;
}

int futil_cb_show_gbb(struct futil_traverse_state_s *state)
{
	uint8_t *buf = state->my_area->buf;
	uint32_t len = state->my_area->len;
	GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader *)buf;
	VbPublicKey *pubkey;
	BmpBlockHeader *bmp;
	int retval = 0;
	uint32_t maxlen = 0;

	/* It looks like a GBB or we wouldn't be called. */
	if (!futil_valid_gbb_header(gbb, len, &maxlen))
		retval = 1;

	printf("GBB header:              %s\n",
	       state->component == CB_GBB ? state->in_filename : state->name);
	printf("  Version:               %d.%d\n",
	       gbb->major_version, gbb->minor_version);
	printf("  Flags:                 0x08%x\n", gbb->flags);
	printf("  Regions:                 offset       size\n");
	printf("    hwid                 0x%08x   0x%08x\n",
	       gbb->hwid_offset, gbb->hwid_size);
	printf("    bmpvf                0x%08x   0x%08x\n",
	       gbb->bmpfv_offset, gbb->bmpfv_size);
	printf("    rootkey              0x%08x   0x%08x\n",
	       gbb->rootkey_offset, gbb->rootkey_size);
	printf("    recovery_key         0x%08x   0x%08x\n",
	       gbb->recovery_key_offset, gbb->recovery_key_size);

	printf("  Size:                  0x%08x / 0x%08x%s\n",
	       maxlen, len, maxlen > len ? "  (not enough)" : "");

	if (retval) {
		printf("GBB header is invalid, ignoring content\n");
		return 1;
	}

	printf("GBB content:\n");
	printf("  HWID:                  %s\n",
	       (const char *)(buf + gbb->hwid_offset));

	pubkey = (VbPublicKey *)(buf + gbb->rootkey_offset);
	if (PublicKeyLooksOkay(pubkey, gbb->rootkey_size)) {
		state->rootkey.offset = state->my_area->offset +
			gbb->rootkey_offset;
		state->rootkey.buf = buf + gbb->rootkey_offset;
		state->rootkey.len = gbb->rootkey_size;
		state->rootkey._flags |= AREA_IS_VALID;
		printf("  Root Key:\n");
		show_key(pubkey, "    ");
	} else {
		retval = 1;
		printf("  Root Key:              <invalid>\n");
	}

	pubkey = (VbPublicKey *)(buf + gbb->recovery_key_offset);
	if (PublicKeyLooksOkay(pubkey, gbb->recovery_key_size)) {
		state->recovery_key.offset = state->my_area->offset +
			gbb->recovery_key_offset;
		state->recovery_key.buf = buf + gbb->recovery_key_offset;
		state->recovery_key.len = gbb->recovery_key_size;
		state->recovery_key._flags |= AREA_IS_VALID;
		printf("  Recovery Key:\n");
		show_key(pubkey, "    ");
	} else {
		retval = 1;
		printf("  Recovery Key:          <invalid>\n");
	}

	bmp = (BmpBlockHeader *)(buf + gbb->bmpfv_offset);
	if (0 != memcmp(bmp, BMPBLOCK_SIGNATURE, BMPBLOCK_SIGNATURE_SIZE)) {
		printf("  BmpBlock:              <invalid>\n");
		/* We don't support old formats, so it's not always an error */
		/* TODO: Add a --strict option to make this fatal? */
	} else {
		printf("  BmpBlock:\n");
		printf("    Version:             %d.%d\n",
		       bmp->major_version, bmp->minor_version);
		printf("    Localizations:       %d\n",
		       bmp->number_of_localizations);
		printf("    Screen layouts:      %d\n",
		       bmp->number_of_screenlayouts);
		printf("    Image infos:         %d\n",
		       bmp->number_of_imageinfos);
	}

	if (!retval)
		state->my_area->_flags |= AREA_IS_VALID;

	return retval;
}

int futil_cb_show_keyblock(struct futil_traverse_state_s *state)
{
	VbKeyBlockHeader *block = (VbKeyBlockHeader *)state->my_area->buf;
	VbPublicKey *sign_key = option.k;
	int good_sig = 0;

	/* Check the hash only first */
	if (0 != KeyBlockVerify(block, state->my_area->len, NULL, 1)) {
		printf("%s is invalid\n", state->name);
		return 1;
	}

	/* Check the signature if we have one */
	if (sign_key && VBOOT_SUCCESS ==
	    KeyBlockVerify(block, state->my_area->len, sign_key, 0))
		good_sig = 1;

	show_keyblock(block, state->in_filename, !!sign_key, good_sig);

	state->my_area->_flags |= AREA_IS_VALID;

	return 0;
}

/*
 * This handles FW_MAIN_A and FW_MAIN_B while processing a BIOS image.
 *
 * The data in state->my_area is just the RW firmware blob, so there's nothing
 * useful to show about it. We'll just mark it as present so when we encounter
 * corresponding VBLOCK area, we'll have this to verify.
 */
int futil_cb_show_fw_main(struct futil_traverse_state_s *state)
{
	printf("Firmware body:           %s\n", state->name);
	printf("  Offset:                0x%08x\n", state->my_area->offset);
	printf("  Size:                  0x%08x\n", state->my_area->len);

	state->my_area->_flags |= AREA_IS_VALID;

	return 0;
}

int futil_cb_show_fw_preamble(struct futil_traverse_state_s *state)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)state->my_area->buf;
	uint32_t len = state->my_area->len;
	VbPublicKey *sign_key = 0;
	uint8_t *fv_data = 0;
	uint64_t fv_size = 0;
	struct cb_area_s *fw_body_area = 0;
	int good_sig = 0;

	/* Check the hash... */
	if (VBOOT_SUCCESS != KeyBlockVerify(key_block, len, NULL, 1)) {
		printf("%s keyblock component is invalid\n", state->name);
		return 1;
	}

	switch (state->component) {
	case CB_FMAP_VBLOCK_A:
		/* BIOS should have a rootkey in the GBB */
		if (state->rootkey._flags & AREA_IS_VALID)
			sign_key = (VbPublicKey *)state->rootkey.buf;
		/* And we should have already seen the firmware body */
		fw_body_area = &state->cb_area[CB_FMAP_FW_MAIN_A];
		break;
	case CB_FMAP_VBLOCK_B:
		/* BIOS should have a rootkey in the GBB */
		if (state->rootkey._flags & AREA_IS_VALID)
			sign_key = (VbPublicKey *)state->rootkey.buf;
		/* And we should have already seen the firmware body */
		fw_body_area = &state->cb_area[CB_FMAP_FW_MAIN_B];
		break;
	case CB_FW_PREAMBLE:
		/* We'll have to get a signature and body from elsewhere */
		sign_key = option.k;
		fv_data = option.f;
		fv_size = option.f_size;
		break;
	default:
		DIE;
	}

	/* If we have a key, check the signature too */
	if (sign_key && VBOOT_SUCCESS ==
	    KeyBlockVerify(key_block, len, sign_key, 0))
		good_sig = 1;

	show_keyblock(key_block,
		      state->component == CB_FW_PREAMBLE
		      ? state->in_filename : state->name,
		      !!sign_key, good_sig);

	RSAPublicKey *rsa = PublicKeyToRSA(&key_block->data_key);
	if (!rsa) {
		VbExError("Error parsing data key in %s\n", state->name);
		return 1;
	}
	uint32_t more = key_block->key_block_size;
	VbFirmwarePreambleHeader *preamble =
		(VbFirmwarePreambleHeader *)(state->my_area->buf + more);

	if (VBOOT_SUCCESS != VerifyFirmwarePreamble(preamble,
						    len - more, rsa)) {
		printf("%s is invalid\n", state->name);
		return 1;
	}

	uint32_t flags = VbGetFirmwarePreambleFlags(preamble);
	printf("Preamble:\n");
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
	printf("  Kernel key version:    %" PRIu64 "\n",
	       kernel_subkey->key_version);
	printf("  Kernel key sha1sum:    ");
	PrintPubKeySha1Sum(kernel_subkey);
	printf("\n");
	printf("  Firmware body size:    %" PRIu64 "\n",
	       preamble->body_signature.data_size);
	printf("  Preamble flags:        %" PRIu32 "\n", flags);


	if (flags & VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL) {
		printf ("Preamble requests USE_RO_NORMAL;"
			" skipping body verification.\n");
		goto done;
	}

	/* We'll need to get the firmware body from somewhere... */
	if (fw_body_area && (fw_body_area->_flags & AREA_IS_VALID))
	{
		fv_data = fw_body_area->buf;
		fv_size = fw_body_area->len;
	}

	if (!fv_data) {
		printf("No firmware body available to verify.\n");
		/* TODO: Add a --strict option to make this fatal? */
		return 0;
	}

	if (VBOOT_SUCCESS !=
	    VerifyData(fv_data, fv_size, &preamble->body_signature, rsa)) {
		VbExError("Error verifying firmware body.\n");
		return 1;
	}

done:
	/* Can't trust the BIOS unless everything is signed,
	 * but standalone files are okay. */
	if ((state->component == CB_FW_PREAMBLE) ||
	    (sign_key && good_sig)) {
		if (!(flags & VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL))
			printf("Body verification succeeded.\n");
		state->my_area->_flags |= AREA_IS_VALID;
	} else {
		printf("Seems legit, but the signature is unverified.\n");
		/* TODO: Add a --strict option to make this fatal? */
	}

	return 0;
}

int futil_cb_show_begin(struct futil_traverse_state_s *state)
{
	switch (state->in_type) {
	case FILE_TYPE_UNKNOWN:
		fprintf(stderr, "Unable to determine type of %s\n",
			state->in_filename);
		return 1;

	case FILE_TYPE_BIOS_IMAGE:
	case FILE_TYPE_OLD_BIOS_IMAGE:
		printf("BIOS:                    %s\n", state->in_filename);
		break;

	default:
		break;
	}
	return 0;
}

static void help_and_quit(const char *prog)
{
	fprintf(stderr, "\n"
		"Usage:  " MYNAME " %s [OPTIONS] FILE\n"
		"\n"
		"Display the contents of the given FILE\n"
		"\n"
		"Options:\n"
		"  -k|--publickey FILE    Use this public key for validation\n"
		"  -f|--fv FILE           Use this firmware blob where needed\n"
		"\n", prog);
	exit(1);
}

static const struct option long_opts[] = {
	/* name    hasarg *flag val */
	{"publickey", 1, 0, 'k'},
	{"fv", 1, 0, 'f'},
	{NULL, 0, NULL, 0},
};
static char *short_opts = ":f:k:";

static int do_show(int argc, char *argv[])
{
	char *infile = 0;
	int ifd, i;
	int errorcnt = 0;
	struct futil_traverse_state_s state;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, short_opts, long_opts, 0)) != -1) {
		switch (i) {
		case 'f':
			option.f = ReadFile(optarg, &option.f_size);
			if (!option.f) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'k':
			option.k = PublicKeyRead(optarg);
			if (!option.k) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
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

	if (errorcnt)
		help_and_quit(argv[0]);

	if (argc - optind < 1) {
		fprintf(stderr, "ERROR: missing input filename\n");
		help_and_quit(argv[0]);
	}

	for (i = optind; i < argc; i++) {
		infile = argv[i];
		ifd = open(infile, O_RDONLY);
		if (ifd < 0) {
			errorcnt++;
			fprintf(stderr, "Can't open %s: %s\n",
				infile, strerror(errno));
			return 1;
		}

		memset(&state, 0, sizeof(state));
		state.in_filename = infile ? infile : "<none>";
		state.op = FUTIL_OP_SHOW;

		errorcnt += futil_traverse(ifd, &state, 0);

		if (close(ifd)) {
			errorcnt++;
			fprintf(stderr, "Error when closing %s: %s\n",
				infile, strerror(errno));
		}
	}

	if (option.k)
		free(option.k);
	if (option.f)
		free(option.f);

	return !!errorcnt;
}

DECLARE_FUTIL_COMMAND(show, do_show, "Display what we know about a file");
