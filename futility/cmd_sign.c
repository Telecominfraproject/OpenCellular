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
#include "file_type.h"
#include "fmap.h"
#include "futility.h"
#include "gbb_header.h"
#include "host_common.h"
#include "kernel_blob.h"
#include "traversal.h"
#include "util_misc.h"
#include "vb1_helper.h"
#include "vboot_common.h"

/* Local values for cb_area_s._flags */
enum callback_flags {
	AREA_IS_VALID =     0x00000001,
};

/* Local structure for args, etc. */
static struct local_data_s {
	VbPrivateKey *signprivate;
	VbKeyBlockHeader *keyblock;
	VbPublicKey *kernel_subkey;
	VbPrivateKey *devsignprivate;
	VbKeyBlockHeader *devkeyblock;
	uint32_t version;
	int version_specified;
	uint32_t flags;
	int flags_specified;
	char *loemdir;
	char *loemid;
	uint8_t *bootloader_data;
	uint64_t bootloader_size;
	uint8_t *config_data;
	uint64_t config_size;
	enum arch_t arch;
	int fv_specified;
	uint32_t kloadaddr;
	uint32_t padding;
	int vblockonly;
	char *outfile;
	int create_new_outfile;
	char *pem_signpriv;
	int pem_algo_specified;
	uint32_t pem_algo;
	char *pem_external;
} option = {
	.version = 1,
	.arch = ARCH_UNSPECIFIED,
	.kloadaddr = CROS_32BIT_ENTRY_ADDR,
	.padding = 65536,
};


/* Helper to complain about invalid args. Returns num errors discovered */
static int no_opt_if(int expr, const char *optname)
{
	if (expr) {
		fprintf(stderr, "Missing --%s option\n", optname);
		return 1;
	}
	return 0;
}

/* This wraps/signs a public key, producing a keyblock. */
int futil_cb_sign_pubkey(struct futil_traverse_state_s *state)
{
	VbPublicKey *data_key = (VbPublicKey *)state->my_area->buf;
	VbKeyBlockHeader *vblock;

	if (option.pem_signpriv) {
		if (option.pem_external) {
			/* External signing uses the PEM file directly. */
			vblock = KeyBlockCreate_external(
				data_key,
				option.pem_signpriv,
				option.pem_algo, option.flags,
				option.pem_external);
		} else {
			option.signprivate = PrivateKeyReadPem(
				option.pem_signpriv, option.pem_algo);
			if (!option.signprivate) {
				fprintf(stderr,
					"Unable to read PEM signing key: %s\n",
					strerror(errno));
				return 1;
			}
			vblock = KeyBlockCreate(data_key, option.signprivate,
						option.flags);
		}
	} else {
		/* Not PEM. Should already have a signing key. */
		vblock = KeyBlockCreate(data_key, option.signprivate,
					option.flags);
	}

	/* Write it out */
	return WriteSomeParts(option.outfile,
			      vblock, vblock->key_block_size,
			      NULL, 0);
}

/*
 * This handles FW_MAIN_A and FW_MAIN_B while processing a BIOS image.
 * The data in state->my_area is just the RW firmware blob, so there's nothing
 * useful to show about it. We'll just mark it as present so when we encounter
 * corresponding VBLOCK area, we'll have this to verify.
 */
int futil_cb_sign_fw_main(struct futil_traverse_state_s *state)
{
	state->my_area->_flags |= AREA_IS_VALID;
	return 0;
}

/*
 * This handles VBLOCK_A and VBLOCK_B while processing a BIOS image.
 * We don't do any signing here. We just check to see if the VBLOCK
 * area contains a firmware preamble.
 */
int futil_cb_sign_fw_vblock(struct futil_traverse_state_s *state)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)state->my_area->buf;
	uint32_t len = state->my_area->len;

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

