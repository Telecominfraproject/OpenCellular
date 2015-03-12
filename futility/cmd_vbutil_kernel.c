/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Verified boot kernel utility
 */

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>		/* For PRIu64 */
#ifndef HAVE_MACOS
#include <linux/fs.h>		/* For BLKGETSIZE64 */
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_type.h"
#include "futility.h"
#include "host_common.h"
#include "kernel_blob.h"
#include "traversal.h"
#include "vb1_helper.h"

static void Fatal(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(1);
}

/* Global opts */
static int opt_verbose;
static int opt_vblockonly;
static uint64_t opt_pad = 65536;

/* Command line options */
enum {
	OPT_MODE_PACK = 1000,
	OPT_MODE_REPACK,
	OPT_MODE_VERIFY,
	OPT_MODE_GET_VMLINUZ,
	OPT_ARCH,
	OPT_OLDBLOB,
	OPT_KLOADADDR,
	OPT_KEYBLOCK,
	OPT_SIGNPUBKEY,
	OPT_SIGNPRIVATE,
	OPT_VERSION,
	OPT_VMLINUZ,
	OPT_BOOTLOADER,
	OPT_CONFIG,
	OPT_VBLOCKONLY,
	OPT_PAD,
	OPT_VERBOSE,
	OPT_MINVERSION,
	OPT_VMLINUZ_OUT,
	OPT_FLAGS,
};

static const struct option long_opts[] = {
	{"pack", 1, 0, OPT_MODE_PACK},
	{"repack", 1, 0, OPT_MODE_REPACK},
	{"verify", 1, 0, OPT_MODE_VERIFY},
	{"get-vmlinuz", 1, 0, OPT_MODE_GET_VMLINUZ},
	{"arch", 1, 0, OPT_ARCH},
	{"oldblob", 1, 0, OPT_OLDBLOB},
	{"kloadaddr", 1, 0, OPT_KLOADADDR},
	{"keyblock", 1, 0, OPT_KEYBLOCK},
	{"signpubkey", 1, 0, OPT_SIGNPUBKEY},
	{"signprivate", 1, 0, OPT_SIGNPRIVATE},
	{"version", 1, 0, OPT_VERSION},
	{"minversion", 1, 0, OPT_MINVERSION},
	{"vmlinuz", 1, 0, OPT_VMLINUZ},
	{"bootloader", 1, 0, OPT_BOOTLOADER},
	{"config", 1, 0, OPT_CONFIG},
	{"vblockonly", 0, 0, OPT_VBLOCKONLY},
	{"pad", 1, 0, OPT_PAD},
	{"verbose", 0, &opt_verbose, 1},
	{"debug", 0, &debugging_enabled, 1},
	{"vmlinuz-out", 1, 0, OPT_VMLINUZ_OUT},
	{"flags", 1, 0, OPT_FLAGS},
	{NULL, 0, 0, 0}
};



