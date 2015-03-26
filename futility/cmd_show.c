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

#include "bmpblk_header.h"
#include "file_type.h"
#include "fmap.h"
#include "futility.h"
#include "gbb_header.h"
#include "host_common.h"
#include "traversal.h"
#include "util_misc.h"
#include "vb1_helper.h"
#include "vboot_common.h"

/* Local structure for args, etc. */
static struct local_data_s {
	VbPublicKey *k;
	uint8_t *fv;
	uint64_t fv_size;
	uint32_t padding;
	int strict;
	int t_flag;
} option = {
	.padding = 65536,
};

/* Stuff for BIOS images. */

/* Forward declarations */
static int fmap_fw_main(const char *name, uint8_t *buf, uint32_t len,
			void *data);

/* These are the functions we'll call for each FMAP area. */
static int (*fmap_func[])(const char *name, uint8_t *buf, uint32_t len,
			void *data) = {
	ft_show_gbb,
	fmap_fw_main,
	fmap_fw_main,
	ft_show_fw_preamble,
	ft_show_fw_preamble,
};
BUILD_ASSERT(ARRAY_SIZE(fmap_func) == NUM_BIOS_COMPONENTS);

/* Where is the component we're looking at? */
struct bios_area_s {
	uint32_t offset;			/* to avoid pointer math */
	uint8_t *buf;
	uint32_t len;
	uint32_t is_valid;
};

/* When looking at the FMAP areas, we need to gather some state for later. */
struct show_state_s {
	/* Current component */
	enum bios_component c;
	/* Other activites, possibly before or after the current one */
	struct bios_area_s area[NUM_BIOS_COMPONENTS];
	struct bios_area_s recovery_key;
	struct bios_area_s rootkey;
};

static void show_key(VbPublicKey *pubkey, const char *sp)
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
	show_key(pubkey, "  ");

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

int ft_show_gbb(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	GoogleBinaryBlockHeader *gbb = (GoogleBinaryBlockHeader *)buf;
	struct show_state_s *state = (struct show_state_s *)data;
	VbPublicKey *pubkey;
	BmpBlockHeader *bmp;
	int retval = 0;
	uint32_t maxlen = 0;

	if (!len) {
		printf("GBB header:              %s <invalid>\n", name);
		return 1;
	}

	/* It looks like a GBB or we wouldn't be called. */
	if (!futil_valid_gbb_header(gbb, len, &maxlen))
		retval = 1;

	printf("GBB header:              %s\n", name);
	printf("  Version:               %d.%d\n",
	       gbb->major_version, gbb->minor_version);
	printf("  Flags:                 0x%08x\n", gbb->flags);
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
	printf("  HWID:                  %s\n", buf + gbb->hwid_offset);
	print_hwid_digest(gbb, "     digest:             ", "\n");

	pubkey = (VbPublicKey *)(buf + gbb->rootkey_offset);
	if (PublicKeyLooksOkay(pubkey, gbb->rootkey_size)) {
		if (state) {
			state->rootkey.offset =
				state->area[BIOS_FMAP_GBB].offset +
				gbb->rootkey_offset;
			state->rootkey.buf = buf + gbb->rootkey_offset;
			state->rootkey.len = gbb->rootkey_size;
			state->rootkey.is_valid = 1;
		}
		printf("  Root Key:\n");
		show_key(pubkey, "    ");
	} else {
		retval = 1;
		printf("  Root Key:              <invalid>\n");
	}

	pubkey = (VbPublicKey *)(buf + gbb->recovery_key_offset);
	if (PublicKeyLooksOkay(pubkey, gbb->recovery_key_size)) {
		if (state) {
			state->recovery_key.offset =
				state->area[BIOS_FMAP_GBB].offset +
				gbb->recovery_key_offset;
			state->recovery_key.buf = buf +
				gbb->recovery_key_offset;
			state->recovery_key.len = gbb->recovery_key_size;
			state->recovery_key.is_valid = 1;
		}
		printf("  Recovery Key:\n");
		show_key(pubkey, "    ");
	} else {
		retval = 1;
		printf("  Recovery Key:          <invalid>\n");
	}

	bmp = (BmpBlockHeader *)(buf + gbb->bmpfv_offset);
	if (0 != memcmp(bmp, BMPBLOCK_SIGNATURE, BMPBLOCK_SIGNATURE_SIZE)) {
		printf("  BmpBlock:              <invalid>\n");
		/* We don't support older BmpBlock formats, so we can't
		 * be strict about this. */
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

	if (!retval && state)
		state->area[BIOS_FMAP_GBB].is_valid = 1;

	return retval;
}

int ft_show_keyblock(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	VbKeyBlockHeader *block = (VbKeyBlockHeader *)buf;
	VbPublicKey *sign_key = option.k;
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

	if (option.strict && (!sign_key || !good_sig))
		retval = 1;

	show_keyblock(block, name, !!sign_key, good_sig);

	return retval;
}

/*
 * This handles FW_MAIN_A and FW_MAIN_B while processing a BIOS image.
 *
 * The data is just the RW firmware blob, so there's nothing useful to show
 * about it. We'll just mark it as present so when we encounter corresponding
 * VBLOCK area, we'll have this to verify.
 */
static int fmap_fw_main(const char *name, uint8_t *buf, uint32_t len,
			void *data)
{
	struct show_state_s *state = (struct show_state_s *)data;

	if (!len) {
		printf("Firmware body:           %s <invalid>\n", name);
		return 1;
	}

	printf("Firmware body:           %s\n", name);
	printf("  Offset:                0x%08x\n",
	       state->area[state->c].offset);
	printf("  Size:                  0x%08x\n", len);

	state->area[state->c].is_valid = 1;

	return 0;
}

int ft_show_fw_preamble(const char *name, uint8_t *buf, uint32_t len,
			void *data)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)buf;
	struct show_state_s *state = (struct show_state_s *)data;
	VbPublicKey *sign_key = option.k;
	uint8_t *fv_data = option.fv;
	uint64_t fv_size = option.fv_size;
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

	if (option.strict && (!sign_key || !good_sig))
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
		if (option.strict)
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
		if (option.strict)
			retval = 1;
	}

	return retval;
}

