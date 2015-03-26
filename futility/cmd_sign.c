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
#include "futility_options.h"
#include "gbb_header.h"
#include "host_common.h"
#include "kernel_blob.h"
#include "traversal.h"
#include "util_misc.h"
#include "vb1_helper.h"
#include "vboot_common.h"

/* Options */
struct sign_option_s sign_option = {
	.version = 1,
	.arch = ARCH_UNSPECIFIED,
	.kloadaddr = CROS_32BIT_ENTRY_ADDR,
	.padding = 65536,
	.type = FILE_TYPE_UNKNOWN,
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

/* Stuff for BIOS images */

/* Forward declarations */
static int fmap_fw_main(const char *name, uint8_t *buf, uint32_t len,
			void *data);
static int fmap_fw_preamble(const char *name, uint8_t *buf, uint32_t len,
			    void *data);

/* These are the functions we'll call for each FMAP area. */
static int (*fmap_func[])(const char *name, uint8_t *buf, uint32_t len,
			  void *data) = {
	0,
	fmap_fw_main,
	fmap_fw_main,
	fmap_fw_preamble,
	fmap_fw_preamble,
};
BUILD_ASSERT(ARRAY_SIZE(fmap_func) == NUM_BIOS_COMPONENTS);

/* Where is the component we're looking at? */
struct bios_area_s {
	uint8_t *buf;
	uint32_t len;
	uint32_t is_valid;
};

/* When looking at the FMAP areas, we need to gather some state for later. */
struct sign_state_s {
	/* Current component */
	enum bios_component c;
	/* Other activites, possibly before or after the current one */
	struct bios_area_s area[NUM_BIOS_COMPONENTS];
};


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

/*
 * This handles FW_MAIN_A and FW_MAIN_B while signing a BIOS image. The data is
 * just the RW firmware blob so there's nothing useful to do with it, but we'll
 * mark it as valid so that we'll know that this FMAP area exists and can
 * be signed.
 */
static int fmap_fw_main(const char *name, uint8_t *buf, uint32_t len,
			void *data)
{
	struct sign_state_s *state = (struct sign_state_s *)data;
	state->area[state->c].is_valid = 1;
	return 0;
}

/*
 * This handles VBLOCK_A and VBLOCK_B while processing a BIOS image. We don't
 * do any signing here. We just check to see if the existing FMAP area contains
 * a firmware preamble so we can preserve its contents. We do the signing once
 * we've looked over all the components.
 */
static int fmap_fw_preamble(const char *name, uint8_t *buf, uint32_t len,
			    void *data)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)buf;
	struct sign_state_s *state = (struct sign_state_s *)data;

	/*
	 * If we have a valid keyblock and fw_preamble, then we can use them to
	 * determine the size of the firmware body. Otherwise, we'll have to
	 * just sign the whole region.
	 */
	if (VBOOT_SUCCESS != KeyBlockVerify(key_block, len, NULL, 1)) {
		fprintf(stderr, "Warning: %s keyblock is invalid. "
			"Signing the entire FW FMAP region...\n", name);
		goto whatever;
	}

	RSAPublicKey *rsa = PublicKeyToRSA(&key_block->data_key);
	if (!rsa) {
		fprintf(stderr, "Warning: %s public key is invalid. "
			"Signing the entire FW FMAP region...\n", name);
		goto whatever;
	}
	uint32_t more = key_block->key_block_size;
	VbFirmwarePreambleHeader *preamble =
		(VbFirmwarePreambleHeader *)(buf + more);
	uint32_t fw_size = preamble->body_signature.data_size;
	struct bios_area_s *fw_body_area = 0;

	switch (state->c) {
	case BIOS_FMAP_VBLOCK_A:
		fw_body_area = &state->area[BIOS_FMAP_FW_MAIN_A];
		/* Preserve the flags if they're not specified */
		if (!sign_option.flags_specified)
			sign_option.flags = preamble->flags;
		break;
	case BIOS_FMAP_VBLOCK_B:
		fw_body_area = &state->area[BIOS_FMAP_FW_MAIN_B];
		break;
	default:
		DIE;
	}

	if (fw_size > fw_body_area->len) {
		fprintf(stderr,
			"%s says the firmware is larger than we have\n",
			name);
		return 1;
	}

	/* Update the firmware size */
	fw_body_area->len = fw_size;

whatever:
	state->area[state->c].is_valid = 1;

	return 0;
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