int futil_cb_create_kernel_part(struct futil_traverse_state_s *state)
{
	uint8_t *vmlinuz_data, *kblob_data, *vblock_data;
	uint64_t vmlinuz_size, kblob_size, vblock_size;
	int rv;

	vmlinuz_data = state->my_area->buf;
	vmlinuz_size = state->my_area->len;

	kblob_data = CreateKernelBlob(
		vmlinuz_data, vmlinuz_size,
		option.arch, option.kloadaddr,
		option.config_data, option.config_size,
		option.bootloader_data, option.bootloader_size,
		&kblob_size);
	if (!kblob_data) {
		fprintf(stderr, "Unable to create kernel blob\n");
		return 1;
	}
	Debug("kblob_size = 0x%" PRIx64 "\n", kblob_size);

	vblock_data = SignKernelBlob(kblob_data, kblob_size, option.padding,
				     option.version, option.kloadaddr,
				     option.keyblock, option.signprivate,
				     option.flags, &vblock_size);
	if (!vblock_data) {
		fprintf(stderr, "Unable to sign kernel blob\n");
		free(kblob_data);
		return 1;
	}
	Debug("vblock_size = 0x%" PRIx64 "\n", vblock_size);

	/* We should be creating a completely new output file.
	 * If not, something's wrong. */
	if (!option.create_new_outfile)
		DIE;

	if (option.vblockonly)
		rv = WriteSomeParts(option.outfile,
				    vblock_data, vblock_size,
				    NULL, 0);
	else
		rv = WriteSomeParts(option.outfile,
				    vblock_data, vblock_size,
				    kblob_data, kblob_size);

	free(vblock_data);
	free(kblob_data);
	return rv;
}

int futil_cb_resign_kernel_part(struct futil_traverse_state_s *state)
{
	uint8_t *kpart_data, *kblob_data, *vblock_data;
	uint64_t kpart_size, kblob_size, vblock_size;
	VbKeyBlockHeader *keyblock = NULL;
	VbKernelPreambleHeader *preamble = NULL;
	int rv = 0;

	kpart_data = state->my_area->buf;
	kpart_size = state->my_area->len;

	/* Note: This just sets some static pointers. It doesn't malloc. */
	kblob_data = UnpackKPart(kpart_data, kpart_size, option.padding,
				 &keyblock, &preamble, &kblob_size);

	if (!kblob_data) {
		fprintf(stderr, "Unable to unpack kernel partition\n");
		return 1;
	}

	/*
	 * We don't let --kloadaddr change when resigning, because the original
	 * vbutil_kernel program didn't do it right. Since obviously no one
	 * ever noticed, we'll maintain bug-compatibility by just not allowing
	 * it here either. To enable it, we'd need to update the zeropage
	 * table's cmd_line_ptr as well as the preamble.
	 */
	option.kloadaddr = preamble->body_load_address;

	/* Replace the config if asked */
	if (option.config_data &&
	    0 != UpdateKernelBlobConfig(kblob_data, kblob_size,
					option.config_data,
					option.config_size)) {
		fprintf(stderr, "Unable to update config\n");
		return 1;
	}

	/* Preserve the version unless a new one is given */
	if (!option.version_specified)
		option.version = preamble->kernel_version;

	/* Preserve the flags if not specified */
	if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS) {
		if (option.flags_specified == 0)
			option.flags = preamble->flags;
	}

	/* Replace the keyblock if asked */
	if (option.keyblock)
		keyblock = option.keyblock;

	/* Compute the new signature */
	vblock_data = SignKernelBlob(kblob_data, kblob_size, option.padding,
				     option.version, option.kloadaddr,
				     keyblock, option.signprivate,
				     option.flags, &vblock_size);
	if (!vblock_data) {
		fprintf(stderr, "Unable to sign kernel blob\n");
		return 1;
	}
	Debug("vblock_size = 0x%" PRIx64 "\n", vblock_size);

	if (option.create_new_outfile) {
		/* Write out what we've been asked for */
		if (option.vblockonly)
			rv = WriteSomeParts(option.outfile,
					    vblock_data, vblock_size,
					    NULL, 0);
		else
			rv = WriteSomeParts(option.outfile,
					    vblock_data, vblock_size,
					    kblob_data, kblob_size);
	} else {
		/* If we're modifying an existing file, it's mmap'ed so that
		 * all our modifications to the buffer will get flushed to
		 * disk when we close it. */
		Memcpy(kpart_data, vblock_data, vblock_size);
	}

	free(vblock_data);
	return rv;
}