int ft_show_bios(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	FmapHeader *fmap;
	FmapAreaHeader *ah = 0;
	char ah_name[FMAP_NAMELEN + 1];
	int i;
	int retval = 0;
	struct show_state_s state;

	memset(&state, 0, sizeof(state));

	printf("BIOS:                    %s\n", name);

	/* We've already checked, so we know this will work. */
	fmap = fmap_find(buf, len);
	for (i = 0; i < NUM_BIOS_COMPONENTS; i++) {
		/* We know one of these will work, too */
		if (fmap_find_by_name(buf, len, fmap,
				      bios_area[i].name, &ah) ||
		    fmap_find_by_name(buf, len, fmap,
				      bios_area[i].oldname, &ah)) {
			/* But the file might be truncated */
			fmap_limit_area(ah, len);
			/* The name is not necessarily null-terminated */
			snprintf(ah_name, sizeof(ah_name), "%s", ah->area_name);

			/* Update the state we're passing around */
			state.c = i;
			state.area[i].offset = ah->area_offset;
			state.area[i].buf = buf + ah->area_offset;
			state.area[i].len = ah->area_size;

			Debug("%s() showing FMAP area %d (%s),"
			      " offset=0x%08x len=0x%08x\n",
			      __func__, i, ah_name,
			      ah->area_offset, ah->area_size);

			/* Go look at it. */
			if (fmap_func[i])
				retval += fmap_func[i](ah_name,
						       state.area[i].buf,
						       state.area[i].len,
						       &state);
		}
	}

	return retval;
}

int ft_show_kernel_preamble(const char *name, uint8_t *buf, uint32_t len,
			    void *data)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)buf;
	VbPublicKey *sign_key = option.k;
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

	if (option.strict && (!sign_key || !good_sig))
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
	if (option.fv) {
		/* It's in a separate file, which we've already read in */
		kernel_blob = option.fv;
		kernel_size = option.fv_size;
	} else if (len > option.padding) {
		/* It should be at an offset within the input file. */
		kernel_blob = buf + option.padding;
		kernel_size = len - option.padding;
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
	OPT_HELP,
};

static const char usage[] = "\n"
	"Usage:  " MYNAME " %s [OPTIONS] FILE [...]\n"
	"\n"
	"Where FILE could be a\n"
	"\n"
	"%s"
	"  keyblock (.keyblock)\n"
	"  firmware preamble signature (VBLOCK_A/B)\n"
	"  firmware image (bios.bin)\n"
	"  kernel partition (/dev/sda2, /dev/mmcblk0p2)\n"
	"\n"
	"Options:\n"
	"  -t                               Just show the type of each file\n"
	"  -k|--publickey   FILE"
	"            Use this public key for validation\n"
	"  -f|--fv          FILE            Verify this payload (FW_MAIN_A/B)\n"
	"  --pad            NUM             Kernel vblock padding size\n"
	"%s"
	"\n";

static void print_help(int argc, char *argv[])
{
	if (strcmp(argv[0], "verify"))
		printf(usage, argv[0],
		       "  public key (.vbpubk)\n",
		       "  --strict                         "
		       "Fail unless all signatures are valid\n");
	else
		printf(usage, argv[0], "",
		       "\nIt will fail unless all signatures are valid\n");
}

static const struct option long_opts[] = {
	/* name    hasarg *flag val */
	{"publickey",   1, 0, 'k'},
	{"fv",          1, 0, 'f'},
	{"pad",         1, NULL, OPT_PADDING},
	{"strict",      0, &option.strict, 1},
	{"help",        0, NULL, OPT_HELP},
	{NULL, 0, NULL, 0},
};
static char *short_opts = ":f:k:t";


static void show_type(char *filename)
{
	enum futil_file_err err;
	enum futil_file_type type;
	err = futil_file_type(filename, &type);
	switch (err) {
	case FILE_ERR_NONE:
		printf("%s:\t%s\n", filename, futil_file_type_name(type));
		break;
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
}

static int do_show(int argc, char *argv[])
{
	char *infile = 0;
	int ifd, i;
	int errorcnt = 0;
	enum futil_file_type type;
	uint8_t *buf;
	uint32_t len;
	char *e = 0;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, short_opts, long_opts, 0)) != -1) {
		switch (i) {
		case 'f':
			option.fv = ReadFile(optarg, &option.fv_size);
			if (!option.fv) {
				fprintf(stderr, "Error reading %s: %s\n",
					optarg, strerror(errno));
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
		case 't':
			option.t_flag = 1;
			break;
		case OPT_PADDING:
			option.padding = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --padding \"%s\"\n", optarg);
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

	if (option.t_flag) {
		for (i = optind; i < argc; i++)
			show_type(argv[i]);
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
	if (option.k)
		free(option.k);
	if (option.fv)
		free(option.fv);

	return !!errorcnt;
}

DECLARE_FUTIL_COMMAND(show, do_show, VBOOT_VERSION_ALL,
		      "Display the content of various binary components");

static int do_verify(int argc, char *argv[])
{
	option.strict = 1;
	return do_show(argc, argv);
}

DECLARE_FUTIL_COMMAND(verify, do_verify,
		      VBOOT_VERSION_ALL,
		      "Verify the signatures of various binary components");