static int write_new_preamble(struct bios_area_s *vblock,
			      struct bios_area_s *fw_body,
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

	preamble = CreateFirmwarePreamble(sign_option.version,
					  sign_option.kernel_subkey,
					  body_sig,
					  signkey,
					  sign_option.flags);
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

static int write_loem(const char *ab, struct bios_area_s *vblock)
{
	char filename[PATH_MAX];
	int n;
	n = snprintf(filename, sizeof(filename), "%s/vblock_%s.%s",
		     sign_option.loemdir ? sign_option.loemdir : ".",
		     ab, sign_option.loemid);
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
static int sign_bios_at_end(struct sign_state_s *state)
{
	struct bios_area_s *vblock_a = &state->area[BIOS_FMAP_VBLOCK_A];
	struct bios_area_s *vblock_b = &state->area[BIOS_FMAP_VBLOCK_B];
	struct bios_area_s *fw_a = &state->area[BIOS_FMAP_FW_MAIN_A];
	struct bios_area_s *fw_b = &state->area[BIOS_FMAP_FW_MAIN_B];
	int retval = 0;

	if (!vblock_a->is_valid || !vblock_b->is_valid ||
	    !fw_a->is_valid || !fw_b->is_valid) {
		fprintf(stderr, "Something's wrong. Not changing anything\n");
		return 1;
	}

	/* Do A & B differ ? */
	if (fw_a->len != fw_b->len ||
	    memcmp(fw_a->buf, fw_b->buf, fw_a->len)) {
		/* Yes, must use DEV keys for A */
		if (!sign_option.devsignprivate || !sign_option.devkeyblock) {
			fprintf(stderr,
				"FW A & B differ. DEV keys are required.\n");
			return 1;
		}
		retval |= write_new_preamble(vblock_a, fw_a,
					     sign_option.devsignprivate,
					     sign_option.devkeyblock);
	} else {
		retval |= write_new_preamble(vblock_a, fw_a,
					     sign_option.signprivate,
					     sign_option.keyblock);
	}

	/* FW B is always normal keys */
	retval |= write_new_preamble(vblock_b, fw_b,
				     sign_option.signprivate,
				     sign_option.keyblock);




	if (sign_option.loemid) {
		retval |= write_loem("A", vblock_a);
		retval |= write_loem("B", vblock_b);
	}

	return retval;
}


int ft_sign_bios(const char *name, uint8_t *buf, uint32_t len, void *data)
{
	FmapHeader *fmap;
	FmapAreaHeader *ah = 0;
	char ah_name[FMAP_NAMELEN + 1];
	int i;
	int retval = 0;
	struct sign_state_s state;

	memset(&state, 0, sizeof(state));

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
			state.area[i].buf = buf + ah->area_offset;
			state.area[i].len = ah->area_size;

			Debug("%s() examining FMAP area %d (%s),"
			      " offset=0x%08x len=0x%08x\n",
			      __func__, i, ah_name,
			      ah->area_offset, ah->area_size);

			/* Go look at it, but abort on error */
			if (fmap_func[i])
				retval += fmap_func[i](ah_name,
						       state.area[i].buf,
						       state.area[i].len,
						       &state);
		}
	}

	retval += sign_bios_at_end(&state);

	return retval;
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

static void (*help_type[NUM_FILE_TYPES])(int argc, char *argv[]) = {
	[FILE_TYPE_PUBKEY] = &print_help_pubkey,
	[FILE_TYPE_RAW_FIRMWARE] = &print_help_raw_firmware,
	[FILE_TYPE_BIOS_IMAGE] = &print_help_bios_image,
	[FILE_TYPE_RAW_KERNEL] = &print_help_raw_kernel,
	[FILE_TYPE_KERN_PREAMBLE] = &print_help_kern_preamble,
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
	"\n"
	"For more information, use \"" MYNAME " help %s TYPE\",\n"
	"where TYPE is one of:\n\n";
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
	{"pem_algo",     1, NULL, OPT_PEM_ALGO},
	{"pem_external", 1, NULL, OPT_PEM_EXTERNAL},
	{"type",         1, NULL, OPT_TYPE},
	{"vblockonly",   0, &sign_option.vblockonly, 1},
	{"help",         0, NULL, OPT_HELP},
	{NULL,           0, NULL, 0},
};
static char *short_opts = ":s:b:k:S:B:v:f:d:l:";

static int do_sign(int argc, char *argv[])
{
	char *infile = 0;
	int i;
	int ifd = -1;
	int errorcnt = 0;
	uint8_t *buf;
	uint32_t buf_len;
	char *e = 0;
	int inout_file_count = 0;
	int mapping;
	int helpind = 0;

	opterr = 0;		/* quiet, you */
	while ((i = getopt_long(argc, argv, short_opts, long_opts, 0)) != -1) {
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
			sign_option.flags = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --flags \"%s\"\n", optarg);
				errorcnt++;
			}
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
			inout_file_count++;
			infile = optarg;
			break;
		case OPT_OUTFILE:
			inout_file_count++;
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
			sign_option.kloadaddr = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --kloadaddr \"%s\"\n", optarg);
				errorcnt++;
			}
			break;
		case OPT_PADDING:
			sign_option.padding = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr,
					"Invalid --padding \"%s\"\n", optarg);
				errorcnt++;
			}
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
			inout_file_count++;
			infile = argv[optind++];
		}
	}

	/* Look for an output file if we don't have one, just in case. */
	if (!sign_option.outfile && argc - optind > 0) {
		inout_file_count++;
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
		if (sign_option.vblockonly || inout_file_count > 1)
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
	default:
		/* Anything else we don't care */
		break;
	}

	Debug("infile=%s\n", infile);
	Debug("inout_file_count=%d\n", inout_file_count);
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
		if (inout_file_count > 1)
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

	errorcnt += futil_unmap_file(ifd, MAP_RW, buf, buf_len);

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