static const char usage[] =
	"\n"
	"Usage:  " MYNAME " %s --pack <file> [PARAMETERS]\n"
	"\n"
	"  Required parameters:\n"
	"    --keyblock <file>         Key block in .keyblock format\n"
	"    --signprivate <file>      Private key to sign kernel data,\n"
	"                                in .vbprivk format\n"
	"    --version <number>        Kernel version\n"
	"    --vmlinuz <file>          Linux kernel bzImage file\n"
	"    --bootloader <file>       Bootloader stub\n"
	"    --config <file>           Command line file\n"
	"    --arch <arch>             Cpu architecture (default x86)\n"
	"\n"
	"  Optional:\n"
	"    --kloadaddr <address>     Assign kernel body load address\n"
	"    --pad <number>            Verification padding size in bytes\n"
	"    --vblockonly              Emit just the verification blob\n"
	"    --flags NUM               Flags to be passed in the header\n"
	"\nOR\n\n"
	"Usage:  " MYNAME " %s --repack <file> [PARAMETERS]\n"
	"\n"
	"  Required parameters:\n"
	"    --signprivate <file>      Private key to sign kernel data,\n"
	"                                in .vbprivk format\n"
	"    --oldblob <file>          Previously packed kernel blob\n"
	"                                (including verfication blob)\n"
	"\n"
	"  Optional:\n"
	"    --keyblock <file>         Key block in .keyblock format\n"
	"    --config <file>           New command line file\n"
	"    --version <number>        Kernel version\n"
	"    --kloadaddr <address>     Assign kernel body load address\n"
	"    --pad <number>            Verification blob size in bytes\n"
	"    --vblockonly              Emit just the verification blob\n"
	"\nOR\n\n"
	"Usage:  " MYNAME " %s --verify <file> [PARAMETERS]\n"
	"\n"
	"  Optional:\n"
	"    --signpubkey <file>"
	"       Public key to verify kernel keyblock,\n"
	"                                in .vbpubk format\n"
	"    --verbose                 Print a more detailed report\n"
	"    --keyblock <file>         Outputs the verified key block,\n"
	"                                in .keyblock format\n"
	"    --pad <number>            Verification padding size in bytes\n"
	"    --minversion <number>     Minimum combined kernel key version\n"
	"\nOR\n\n"
	"Usage:  " MYNAME " %s --get-vmlinuz <file> [PARAMETERS]\n"
	"\n"
	"  Required parameters:\n"
	"    --vmlinuz-out <file>      vmlinuz image output file\n"
	"\n";


/* Print help and return error */
static void print_help(const char *progname)
{
	printf(usage, progname, progname, progname, progname);
}


/* Return an explanation when fread() fails. */
static const char *error_fread(FILE *fp)
{
	const char *retval = "beats me why";
	if (feof(fp))
		retval = "EOF";
	else if (ferror(fp))
		retval = strerror(errno);
	clearerr(fp);
	return retval;
}


/* This reads a complete kernel partition into a buffer */
static uint8_t *ReadOldKPartFromFileOrDie(const char *filename,
					 uint64_t *size_ptr)
{
	FILE *fp = NULL;
	struct stat statbuf;
	uint8_t *buf;
	uint64_t file_size = 0;

	if (0 != stat(filename, &statbuf))
		Fatal("Unable to stat %s: %s\n", filename, strerror(errno));

	if (S_ISBLK(statbuf.st_mode)) {
#ifndef HAVE_MACOS
		int fd = open(filename, O_RDONLY);
		if (fd >= 0) {
			ioctl(fd, BLKGETSIZE64, &file_size);
			close(fd);
		}
#endif
	} else {
		file_size = statbuf.st_size;
	}
	Debug("%s size is 0x%" PRIx64 "\n", filename, file_size);
	if (file_size < opt_pad)
		Fatal("%s is too small to be a valid kernel blob\n");

	Debug("Reading %s\n", filename);
	fp = fopen(filename, "rb");
	if (!fp)
		Fatal("Unable to open file %s: %s\n", filename,
		      strerror(errno));

	buf = malloc(file_size);
	if (1 != fread(buf, file_size, 1, fp))
		Fatal("Unable to read entirety of %s: %s\n", filename,
		      error_fread(fp));

	if (size_ptr)
		*size_ptr = file_size;

	return buf;
}

/****************************************************************************/