int futil_cb_sign_raw_firmware(struct futil_traverse_state_s *state)
{
	VbSignature *body_sig;
	VbFirmwarePreambleHeader *preamble;
	int rv;

	body_sig = CalculateSignature(state->my_area->buf, state->my_area->len,
				      option.signprivate);
	if (!body_sig) {
		fprintf(stderr, "Error calculating body signature\n");
		return 1;
	}

	preamble = CreateFirmwarePreamble(option.version,
					  option.kernel_subkey,
					  body_sig,
					  option.signprivate,
					  option.flags);
	if (!preamble) {
		fprintf(stderr, "Error creating firmware preamble.\n");
		free(body_sig);
		return 1;
	}

	rv = WriteSomeParts(option.outfile,
			    option.keyblock, option.keyblock->key_block_size,
			    preamble, preamble->preamble_size);

	free(preamble);
	free(body_sig);

	return rv;
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

/* This signs a full BIOS image after it's been traversed. */
static int sign_bios_at_end(struct futil_traverse_state_s *state)
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

int futil_cb_sign_end(struct futil_traverse_state_s *state)
{
	switch (state->in_type) {
	case FILE_TYPE_BIOS_IMAGE:
	case FILE_TYPE_OLD_BIOS_IMAGE:
		return sign_bios_at_end(state);

	default:
		/* Any other cleanup needed? */
		break;
	}

	return state->errors;
}

static const char usage[] = "\n"
	"Usage:  " MYNAME " %s [PARAMS] INFILE [OUTFILE]\n"
	"\n"
	"Where INFILE is a\n"
	"\n"
	"  public key (.vbpubk); OUTFILE is a keyblock\n"
	"  raw firmware blob (FW_MAIN_A/B); OUTFILE is a VBLOCK_A/B\n"
	"  complete firmware image (bios.bin)\n"
	"  raw linux kernel; OUTFILE is a kernel partition image\n"
	"  kernel partition image (/dev/sda2, /dev/mmcblk0p2)\n";

static const char usage_pubkey[] = "\n"
	"-----------------------------------------------------------------\n"
	"To sign a public key / create a new keyblock:\n"
	"\n"
	"Required PARAMS:\n"
	"  [--datapubkey]   INFILE          The public key to wrap\n"
	"  [--outfile]      OUTFILE         The resulting keyblock\n"
	"\n"
	"Optional PARAMS:\n"
	"  A private signing key, specified as either\n"
	"    -s|--signprivate FILE.vbprivk  Signing key in .vbprivk format\n"
	"  Or\n"
	"    --pem_signpriv   FILE.pem      Signing key in PEM format...\n"
	"    --pem_algo       NUM           AND the algorithm to use (0 - %d)\n"
	"\n"
	"  If a signing key is not given, the keyblock will not be signed (duh)."
	"\n\n"
	"And these, too:\n\n"
	"  -f|--flags       NUM             Flags specifying use conditions\n"
	"  --pem_external   PROGRAM"
	"         External program to compute the signature\n"
	"                                     (requires a PEM signing key)\n";

static const char usage_fw_main[] = "\n"
	"-----------------------------------------------------------------\n"
	"To sign a raw firmware blob (FW_MAIN_A/B):\n"
	"\n"
	"Required PARAMS:\n"
	"  -s|--signprivate FILE.vbprivk    The private firmware data key\n"
	"  -b|--keyblock    FILE.keyblock   The keyblock containing the\n"
	"                                     public firmware data key\n"
	"  -k|--kernelkey   FILE.vbpubk     The public kernel subkey\n"
	"  -v|--version     NUM             The firmware version number\n"
	"  [--fv]           INFILE"
	"          The raw firmware blob (FW_MAIN_A/B)\n"
	"  [--outfile]      OUTFILE         Output VBLOCK_A/B\n"
	"\n"
	"Optional PARAMS:\n"
	"  -f|--flags       NUM             The preamble flags value"
	" (default is 0)\n";

static const char usage_bios[] = "\n"
	"-----------------------------------------------------------------\n"
	"To sign a complete firmware image (bios.bin):\n"
	"\n"
	"Required PARAMS:\n"
	"  -s|--signprivate FILE.vbprivk    The private firmware data key\n"
	"  -b|--keyblock    FILE.keyblock   The keyblock containing the\n"
	"                                     public firmware data key\n"
	"  -k|--kernelkey   FILE.vbpubk     The public kernel subkey\n"
	"  [--infile]       INFILE          Input firmware image (modified\n"
	"                                     in place if no OUTFILE given)\n"
	"\n"
	"These are required if the A and B firmware differ:\n"
	"  -S|--devsign     FILE.vbprivk    The DEV private firmware data key\n"
	"  -B|--devkeyblock FILE.keyblock   The keyblock containing the\n"
	"                                     DEV public firmware data key\n"
	"\n"
	"Optional PARAMS:\n"
	"  -v|--version     NUM             The firmware version number"
	" (default %d)\n"
	"  -f|--flags       NUM             The preamble flags value"
	" (default is\n"
	"                                     unchanged, or 0 if unknown)\n"
	"  -d|--loemdir     DIR             Local OEM output vblock directory\n"
	"  -l|--loemid      STRING          Local OEM vblock suffix\n"
	"  [--outfile]      OUTFILE         Output firmware image\n";

static const char usage_new_kpart[] = "\n"
	"-----------------------------------------------------------------\n"
	"To create a new kernel parition image (/dev/sda2, /dev/mmcblk0p2):\n"
	"\n"
	"Required PARAMS:\n"
	"  -s|--signprivate FILE.vbprivk"
	"    The private key to sign the kernel blob\n"
	"  -b|--keyblock    FILE.keyblock   The keyblock containing the public\n"
	"                                     key to verify the kernel blob\n"
	"  -v|--version     NUM             The kernel version number\n"
	"  --bootloader     FILE            Bootloader stub\n"
	"  --config         FILE            The kernel commandline file\n"
	"  --arch           ARCH            The CPU architecture (one of\n"
	"                                     x86|amd64, arm|aarch64, mips)\n"
	"  [--vmlinuz]      INFILE          Linux kernel bzImage file\n"
	"  [--outfile]      OUTFILE         Output kernel partition or vblock\n"
	"\n"
	"Optional PARAMS:\n"
	"  --kloadaddr      NUM"
	"             RAM address to load the kernel body\n"
	"                                     (default 0x%x)\n"
	"  --pad            NUM             The vblock padding size in bytes\n"
	"                                     (default 0x%x)\n"
	" --vblockonly                      Emit just the vblock (requires a\n"
	"                                     distinct outfile)\n"
	"  -f|--flags       NUM             The preamble flags value\n";

static const char usage_old_kpart[] = "\n"
	"-----------------------------------------------------------------\n"
	"To resign an existing kernel parition (/dev/sda2, /dev/mmcblk0p2):\n"
	"\n"
	"Required PARAMS:\n"
	"  -s|--signprivate FILE.vbprivk"
	"    The private key to sign the kernel blob\n"
	"  [--infile]       INFILE          Input kernel partition (modified\n"
	"                                     in place if no OUTFILE given)\n"
	"\n"
	"Optional PARAMS:\n"
	"  -b|--keyblock    FILE.keyblock   The keyblock containing the public\n"
	"                                     key to verify the kernel blob\n"
	"  -v|--version     NUM             The kernel version number\n"
	"  --config         FILE            The kernel commandline file\n"
	"  --pad            NUM             The vblock padding size in bytes\n"
	"                                     (default 0x%x)\n"
	"  [--outfile]      OUTFILE         Output kernel partition or vblock\n"
	"  --vblockonly                     Emit just the vblock (requires a\n"
	"                                     distinct OUTFILE)\n"
	"  -f|--flags       NUM             The preamble flags value\n"
	"\n";

static void print_help(const char *prog)
{
	printf(usage, prog);
	printf(usage_pubkey, kNumAlgorithms - 1);
	puts(usage_fw_main);
	printf(usage_bios, option.version);
	printf(usage_new_kpart, option.kloadaddr, option.padding);
	printf(usage_old_kpart, option.padding);
}

enum no_short_opts {
	OPT_FV = 1000,
	OPT_INFILE,			/* aka "--vmlinuz" */
	OPT_OUTFILE,
	OPT_BOOTLOADER,
	OPT_CONFIG,
	OPT_ARCH,
	OPT_KLOADADDR,
	OPT_PADDING,
	OPT_PEM_SIGNPRIV,
	OPT_PEM_ALGO,
	OPT_PEM_EXTERNAL,
};

static const struct option long_opts[] = {
	/* name    hasarg *flag  val */
	{"signprivate",  1, NULL, 's'},
	{"keyblock",     1, NULL, 'b'},
	{"kernelkey",    1, NULL, 'k'},
	{"devsign",      1, NULL, 'S'},
	{"devkeyblock",  1, NULL, 'B'},
	{"version",      1, NULL, 'v'},
	{"flags",        1, NULL, 'f'},
	{"loemdir",      1, NULL, 'd'},
	{"loemid",       1, NULL, 'l'},
	{"fv",           1, NULL, OPT_FV},
	{"infile",       1, NULL, OPT_INFILE},
	{"datapubkey",   1, NULL, OPT_INFILE},	/* alias */
	{"vmlinuz",      1, NULL, OPT_INFILE},	/* alias */
	{"outfile",      1, NULL, OPT_OUTFILE},
	{"bootloader",   1, NULL, OPT_BOOTLOADER},
	{"config",       1, NULL, OPT_CONFIG},
	{"arch",         1, NULL, OPT_ARCH},
	{"kloadaddr",    1, NULL, OPT_KLOADADDR},
	{"pad",          1, NULL, OPT_PADDING},
	{"pem_signpriv", 1, NULL, OPT_PEM_SIGNPRIV},
	{"pem_algo",     1, NULL, OPT_PEM_ALGO},
	{"pem_external", 1, NULL, OPT_PEM_EXTERNAL},
	{"vblockonly",   0, &option.vblockonly, 1},
	{"debug",        0, &debugging_enabled, 1},
	{NULL,           0, NULL, 0},
};
static char *short_opts = ":s:b:k:S:B:v:f:d:l:";

static int do_sign(int argc, char *argv[])
{
	char *infile = 0;
	int i;
	int ifd = -1;
	int errorcnt = 0;
	struct futil_traverse_state_s state;
	uint8_t *buf;
	uint32_t buf_len;
	char *e = 0;
	enum futil_file_type type;
	int inout_file_count = 0;
	int mapping;

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
			option.version_specified = 1;
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
		case OPT_FV:
			option.fv_specified = 1;
			/* fallthrough */
		case OPT_INFILE:		/* aka "--vmlinuz" */
			inout_file_count++;
			infile = optarg;
			break;
		case OPT_OUTFILE:
			inout_file_count++;
			option.outfile = optarg;
			break;
		case OPT_BOOTLOADER:
			option.bootloader_data = ReadFile(
				optarg, &option.bootloader_size);
			if (!option.bootloader_data) {
				fprintf(stderr,
					"Error reading bootloader file: %s\n",
					strerror(errno));
				errorcnt++;
			}
			Debug("bootloader file size=0x%" PRIx64 "\n",
			      option.bootloader_size);
			break;
		case OPT_CONFIG:
			option.config_data = ReadConfigFile(
				optarg, &option.config_size);
			if (!option.config_data) {
				fprintf(stderr,
					"Error reading config file: %s\n",
					strerror(errno));
				errorcnt++;
			}
			break;
		case OPT_ARCH:
			/* check the first 3 characters to also match x86_64 */
			if ((!strncasecmp(optarg, "x86", 3)) ||
			    (!strcasecmp(optarg, "amd64")))
				option.arch = ARCH_X86;
			else if ((!strcasecmp(optarg, "arm")) ||
				 (!strcasecmp(optarg, "aarch64")))
				option.arch = ARCH_ARM;
			else if (!strcasecmp(optarg, "mips"))
				option.arch = ARCH_MIPS;
			else {
				fprintf(stderr,
					"Unknown architecture: \"%s\"\n",
					optarg);
				errorcnt++;
			}
			break;
		case OPT_KLOADADDR:
			option.kloadaddr = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --kloadaddr \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_PADDING:
			option.padding = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --padding \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_PEM_SIGNPRIV:
			option.pem_signpriv = optarg;
			break;
		case OPT_PEM_ALGO:
			option.pem_algo_specified = 1;
			option.pem_algo = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e) ||
			    (option.pem_algo >= kNumAlgorithms)) {
				fprintf(stderr,
					"Invalid --pem_algo \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_PEM_EXTERNAL:
			option.pem_external = optarg;
			break;

		case '?':
			if (optopt)
				fprintf(stderr, "Unrecognized option: -%c\n",
					optopt);
			else
				fprintf(stderr, "Unrecognized option: %s\n",
					argv[optind - 1]);
			errorcnt++;
			break;
		case ':':
			fprintf(stderr, "Missing argument to -%c\n", optopt);
			errorcnt++;
			break;
		case 0:				/* handled option */
			break;
		default:
			Debug("i=%d\n", i);
			DIE;
		}
	}

	/* If we don't have an input file already, we need one */
	if (!infile) {
		if (argc - optind <= 0) {
			errorcnt++;
			fprintf(stderr, "ERROR: missing input filename\n");
			goto done;
		} else {
			inout_file_count++;
			infile = argv[optind++];
		}
	}

	/* Look for an output file if we don't have one, just in case. */
	if (!option.outfile && argc - optind > 0) {
		inout_file_count++;
		option.outfile = argv[optind++];
	}

	/* What are we looking at? */
	if (futil_file_type(infile, &type)) {
		errorcnt++;
		goto done;
	}

	/* We may be able to infer the type based on the other args */
	if (type == FILE_TYPE_UNKNOWN) {
		if (option.bootloader_data || option.config_data
		    || option.arch != ARCH_UNSPECIFIED)
			type = FILE_TYPE_RAW_KERNEL;
		else if (option.kernel_subkey || option.fv_specified)
			type = FILE_TYPE_RAW_FIRMWARE;
	}

	Debug("type=%s\n", futil_file_type_str(type));

	/* Check the arguments for the type of thing we want to sign */
	switch (type) {
	case FILE_TYPE_UNKNOWN:
		fprintf(stderr,
			"Unable to determine the type of the input file\n");
		errorcnt++;
		goto done;
	case FILE_TYPE_PUBKEY:
		option.create_new_outfile = 1;
		if (option.signprivate && option.pem_signpriv) {
			fprintf(stderr,
				"Only one of --signprivate and --pem_signpriv"
				" can be specified\n");
			errorcnt++;
		}
		if ((option.signprivate && option.pem_algo_specified) ||
		    (option.pem_signpriv && !option.pem_algo_specified)) {
			fprintf(stderr, "--pem_algo must be used with"
				" --pem_signpriv\n");
			errorcnt++;
		}
		if (option.pem_external && !option.pem_signpriv) {
			fprintf(stderr, "--pem_external must be used with"
				" --pem_signpriv\n");
			errorcnt++;
		}
		/* We'll wait to read the PEM file, since the external signer
		 * may want to read it instead. */
		break;
	case FILE_TYPE_KEYBLOCK:
		fprintf(stderr, "Resigning a keyblock is kind of pointless.\n");
		fprintf(stderr, "Just create a new one.\n");
		errorcnt++;
		break;
	case FILE_TYPE_FW_PREAMBLE:
		fprintf(stderr,
			"%s IS a signature. Sign the firmware instead\n",
			infile);
		break;
	case FILE_TYPE_GBB:
		fprintf(stderr, "There's no way to sign a GBB\n");
		errorcnt++;
		break;
	case FILE_TYPE_BIOS_IMAGE:
	case FILE_TYPE_OLD_BIOS_IMAGE:
		errorcnt += no_opt_if(!option.signprivate, "signprivate");
		errorcnt += no_opt_if(!option.keyblock, "keyblock");
		errorcnt += no_opt_if(!option.kernel_subkey, "kernelkey");
		break;
	case FILE_TYPE_KERN_PREAMBLE:
		errorcnt += no_opt_if(!option.signprivate, "signprivate");
		if (option.vblockonly || inout_file_count > 1)
			option.create_new_outfile = 1;
		break;
	case FILE_TYPE_RAW_FIRMWARE:
		option.create_new_outfile = 1;
		errorcnt += no_opt_if(!option.signprivate, "signprivate");
		errorcnt += no_opt_if(!option.keyblock, "keyblock");
		errorcnt += no_opt_if(!option.kernel_subkey, "kernelkey");
		errorcnt += no_opt_if(!option.version_specified, "version");
		break;
	case FILE_TYPE_RAW_KERNEL:
		option.create_new_outfile = 1;
		errorcnt += no_opt_if(!option.signprivate, "signprivate");
		errorcnt += no_opt_if(!option.keyblock, "keyblock");
		errorcnt += no_opt_if(!option.version_specified, "version");
		errorcnt += no_opt_if(!option.bootloader_data, "bootloader");
		errorcnt += no_opt_if(!option.config_data, "config");
		errorcnt += no_opt_if(option.arch == ARCH_UNSPECIFIED, "arch");
		break;
	case FILE_TYPE_CHROMIUMOS_DISK:
		fprintf(stderr, "Signing a %s is not yet supported\n",
			futil_file_type_str(type));
		errorcnt++;
		break;
	default:
		DIE;
	}

	Debug("infile=%s\n", infile);
	Debug("inout_file_count=%d\n", inout_file_count);
	Debug("option.create_new_outfile=%d\n", option.create_new_outfile);

	/* Make sure we have an output file if one is needed */
	if (!option.outfile) {
		if (option.create_new_outfile) {
			errorcnt++;
			fprintf(stderr, "Missing output filename\n");
			goto done;
		} else {
			option.outfile = infile;
		}
	}

	Debug("option.outfile=%s\n", option.outfile);

	if (argc - optind > 0) {
		errorcnt++;
		fprintf(stderr, "ERROR: too many arguments left over\n");
	}

	if (errorcnt)
		goto done;

	memset(&state, 0, sizeof(state));
	state.op = FUTIL_OP_SIGN;

	if (option.create_new_outfile) {
		/* The input is read-only, the output is write-only. */
		mapping = MAP_RO;
		state.in_filename = infile;
		Debug("open RO %s\n", infile);
		ifd = open(infile, O_RDONLY);
		if (ifd < 0) {
			errorcnt++;
			fprintf(stderr, "Can't open %s for reading: %s\n",
				infile, strerror(errno));
			goto done;
		}
	} else {
		/* We'll read-modify-write the output file */
		mapping = MAP_RW;
		state.in_filename = option.outfile;
		if (inout_file_count > 1)
			futil_copy_file_or_die(infile, option.outfile);
		Debug("open RW %s\n", option.outfile);
		ifd = open(option.outfile, O_RDWR);
		if (ifd < 0) {
			errorcnt++;
			fprintf(stderr, "Can't open %s for writing: %s\n",
				option.outfile, strerror(errno));
			goto done;
		}
	}

	if (0 != futil_map_file(ifd, mapping, &buf, &buf_len)) {
		errorcnt++;
		goto done;
	}

	errorcnt += futil_traverse(buf, buf_len, &state, type);

	errorcnt += futil_unmap_file(ifd, MAP_RW, buf, buf_len);

done:
	if (ifd >= 0 && close(ifd)) {
		errorcnt++;
		fprintf(stderr, "Error when closing ifd: %s\n",
			strerror(errno));
	}

	if (option.signprivate)
		free(option.signprivate);
	if (option.keyblock)
		free(option.keyblock);
	if (option.kernel_subkey)
		free(option.kernel_subkey);

	if (errorcnt)
		fprintf(stderr, "Use --help for usage instructions\n");

	return !!errorcnt;
}

DECLARE_FUTIL_COMMAND(sign, do_sign,
		      VBOOT_VERSION_ALL,
		      "Sign / resign various binary components",
		      print_help);
