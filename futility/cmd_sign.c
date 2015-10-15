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

#include "file_type.h"
#include "file_type_bios.h"
#include "fmap.h"
#include "futility.h"
#include "futility_options.h"
#include "gbb_header.h"
#include "host_common.h"
#include "kernel_blob.h"
#include "util_misc.h"
#include "vb1_helper.h"
#include "vb2_common.h"
#include "host_key2.h"
#include "vboot_common.h"

/* Options */
struct sign_option_s sign_option = {
	.version = 1,
	.arch = ARCH_UNSPECIFIED,
	.kloadaddr = CROS_32BIT_ENTRY_ADDR,
	.padding = 65536,
	.type = FILE_TYPE_UNKNOWN,
	.hash_alg = VB2_HASH_SHA256,		/* default */
	.ro_size = 0xffffffff,
	.rw_size = 0xffffffff,
	.ro_offset = 0xffffffff,
	.rw_offset = 0xffffffff,
	.sig_size = 1024,
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
int ft_sign_pubkey(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	VbPublicKey *data_key = (VbPublicKey *)buf;
	VbKeyBlockHeader *vblock;

	if (sign_option.pem_signpriv) {
		if (sign_option.pem_external) {
			/* External signing uses the PEM file directly. */
			vblock = KeyBlockCreate_external(
				data_key,
				sign_option.pem_signpriv,
				sign_option.pem_algo, sign_option.flags,
				sign_option.pem_external);
		} else {
			sign_option.signprivate = PrivateKeyReadPem(
				sign_option.pem_signpriv,
				sign_option.pem_algo);
			if (!sign_option.signprivate) {
				fprintf(stderr,
					"Unable to read PEM signing key: %s\n",
					strerror(errno));
				return 1;
			}
			vblock = KeyBlockCreate(data_key,
						sign_option.signprivate,
						sign_option.flags);
		}
	} else {
		/* Not PEM. Should already have a signing key. */
		vblock = KeyBlockCreate(data_key, sign_option.signprivate,
					sign_option.flags);
	}

	/* Write it out */
	return WriteSomeParts(sign_option.outfile,
			      vblock, vblock->key_block_size,
			      NULL, 0);
}

int ft_sign_raw_kernel(const char *name, uint8_t *buf, uint32_t len,
		       void *data)
{
	uint8_t *vmlinuz_data, *kblob_data, *vblock_data;
	uint64_t vmlinuz_size, kblob_size, vblock_size;
	int rv;

	vmlinuz_data = buf;
	vmlinuz_size = len;

	kblob_data = CreateKernelBlob(
		vmlinuz_data, vmlinuz_size,
		sign_option.arch, sign_option.kloadaddr,
		sign_option.config_data, sign_option.config_size,
		sign_option.bootloader_data, sign_option.bootloader_size,
		&kblob_size);
	if (!kblob_data) {
		fprintf(stderr, "Unable to create kernel blob\n");
		return 1;
	}
	Debug("kblob_size = 0x%" PRIx64 "\n", kblob_size);

	vblock_data = SignKernelBlob(kblob_data, kblob_size,
				     sign_option.padding,
				     sign_option.version,
				     sign_option.kloadaddr,
				     sign_option.keyblock,
				     sign_option.signprivate,
				     sign_option.flags, &vblock_size);
	if (!vblock_data) {
		fprintf(stderr, "Unable to sign kernel blob\n");
		free(kblob_data);
		return 1;
	}
	Debug("vblock_size = 0x%" PRIx64 "\n", vblock_size);

	/* We should be creating a completely new output file.
	 * If not, something's wrong. */
	if (!sign_option.create_new_outfile)
		DIE;

	if (sign_option.vblockonly)
		rv = WriteSomeParts(sign_option.outfile,
				    vblock_data, vblock_size,
				    NULL, 0);
	else
		rv = WriteSomeParts(sign_option.outfile,
				    vblock_data, vblock_size,
				    kblob_data, kblob_size);

	free(vblock_data);
	free(kblob_data);
	return rv;
}

int ft_sign_kern_preamble(const char *name, uint8_t *buf, uint32_t len,
			  void *data)
{
	uint8_t *kpart_data, *kblob_data, *vblock_data;
	uint64_t kpart_size, kblob_size, vblock_size;
	VbKeyBlockHeader *keyblock = NULL;
	VbKernelPreambleHeader *preamble = NULL;
	int rv = 0;

	kpart_data = buf;
	kpart_size = len;

	/* Note: This just sets some static pointers. It doesn't malloc. */
	kblob_data = UnpackKPart(kpart_data, kpart_size, sign_option.padding,
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
	sign_option.kloadaddr = preamble->body_load_address;

	/* Replace the config if asked */
	if (sign_option.config_data &&
	    0 != UpdateKernelBlobConfig(kblob_data, kblob_size,
					sign_option.config_data,
					sign_option.config_size)) {
		fprintf(stderr, "Unable to update config\n");
		return 1;
	}

	/* Preserve the version unless a new one is given */
	if (!sign_option.version_specified)
		sign_option.version = preamble->kernel_version;

	/* Preserve the flags if not specified */
	if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS) {
		if (sign_option.flags_specified == 0)
			sign_option.flags = preamble->flags;
	}

	/* Replace the keyblock if asked */
	if (sign_option.keyblock)
		keyblock = sign_option.keyblock;

	/* Compute the new signature */
	vblock_data = SignKernelBlob(kblob_data, kblob_size,
				     sign_option.padding,
				     sign_option.version,
				     sign_option.kloadaddr,
				     keyblock,
				     sign_option.signprivate,
				     sign_option.flags,
				     &vblock_size);
	if (!vblock_data) {
		fprintf(stderr, "Unable to sign kernel blob\n");
		return 1;
	}
	Debug("vblock_size = 0x%" PRIx64 "\n", vblock_size);

	if (sign_option.create_new_outfile) {
		/* Write out what we've been asked for */
		if (sign_option.vblockonly)
			rv = WriteSomeParts(sign_option.outfile,
					    vblock_data, vblock_size,
					    NULL, 0);
		else
			rv = WriteSomeParts(sign_option.outfile,
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


int ft_sign_raw_firmware(const char *name, uint8_t *buf, uint32_t len,
			 void *data)
{
	VbSignature *body_sig;
	VbFirmwarePreambleHeader *preamble;
	int rv;

	body_sig = CalculateSignature(buf, len, sign_option.signprivate);
	if (!body_sig) {
		fprintf(stderr, "Error calculating body signature\n");
		return 1;
	}

	preamble = CreateFirmwarePreamble(sign_option.version,
					  sign_option.kernel_subkey,
					  body_sig,
					  sign_option.signprivate,
					  sign_option.flags);
	if (!preamble) {
		fprintf(stderr, "Error creating firmware preamble.\n");
		free(body_sig);
		return 1;
	}

	rv = WriteSomeParts(sign_option.outfile,
			    sign_option.keyblock,
			    sign_option.keyblock->key_block_size,
			    preamble, preamble->preamble_size);

	free(preamble);
	free(body_sig);

	return rv;
}

static const char usage_pubkey[] = "\n"
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
	"                                     (requires a PEM signing key)\n"
	"\n";
static void print_help_pubkey(int argc, char *argv[])
{
	printf(usage_pubkey, kNumAlgorithms - 1);
}


static const char usage_fw_main[] = "\n"
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
	" (default is 0)\n"
	"\n";
static void print_help_raw_firmware(int argc, char *argv[])
{
	puts(usage_fw_main);
}

static const char usage_bios[] = "\n"
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
	"  [--outfile]      OUTFILE         Output firmware image\n"
	"\n";
static void print_help_bios_image(int argc, char *argv[])
{
	printf(usage_bios, sign_option.version);
}

static const char usage_new_kpart[] = "\n"
	"To create a new kernel partition image (/dev/sda2, /dev/mmcblk0p2):\n"
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
	"  -f|--flags       NUM             The preamble flags value\n"
	"\n";
static void print_help_raw_kernel(int argc, char *argv[])
{
	printf(usage_new_kpart, sign_option.kloadaddr, sign_option.padding);
}

static const char usage_old_kpart[] = "\n"
	"To resign an existing kernel partition (/dev/sda2, /dev/mmcblk0p2):\n"
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
static void print_help_kern_preamble(int argc, char *argv[])
{
	printf(usage_old_kpart, sign_option.padding);
}

static void print_help_usbpd1(int argc, char *argv[])
{
	struct vb2_text_vs_enum *entry;

	printf("\n"
	       "Usage:  " MYNAME " %s --type %s [options] INFILE [OUTFILE]\n"
	       "\n"
	       "This signs a %s.\n"
	       "\n"
	       "The INFILE is assumed to consist of equal-sized RO and RW"
	       " sections,\n"
	       "with the public key at the end of of the RO section and the"
	       " signature\n"
	       "at the end of the RW section (both in an opaque binary"
	       " format).\n"
	       "Signing the image will update both binary blobs, so both"
	       " public and\n"
	       "private keys are required.\n"
	       "\n"
	       "The signing key is specified with:\n"
	       "\n"
	       "  --pem            "
	       "FILE          Signing keypair in PEM format\n"
	       "\n"
	       "  --hash_alg       "
	       "NUM           Hash algorithm to use:\n",
	       argv[0],
	       futil_file_type_name(FILE_TYPE_USBPD1),
	       futil_file_type_desc(FILE_TYPE_USBPD1));
	for (entry = vb2_text_vs_hash; entry->name; entry++)
		printf("                                   %d / %s%s\n",
		       entry->num, entry->name,
		       entry->num == VB2_HASH_SHA256 ? " (default)" : "");
	printf("\n"
	       "The size and offset assumptions can be overridden. "
	       "All numbers are in bytes.\n"
	       "Specify a size of 0 to ignore that section.\n"
	       "\n"
	       "  --ro_size        NUM"
	       "           Size of the RO section (default half)\n"
	       "  --rw_size        NUM"
	       "           Size of the RW section (default half)\n"
	       "  --ro_offset      NUM"
	       "           Start of the RO section (default 0)\n"
	       "  --rw_offset      NUM"
	       "           Start of the RW section (default half)\n"
	       "\n");
}

static void print_help_rwsig(int argc, char *argv[])
{
	printf("\n"
	       "Usage:  " MYNAME " %s --type %s [options] INFILE [OUTFILE]\n"
	       "\n"
	       "This signs a %s.\n"
	       "\n"
	       "The INFILE is a binary blob of arbitrary size."
	       " It is signed using the\n"
	       "private key and the vb2_signature blob emitted.\n"
	       "\n"
	       "If no OUTFILE is specified, the INFILE should contain"
	       " an existing\n"
	       "vb2_signature blob near its end. The data_size from that"
	       " signature is\n"
	       "used to re-sign a portion of the INFILE, and the old"
	       " signature blob is\n"
	       "replaced.\n"
	       "\n"
	       "Options:\n"
	       "\n"
	       "  --prikey      FILE.vbprik2      "
	       "Private key in vb2 format (required)\n"
	       "  --sig_size    NUM               "
	       "Offset from the end of INFILE where the\n"
	       "                                    "
	       "signature blob should be located\n"
	       "                                    "
	       "(default 1024 bytes)\n"
	       "  --data_size   NUM               "
	       "Number of bytes of INFILE to sign\n"
	       "\n",
	       argv[0],
	       futil_file_type_name(FILE_TYPE_RWSIG),
	       futil_file_type_desc(FILE_TYPE_RWSIG));
}

static void (*help_type[NUM_FILE_TYPES])(int argc, char *argv[]) = {
	[FILE_TYPE_PUBKEY] = &print_help_pubkey,
	[FILE_TYPE_RAW_FIRMWARE] = &print_help_raw_firmware,
	[FILE_TYPE_BIOS_IMAGE] = &print_help_bios_image,
	[FILE_TYPE_RAW_KERNEL] = &print_help_raw_kernel,
	[FILE_TYPE_KERN_PREAMBLE] = &print_help_kern_preamble,
	[FILE_TYPE_USBPD1] = &print_help_usbpd1,
	[FILE_TYPE_RWSIG] = &print_help_rwsig,
};

static const char usage_default[] = "\n"
	"Usage:  " MYNAME " %s [PARAMS] INFILE [OUTFILE]\n"
	"\n"
	"The following signing operations are supported:\n"
	"\n"
	"    INFILE                              OUTFILE\n"
	"  public key (.vbpubk)                keyblock\n"
	"  raw firmware blob (FW_MAIN_A/B)     firmware preamble (VBLOCK_A/B)\n"
	"  full firmware image (bios.bin)      same, or signed in-place\n"
	"  raw linux kernel (vmlinuz)          kernel partition image\n"
	"  kernel partition (/dev/sda2)        same, or signed in-place\n"
	"  usbpd1 firmware image               same, or signed in-place\n"
	"  RW device image                     same, or signed in-place\n"
	"\n"
	"For more information, use \"" MYNAME " help %s TYPE\", where\n"
	"TYPE is one of:\n\n";
static void print_help_default(int argc, char *argv[])
{
	enum futil_file_type type;

	printf(usage_default, argv[0], argv[0]);
	for (type = 0; type < NUM_FILE_TYPES; type++)
		if (help_type[type])
			printf("  %s", futil_file_type_name(type));
	printf("\n\n");
}

static void print_help(int argc, char *argv[])
{
	enum futil_file_type type = FILE_TYPE_UNKNOWN;

	if (argc > 1)
		futil_str_to_file_type(argv[1], &type);

	if (help_type[type])
		help_type[type](argc, argv);
	else
		print_help_default(argc, argv);
}

enum no_short_opts {
	OPT_FV = 1000,
	OPT_INFILE,
	OPT_OUTFILE,
	OPT_BOOTLOADER,
	OPT_CONFIG,
	OPT_ARCH,
	OPT_KLOADADDR,
	OPT_PADDING,
	OPT_PEM_SIGNPRIV,
	OPT_PEM_ALGO,
	OPT_PEM_EXTERNAL,
	OPT_TYPE,
	OPT_HASH_ALG,
	OPT_RO_SIZE,
	OPT_RW_SIZE,
	OPT_RO_OFFSET,
	OPT_RW_OFFSET,
	OPT_DATA_SIZE,
	OPT_SIG_SIZE,
	OPT_PRIKEY,
	OPT_HELP,
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
	{"pem",          1, NULL, OPT_PEM_SIGNPRIV}, /* alias */
	{"pem_algo",     1, NULL, OPT_PEM_ALGO},
	{"pem_external", 1, NULL, OPT_PEM_EXTERNAL},
	{"type",         1, NULL, OPT_TYPE},
	{"vblockonly",   0, &sign_option.vblockonly, 1},
	{"hash_alg",     1, NULL, OPT_HASH_ALG},
	{"ro_size",      1, NULL, OPT_RO_SIZE},
	{"rw_size",      1, NULL, OPT_RW_SIZE},
	{"ro_offset",    1, NULL, OPT_RO_OFFSET},
	{"rw_offset",    1, NULL, OPT_RW_OFFSET},
	{"data_size",    1, NULL, OPT_DATA_SIZE},
	{"sig_size",     1, NULL, OPT_SIG_SIZE},
	{"prikey",       1, NULL, OPT_PRIKEY},
	{"privkey",      1, NULL, OPT_PRIKEY},	/* alias */
	{"help",         0, NULL, OPT_HELP},
	{NULL,           0, NULL, 0},
};
static char *short_opts = ":s:b:k:S:B:v:f:d:l:";

/* Return zero on success */
static int parse_number_opt(const char *arg, const char *name, uint32_t *dest)
{
	char *e;
	uint32_t val = strtoul(arg, &e, 0);
	if (!*arg || (e && *e)) {
		fprintf(stderr, "Invalid --%s \"%s\"\n", name, arg);
		return 1;
	}
	*dest = val;
	return 0;
}

static int do_sign(int argc, char *argv[])
{
	char *infile = 0;
	int i;
	int ifd = -1;
	int errorcnt = 0;
	uint8_t *buf;
	uint32_t buf_len;
	char *e = 0;
	int mapping;
	int helpind = 0;
	int longindex;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, short_opts, long_opts,
				&longindex)) != -1) {
		switch (i) {
		case 's':
			sign_option.signprivate = PrivateKeyRead(optarg);
			if (!sign_option.signprivate) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'b':
			sign_option.keyblock = KeyBlockRead(optarg);
			if (!sign_option.keyblock) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'k':
			sign_option.kernel_subkey = PublicKeyRead(optarg);
			if (!sign_option.kernel_subkey) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'S':
			sign_option.devsignprivate = PrivateKeyRead(optarg);
			if (!sign_option.devsignprivate) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'B':
			sign_option.devkeyblock = KeyBlockRead(optarg);
			if (!sign_option.devkeyblock) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case 'v':
			sign_option.version_specified = 1;
			sign_option.version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --version \"%s\"\n", optarg);
				errorcnt++;
			}
			break;

		case 'f':
			sign_option.flags_specified = 1;
			errorcnt += parse_number_opt(optarg, "flags",
						     &sign_option.flags);
			break;
		case 'd':
			sign_option.loemdir = optarg;
			break;
		case 'l':
			sign_option.loemid = optarg;
			break;
		case OPT_FV:
			sign_option.fv_specified = 1;
			/* fallthrough */
		case OPT_INFILE:
			sign_option.inout_file_count++;
			infile = optarg;
			break;
		case OPT_OUTFILE:
			sign_option.inout_file_count++;
			sign_option.outfile = optarg;
			break;
		case OPT_BOOTLOADER:
			sign_option.bootloader_data = ReadFile(
				optarg, &sign_option.bootloader_size);
			if (!sign_option.bootloader_data) {
				fprintf(stderr,
					"Error reading bootloader file: %s\n",
					strerror(errno));
				errorcnt++;
			}
			Debug("bootloader file size=0x%" PRIx64 "\n",
			      sign_option.bootloader_size);
			break;
		case OPT_CONFIG:
			sign_option.config_data = ReadConfigFile(
				optarg, &sign_option.config_size);
			if (!sign_option.config_data) {
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
				sign_option.arch = ARCH_X86;
			else if ((!strcasecmp(optarg, "arm")) ||
				 (!strcasecmp(optarg, "aarch64")))
				sign_option.arch = ARCH_ARM;
			else if (!strcasecmp(optarg, "mips"))
				sign_option.arch = ARCH_MIPS;
			else {
				fprintf(stderr,
					"Unknown architecture: \"%s\"\n",
					optarg);
				errorcnt++;
			}
			break;
		case OPT_KLOADADDR:
			errorcnt += parse_number_opt(optarg, "kloadaddr",
						     &sign_option.kloadaddr);
			break;
		case OPT_PADDING:
			errorcnt += parse_number_opt(optarg, "padding",
						     &sign_option.padding);
			break;
		case OPT_RO_SIZE:
			errorcnt += parse_number_opt(optarg, "ro_size",
						     &sign_option.ro_size);
			break;
		case OPT_RW_SIZE:
			errorcnt += parse_number_opt(optarg, "rw_size",
						     &sign_option.rw_size);
			break;
		case OPT_RO_OFFSET:
			errorcnt += parse_number_opt(optarg, "ro_offset",
						     &sign_option.ro_offset);
			break;
		case OPT_RW_OFFSET:
			errorcnt += parse_number_opt(optarg, "rw_offset",
						     &sign_option.rw_offset);
			break;
		case OPT_DATA_SIZE:
			errorcnt += parse_number_opt(optarg, "data_size",
						     &sign_option.data_size);
			break;
		case OPT_SIG_SIZE:
			errorcnt += parse_number_opt(optarg, "sig_size",
						     &sign_option.sig_size);
			break;
		case OPT_PEM_SIGNPRIV:
			sign_option.pem_signpriv = optarg;
			break;
		case OPT_PEM_ALGO:
			sign_option.pem_algo_specified = 1;
			sign_option.pem_algo = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e) ||
			    (sign_option.pem_algo >= kNumAlgorithms)) {
				fprintf(stderr,
					"Invalid --pem_algo \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_HASH_ALG:
			if (!vb2_lookup_hash_alg(optarg,
						 &sign_option.hash_alg)) {
				fprintf(stderr,
					"invalid hash_alg \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_PEM_EXTERNAL:
			sign_option.pem_external = optarg;
			break;
		case OPT_TYPE:
			if (!futil_str_to_file_type(optarg,
						    &sign_option.type)) {
				if (!strcasecmp("help", optarg))
				    print_file_types_and_exit(errorcnt);
				fprintf(stderr,
					"Invalid --type \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_PRIKEY:
			if (vb2_private_key_read(&sign_option.prikey,
						 optarg)) {
				fprintf(stderr, "Error reading %s\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_HELP:
			helpind = optind - 1;
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

	if (helpind) {
		/* Skip all the options we've already parsed */
		optind--;
		argv[optind] = argv[0];
		argc -= optind;
		argv += optind;
		print_help(argc, argv);
		return !!errorcnt;
	}

	/* If we don't have an input file already, we need one */
	if (!infile) {
		if (argc - optind <= 0) {
			errorcnt++;
			fprintf(stderr, "ERROR: missing input filename\n");
			goto done;
		} else {
			sign_option.inout_file_count++;
			infile = argv[optind++];
		}
	}

	/* Look for an output file if we don't have one, just in case. */
	if (!sign_option.outfile && argc - optind > 0) {
		sign_option.inout_file_count++;
		sign_option.outfile = argv[optind++];
	}

	/* What are we looking at? */
	if (sign_option.type == FILE_TYPE_UNKNOWN &&
	    futil_file_type(infile, &sign_option.type)) {
		errorcnt++;
		goto done;
	}

	/* We may be able to infer the type based on the other args */
	if (sign_option.type == FILE_TYPE_UNKNOWN) {
		if (sign_option.bootloader_data || sign_option.config_data
		    || sign_option.arch != ARCH_UNSPECIFIED)
			sign_option.type = FILE_TYPE_RAW_KERNEL;
		else if (sign_option.kernel_subkey || sign_option.fv_specified)
			sign_option.type = FILE_TYPE_RAW_FIRMWARE;
	}

	Debug("type=%s\n", futil_file_type_name(sign_option.type));

	/* Check the arguments for the type of thing we want to sign */
	switch (sign_option.type) {
	case FILE_TYPE_PUBKEY:
		sign_option.create_new_outfile = 1;
		if (sign_option.signprivate && sign_option.pem_signpriv) {
			fprintf(stderr,
				"Only one of --signprivate and --pem_signpriv"
				" can be specified\n");
			errorcnt++;
		}
		if ((sign_option.signprivate &&
		     sign_option.pem_algo_specified) ||
		    (sign_option.pem_signpriv &&
		     !sign_option.pem_algo_specified)) {
			fprintf(stderr, "--pem_algo must be used with"
				" --pem_signpriv\n");
			errorcnt++;
		}
		if (sign_option.pem_external && !sign_option.pem_signpriv) {
			fprintf(stderr, "--pem_external must be used with"
				" --pem_signpriv\n");
			errorcnt++;
		}
		/* We'll wait to read the PEM file, since the external signer
		 * may want to read it instead. */
		break;
	case FILE_TYPE_BIOS_IMAGE:
	case FILE_TYPE_OLD_BIOS_IMAGE:
		errorcnt += no_opt_if(!sign_option.signprivate, "signprivate");
		errorcnt += no_opt_if(!sign_option.keyblock, "keyblock");
		errorcnt += no_opt_if(!sign_option.kernel_subkey, "kernelkey");
		break;
	case FILE_TYPE_KERN_PREAMBLE:
		errorcnt += no_opt_if(!sign_option.signprivate, "signprivate");
		if (sign_option.vblockonly || sign_option.inout_file_count > 1)
			sign_option.create_new_outfile = 1;
		break;
	case FILE_TYPE_RAW_FIRMWARE:
		sign_option.create_new_outfile = 1;
		errorcnt += no_opt_if(!sign_option.signprivate, "signprivate");
		errorcnt += no_opt_if(!sign_option.keyblock, "keyblock");
		errorcnt += no_opt_if(!sign_option.kernel_subkey, "kernelkey");
		errorcnt += no_opt_if(!sign_option.version_specified,
				      "version");
		break;
	case FILE_TYPE_RAW_KERNEL:
		sign_option.create_new_outfile = 1;
		errorcnt += no_opt_if(!sign_option.signprivate, "signprivate");
		errorcnt += no_opt_if(!sign_option.keyblock, "keyblock");
		errorcnt += no_opt_if(!sign_option.version_specified,
				      "version");
		errorcnt += no_opt_if(!sign_option.bootloader_data,
				      "bootloader");
		errorcnt += no_opt_if(!sign_option.config_data, "config");
		errorcnt += no_opt_if(sign_option.arch == ARCH_UNSPECIFIED,
				      "arch");
		break;
	case FILE_TYPE_USBPD1:
		errorcnt += no_opt_if(!sign_option.pem_signpriv, "pem");
		errorcnt += no_opt_if(sign_option.hash_alg == VB2_HASH_INVALID,
				      "hash_alg");
		break;
	case FILE_TYPE_RWSIG:
		errorcnt += no_opt_if(!sign_option.prikey, "prikey");
		break;
	default:
		/* Anything else we don't care */
		break;
	}

	Debug("infile=%s\n", infile);
	Debug("sign_option.inout_file_count=%d\n", sign_option.inout_file_count);
	Debug("sign_option.create_new_outfile=%d\n",
	      sign_option.create_new_outfile);

	/* Make sure we have an output file if one is needed */
	if (!sign_option.outfile) {
		if (sign_option.create_new_outfile) {
			errorcnt++;
			fprintf(stderr, "Missing output filename\n");
			goto done;
		} else {
			sign_option.outfile = infile;
		}
	}

	Debug("sign_option.outfile=%s\n", sign_option.outfile);

	if (argc - optind > 0) {
		errorcnt++;
		fprintf(stderr, "ERROR: too many arguments left over\n");
	}

	if (errorcnt)
		goto done;

	if (sign_option.create_new_outfile) {
		/* The input is read-only, the output is write-only. */
		mapping = MAP_RO;
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
	       if (sign_option.inout_file_count > 1)
		       futil_copy_file_or_die(infile, sign_option.outfile);
	       Debug("open RW %s\n", sign_option.outfile);
	       infile = sign_option.outfile;
	       ifd = open(sign_option.outfile, O_RDWR);
	       if (ifd < 0) {
		       errorcnt++;
		       fprintf(stderr, "Can't open %s for writing: %s\n",
			       sign_option.outfile, strerror(errno));
		       goto done;
	       }
	}

	if (0 != futil_map_file(ifd, mapping, &buf, &buf_len)) {
		errorcnt++;
		goto done;
	}

	errorcnt += futil_file_type_sign(sign_option.type, infile,
					 buf, buf_len);

	errorcnt += futil_unmap_file(ifd, mapping, buf, buf_len);

done:
	if (ifd >= 0 && close(ifd)) {
		errorcnt++;
		fprintf(stderr, "Error when closing ifd: %s\n",
			strerror(errno));
	}

	if (sign_option.signprivate)
		free(sign_option.signprivate);
	if (sign_option.keyblock)
		free(sign_option.keyblock);
	if (sign_option.kernel_subkey)
		free(sign_option.kernel_subkey);

	if (errorcnt)
		fprintf(stderr, "Use --help for usage instructions\n");

	return !!errorcnt;
}

DECLARE_FUTIL_COMMAND(sign, do_sign, VBOOT_VERSION_ALL,
		      "Sign / resign various binary components");