static int do_vbutil_kernel(int argc, char *argv[])
{
	char *filename = NULL;
	char *oldfile = NULL;
	char *keyblock_file = NULL;
	char *signpubkey_file = NULL;
	char *signprivkey_file = NULL;
	char *version_str = NULL;
	int version = -1;
	char *vmlinuz_file = NULL;
	char *bootloader_file = NULL;
	char *config_file = NULL;
	char *vmlinuz_out_file = NULL;
	enum arch_t arch = ARCH_X86;
	uint64_t kernel_body_load_address = CROS_32BIT_ENTRY_ADDR;
	int mode = 0;
	int parse_error = 0;
	uint64_t min_version = 0;
	char *e;
	int i = 0;
	int errcount = 0;
	int rv;
	VbKeyBlockHeader *keyblock = NULL;
	VbKeyBlockHeader *t_keyblock = NULL;
	VbPrivateKey *signpriv_key = NULL;
	VbPublicKey *signpub_key = NULL;
	uint8_t *kpart_data = NULL;
	uint64_t kpart_size = 0;
	uint8_t *vmlinuz_buf = NULL;
	uint64_t vmlinuz_size = 0;
	uint8_t *t_config_data;
	uint64_t t_config_size;
	uint8_t *t_bootloader_data;
	uint64_t t_bootloader_size;
	uint64_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;
	uint64_t vmlinuz_header_offset = 0;
	VbKernelPreambleHeader *preamble = NULL;
	uint8_t *kblob_data = NULL;
	uint64_t kblob_size = 0;
	uint8_t *vblock_data = NULL;
	uint64_t vblock_size = 0;
	uint32_t flags = 0;
	FILE *f;

	while (((i = getopt_long(argc, argv, ":", long_opts, NULL)) != -1) &&
	       !parse_error) {
		switch (i) {
		default:
		case '?':
			/* Unhandled option */
			parse_error = 1;
			break;

		case 0:
			/* silently handled option */
			break;

		case OPT_MODE_PACK:
		case OPT_MODE_REPACK:
		case OPT_MODE_VERIFY:
		case OPT_MODE_GET_VMLINUZ:
			if (mode && (mode != i)) {
				fprintf(stderr,
					"Only one mode can be specified\n");
				parse_error = 1;
				break;
			}
			mode = i;
			filename = optarg;
			break;

		case OPT_ARCH:
			/* check the first 3 characters to also detect x86_64 */
			if ((!strncasecmp(optarg, "x86", 3)) ||
			    (!strcasecmp(optarg, "amd64")))
				arch = ARCH_X86;
			else if ((!strcasecmp(optarg, "arm")) ||
				 (!strcasecmp(optarg, "aarch64")))
				arch = ARCH_ARM;
			else if (!strcasecmp(optarg, "mips"))
				arch = ARCH_MIPS;
			else {
				fprintf(stderr,
					"Unknown architecture string: %s\n",
					optarg);
				parse_error = 1;
			}
			break;

		case OPT_OLDBLOB:
			oldfile = optarg;
			break;

		case OPT_KLOADADDR:
			kernel_body_load_address = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --kloadaddr\n");
				parse_error = 1;
			}
			break;

		case OPT_KEYBLOCK:
			keyblock_file = optarg;
			break;

		case OPT_SIGNPUBKEY:
			signpubkey_file = optarg;
			break;

		case OPT_SIGNPRIVATE:
			signprivkey_file = optarg;
			break;

		case OPT_VMLINUZ:
			vmlinuz_file = optarg;
			break;

		case OPT_FLAGS:
			flags = (uint32_t)strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --flags\n");
				parse_error = 1;
			}
			break;

		case OPT_BOOTLOADER:
			bootloader_file = optarg;
			break;

		case OPT_CONFIG:
			config_file = optarg;
			break;

		case OPT_VBLOCKONLY:
			opt_vblockonly = 1;
			break;

		case OPT_VERSION:
			version_str = optarg;
			version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --version\n");
				parse_error = 1;
			}
			break;

		case OPT_MINVERSION:
			min_version = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --minversion\n");
				parse_error = 1;
			}
			break;

		case OPT_PAD:
			opt_pad = strtoul(optarg, &e, 0);
			if (!*optarg || (e && *e)) {
				fprintf(stderr, "Invalid --pad\n");
				parse_error = 1;
			}
			break;
		case OPT_VMLINUZ_OUT:
			vmlinuz_out_file = optarg;
		}
	}

	if (parse_error) {
		print_help(argv[0]);
		return 1;
	}

	switch (mode) {
	case OPT_MODE_PACK:

		if (!keyblock_file)
			Fatal("Missing required keyblock file.\n");

		t_keyblock = (VbKeyBlockHeader *)ReadFile(keyblock_file, 0);
		if (!t_keyblock)
			Fatal("Error reading key block.\n");

		if (!signprivkey_file)
			Fatal("Missing required signprivate file.\n");

		signpriv_key = PrivateKeyRead(signprivkey_file);
		if (!signpriv_key)
			Fatal("Error reading signing key.\n");

		if (!config_file)
			Fatal("Missing required config file.\n");

		Debug("Reading %s\n", config_file);
		t_config_data =
			ReadConfigFile(config_file, &t_config_size);
		if (!t_config_data)
			Fatal("Error reading config file.\n");

		if (!bootloader_file)
			Fatal("Missing required bootloader file.\n");

		Debug("Reading %s\n", bootloader_file);
		t_bootloader_data = ReadFile(bootloader_file,
					     &t_bootloader_size);
		if (!t_bootloader_data)
			Fatal("Error reading bootloader file.\n");
		Debug(" bootloader file size=0x%" PRIx64 "\n",
		      t_bootloader_size);

		if (!vmlinuz_file)
			Fatal("Missing required vmlinuz file.\n");
		Debug("Reading %s\n", vmlinuz_file);
		vmlinuz_buf = ReadFile(vmlinuz_file, &vmlinuz_size);
		if (!vmlinuz_buf)
			Fatal("Error reading vmlinuz file.\n");
		Debug(" vmlinuz file size=0x%" PRIx64 "\n",
		      vmlinuz_size);
		if (!vmlinuz_size)
			Fatal("Empty vmlinuz file\n");

		kblob_data = CreateKernelBlob(
			vmlinuz_buf, vmlinuz_size,
			arch, kernel_body_load_address,
			t_config_data, t_config_size,
			t_bootloader_data, t_bootloader_size,
			&kblob_size);
		if (!kblob_data)
			Fatal("Unable to create kernel blob\n");

		Debug("kblob_size = 0x%" PRIx64 "\n", kblob_size);

		vblock_data = SignKernelBlob(kblob_data, kblob_size, opt_pad,
					     version, kernel_body_load_address,
					     t_keyblock, signpriv_key, flags,
					     &vblock_size);
		if (!vblock_data)
			Fatal("Unable to sign kernel blob\n");

		Debug("vblock_size = 0x%" PRIx64 "\n", vblock_size);

		if (opt_vblockonly)
			rv = WriteSomeParts(filename,
					    vblock_data, vblock_size,
					    NULL, 0);
		else
			rv = WriteSomeParts(filename,
					    vblock_data, vblock_size,
					    kblob_data, kblob_size);
		return rv;

	case OPT_MODE_REPACK:

		/* Required */

		if (!signprivkey_file)
			Fatal("Missing required signprivate file.\n");

		signpriv_key = PrivateKeyRead(signprivkey_file);
		if (!signpriv_key)
			Fatal("Error reading signing key.\n");

		if (!oldfile)
			Fatal("Missing previously packed blob.\n");

		/* Load the kernel partition */
		kpart_data = ReadOldKPartFromFileOrDie(oldfile, &kpart_size);

		/* Make sure we have a kernel partition */
		if (FILE_TYPE_KERN_PREAMBLE !=
		    futil_file_type_buf(kpart_data, kpart_size))
			Fatal("%s is not a kernel blob\n", oldfile);

		kblob_data = UnpackKPart(kpart_data, kpart_size, opt_pad,
					 &keyblock, &preamble, &kblob_size);

		if (!kblob_data)
			Fatal("Unable to unpack kernel partition\n");

		kernel_body_load_address = preamble->body_load_address;

		/* Update the config if asked */
		if (config_file) {
			Debug("Reading %s\n", config_file);
			t_config_data =
				ReadConfigFile(config_file, &t_config_size);
			if (!t_config_data)
				Fatal("Error reading config file.\n");
			if (0 != UpdateKernelBlobConfig(
				    kblob_data, kblob_size,
				    t_config_data, t_config_size))
				Fatal("Unable to update config\n");
		}

		if (!version_str)
			version = preamble->kernel_version;

		if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS)
			flags = preamble->flags;

		if (keyblock_file) {
			t_keyblock =
				(VbKeyBlockHeader *)ReadFile(keyblock_file, 0);
			if (!t_keyblock)
				Fatal("Error reading key block.\n");
		}

		/* Reuse previous body size */
		vblock_data = SignKernelBlob(kblob_data, kblob_size, opt_pad,
					     version, kernel_body_load_address,
					     t_keyblock ? t_keyblock : keyblock,
					     signpriv_key, flags, &vblock_size);
		if (!vblock_data)
			Fatal("Unable to sign kernel blob\n");

		if (opt_vblockonly)
			rv = WriteSomeParts(filename,
					    vblock_data, vblock_size,
					    NULL, 0);
		else
			rv = WriteSomeParts(filename,
					    vblock_data, vblock_size,
					    kblob_data, kblob_size);
		return rv;

	case OPT_MODE_VERIFY:

		/* Optional */

		if (signpubkey_file) {
			signpub_key = PublicKeyRead(signpubkey_file);
			if (!signpub_key)
				Fatal("Error reading public key.\n");
		}

		/* Do it */

		/* Load the kernel partition */
		kpart_data = ReadOldKPartFromFileOrDie(filename, &kpart_size);

		kblob_data = UnpackKPart(kpart_data, kpart_size, opt_pad,
					 0, 0, &kblob_size);
		if (!kblob_data)
			Fatal("Unable to unpack kernel partition\n");

		rv = VerifyKernelBlob(kblob_data, kblob_size,
				      signpub_key, keyblock_file, min_version);

		return rv;

	case OPT_MODE_GET_VMLINUZ:

		if (!vmlinuz_out_file) {
			fprintf(stderr,
				"USE: vbutil_kernel --get-vmlinuz <file> "
				"--vmlinuz-out <file>\n");
			print_help(argv[0]);
			return 1;
		}

		kpart_data = ReadOldKPartFromFileOrDie(filename, &kpart_size);

		kblob_data = UnpackKPart(kpart_data, kpart_size, opt_pad,
					 &keyblock, &preamble, &kblob_size);

		if (!kblob_data)
			Fatal("Unable to unpack kernel partition\n");

		f = fopen(vmlinuz_out_file, "wb");
		if (!f) {
			VbExError("Can't open output file %s\n",
				  vmlinuz_out_file);
			return 1;
		}

		/* Now stick 16-bit header followed by kernel block into
		   output */
		if (VbGetKernelVmlinuzHeader(preamble,
					     &vmlinuz_header_address,
					     &vmlinuz_header_size)
		    != VBOOT_SUCCESS) {
			Fatal("Unable to retrieve Vmlinuz Header!");
		}

		if (vmlinuz_header_size) {
			// verify that the 16-bit header is included in the
			// kblob (to make sure that it's included in the
			// signature)
			if (VerifyVmlinuzInsideKBlob(preamble->body_load_address,
						     kblob_size,
						     vmlinuz_header_address,
						     vmlinuz_header_size)) {
				VbExError("Vmlinuz header not signed!\n");
				fclose(f);
				unlink(vmlinuz_out_file);
				return 1;
			}
			// calculate the vmlinuz_header offset from
			// the beginning of the kpart_data.  The kblob doesn't
			// include the body_load_offset, but does include
			// the keyblock and preamble sections.
			vmlinuz_header_offset = vmlinuz_header_address -
				preamble->body_load_address +
				keyblock->key_block_size +
				preamble->preamble_size;
			errcount |=
				(1 != fwrite(kpart_data + vmlinuz_header_offset,
					     vmlinuz_header_size,
					     1,
					     f));
		}
		errcount |= (1 != fwrite(kblob_data,
					 kblob_size,
					 1,
					 f));
		if (errcount) {
			VbExError("Can't write output file %s\n",
				  vmlinuz_out_file);
			fclose(f);
			unlink(vmlinuz_out_file);
			return 1;
		}

		fclose(f);
		return 0;
	}

	fprintf(stderr,
		"You must specify a mode: "
		"--pack, --repack, --verify, or --get-vmlinuz\n");
	print_help(argv[0]);
	return 1;
}

DECLARE_FUTIL_COMMAND(vbutil_kernel, do_vbutil_kernel,
		      VBOOT_VERSION_1_0,
		      "Creates, signs, and verifies the kernel partition",
		      print_help);
