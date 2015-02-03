/*
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <inttypes.h>		/* For PRIu64 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/rsa.h>

#include "file_type.h"
#include "futility.h"
#include "host_common.h"
#include "kernel_blob.h"
#include "util_misc.h"
#include "vb1_helper.h"

/****************************************************************************/
/* Here are globals containing all the bits & pieces I'm working on.
 *
 * kernel vblock    = keyblock + kernel preamble + padding to 64K (or whatever)
 * kernel blob      = 32-bit kernel + config file + params + bootloader stub +
 *                    vmlinuz_header
 * kernel partition = kernel vblock + kernel blob
 *
 * The VbKernelPreambleHeader.preamble_size includes the padding.
 */

/* The keyblock, preamble, and kernel blob are kept in separate places. */
static VbKeyBlockHeader *g_keyblock;
static VbKernelPreambleHeader *g_preamble;
static uint8_t *g_kernel_blob_data;
static uint64_t g_kernel_blob_size;

/* These refer to individual parts within the kernel blob. */
static uint8_t *g_kernel_data;
static uint64_t g_kernel_size;
static uint8_t *g_config_data;
static uint64_t g_config_size;
static uint8_t *g_param_data;
static uint64_t g_param_size;
static uint8_t *g_bootloader_data;
static uint64_t g_bootloader_size;
static uint8_t *g_vmlinuz_header_data;
static uint64_t g_vmlinuz_header_size;

static uint64_t g_ondisk_bootloader_addr;
static uint64_t g_ondisk_vmlinuz_header_addr;


/*
 * Read the kernel command line from a file. Get rid of \n characters along
 * the way and verify that the line fits into a 4K buffer.
 *
 * Return the buffer contaning the line on success (and set the line length
 * using the passed in parameter), or NULL in case something goes wrong.
 */
uint8_t *ReadConfigFile(const char *config_file, uint64_t *config_size)
{
	uint8_t *config_buf;
	int i;

	config_buf = ReadFile(config_file, config_size);
	if (!config_buf)
		return NULL;
	Debug(" config file size=0x%" PRIx64 "\n", *config_size);
	if (CROS_CONFIG_SIZE <= *config_size) {	/* room for trailing '\0' */
		fprintf(stderr, "Config file %s is too large (>= %d bytes)\n",
			config_file, CROS_CONFIG_SIZE);
		return NULL;
	}

	/* Replace newlines with spaces */
	for (i = 0; i < *config_size; i++)
		if ('\n' == config_buf[i])
			config_buf[i] = ' ';

	return config_buf;
}

/****************************************************************************/

/* Return the smallest integral multiple of [alignment] that is equal
 * to or greater than [val]. Used to determine the number of
 * pages/sectors/blocks/whatever needed to contain [val]
 * items/bytes/etc. */
static uint64_t roundup(uint64_t val, uint64_t alignment)
{
	uint64_t rem = val % alignment;
	if (rem)
		return val + (alignment - rem);
	return val;
}

/* Match regexp /\b--\b/ to delimit the start of the kernel commandline. If we
 * don't find one, we'll use the whole thing. */
static unsigned int find_cmdline_start(uint8_t *buf_ptr, unsigned int max_len)
{
	char *input = (char *)buf_ptr;
	int start = 0;
	int i;
	for (i = 0; i < max_len - 1 && input[i]; i++) {
		if ('-' == input[i] && '-' == input[i + 1]) {
			if ((i == 0 || ' ' == input[i - 1]) &&
			    (i + 2 >= max_len || ' ' == input[i + 2])) {
				/* found "--" with nothing before or after it */
				start = i + 2;	/* hope for a trailing '\0' */
				break;
			}
		}
	}
	while (' ' == input[start])	/* skip leading spaces */
		start++;

	return start;
}

/* Offset of kernel command line string from the start of the kernel blob */
uint64_t KernelCmdLineOffset(VbKernelPreambleHeader *preamble)
{
	return preamble->bootloader_address - preamble->body_load_address -
	    CROS_CONFIG_SIZE - CROS_PARAMS_SIZE;
}

/* Returns the size of the 32-bit kernel, or negative on error. */
static int KernelSize(uint8_t *kernel_buf, uint64_t kernel_size,
		      enum arch_t arch)
{
	uint64_t kernel32_start = 0;
	struct linux_kernel_params *lh;

	/* Except for x86, the kernel is the kernel. */
	if (arch != ARCH_X86)
		return kernel_size;

	/* The first part of the x86 vmlinuz is a header, followed by
	 * a real-mode boot stub. We only want the 32-bit part. */
	lh = (struct linux_kernel_params *)kernel_buf;
	kernel32_start = (lh->setup_sects + 1) << 9;
	if (kernel32_start >= kernel_size) {
		fprintf(stderr, "Malformed kernel\n");
		return -1;
	}
	return kernel_size - kernel32_start;
}

/* This extracts g_kernel_* and g_param_* from a standard vmlinuz file.
 * It returns nonzero on error. */
static int PickApartVmlinuz(uint8_t *kernel_buf, uint64_t kernel_size,
			    enum arch_t arch,
			    uint64_t kernel_body_load_address)
{
	uint64_t kernel32_start = 0;
	uint64_t kernel32_size = kernel_size;
	struct linux_kernel_params *lh, *params;

	/* Except for x86, the kernel is the kernel. */
	if (arch == ARCH_X86) {
		/* The first part of the x86 vmlinuz is a header, followed by
		 * a real-mode boot stub. We only want the 32-bit part. */
		lh = (struct linux_kernel_params *)kernel_buf;
		kernel32_start = (lh->setup_sects + 1) << 9;
		if (kernel32_start >= kernel_size) {
			fprintf(stderr, "Malformed kernel\n");
			return -1;
		}
		kernel32_size = kernel_size - kernel32_start;

		Debug(" kernel16_start=0x%" PRIx64 "\n", 0);
		Debug(" kernel16_size=0x%" PRIx64 "\n", kernel32_start);

		/* Copy the original zeropage data from kernel_buf into
		 * g_param_data, then tweak a few fields for our purposes */
		params = (struct linux_kernel_params *)(g_param_data);
		Memcpy(&(params->setup_sects), &(lh->setup_sects),
		       offsetof(struct linux_kernel_params, e820_entries)
		       - offsetof(struct linux_kernel_params, setup_sects));
		params->boot_flag = 0;
		params->ramdisk_image = 0;	/* we don't support initrd */
		params->ramdisk_size = 0;
		params->type_of_loader = 0xff;
		/* We need to point to the kernel commandline arg. On disk, it
		 * will come right after the 32-bit part of the kernel. */
		params->cmd_line_ptr = kernel_body_load_address +
			roundup(kernel32_size, CROS_ALIGN) +
			find_cmdline_start(g_config_data, g_config_size);
		Debug(" cmdline_addr=0x%x\n", params->cmd_line_ptr);
		Debug(" version=0x%x\n", params->version);
		Debug(" kernel_alignment=0x%x\n", params->kernel_alignment);
		Debug(" relocatable_kernel=0x%x\n", params->relocatable_kernel);
		/* Add a fake e820 memory map with 2 entries. */
		params->n_e820_entry = 2;
		params->e820_entries[0].start_addr = 0x00000000;
		params->e820_entries[0].segment_size = 0x00001000;
		params->e820_entries[0].segment_type = E820_TYPE_RAM;
		params->e820_entries[1].start_addr = 0xfffff000;
		params->e820_entries[1].segment_size = 0x00001000;
		params->e820_entries[1].segment_type = E820_TYPE_RESERVED;
	}

	Debug(" kernel32_start=0x%" PRIx64 "\n", kernel32_start);
	Debug(" kernel32_size=0x%" PRIx64 "\n", kernel32_size);

	/* Keep just the 32-bit kernel. */
	if (kernel32_size) {
		g_kernel_size = kernel32_size;
		Memcpy(g_kernel_data, kernel_buf + kernel32_start,
		       g_kernel_size);
	}

	/* done */
	return 0;
}

/* Split a kernel blob into separate g_kernel, g_param, g_config,
 * g_bootloader, and g_vmlinuz_header parts. */
static void UnpackKernelBlob(uint8_t *kernel_blob_data)
{
	uint64_t now;
	uint64_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;

	/* We have to work backwards from the end, because the preamble
	   only describes the bootloader and vmlinuz stubs. */

	/* Vmlinuz Header is at the end */
	if (VbGetKernelVmlinuzHeader(g_preamble,
				     &vmlinuz_header_address,
				     &vmlinuz_header_size)
	    != VBOOT_SUCCESS) {
		fprintf(stderr, "Unable to retrieve Vmlinuz Header!");
		return;
	}
	if (vmlinuz_header_size) {
		now = vmlinuz_header_address - g_preamble->body_load_address;
		g_vmlinuz_header_size = vmlinuz_header_size;
		g_vmlinuz_header_data = kernel_blob_data + now;

		Debug("vmlinuz_header_size     = 0x%" PRIx64 "\n",
		      g_vmlinuz_header_size);
		Debug("vmlinuz_header_ofs      = 0x%" PRIx64 "\n", now);
	}

	/* Where does the bootloader stub begin? */
	now = g_preamble->bootloader_address - g_preamble->body_load_address;

	/* Bootloader is at the end */
	g_bootloader_size = g_preamble->bootloader_size;
	g_bootloader_data = kernel_blob_data + now;
	/* TODO: What to do if this is beyond the end of the blob? */

	Debug("bootloader_size     = 0x%" PRIx64 "\n", g_bootloader_size);
	Debug("bootloader_ofs      = 0x%" PRIx64 "\n", now);

	/* Before that is the params */
	now -= CROS_PARAMS_SIZE;
	g_param_size = CROS_PARAMS_SIZE;
	g_param_data = kernel_blob_data + now;
	Debug("param_ofs           = 0x%" PRIx64 "\n", now);

	/* Before that is the config */
	now -= CROS_CONFIG_SIZE;
	g_config_size = CROS_CONFIG_SIZE;
	g_config_data = kernel_blob_data + now;
	Debug("config_ofs          = 0x%" PRIx64 "\n", now);

	/* The kernel starts at offset 0 and extends up to the config */
	g_kernel_data = kernel_blob_data;
	g_kernel_size = now;
	Debug("kernel_size         = 0x%" PRIx64 "\n", g_kernel_size);
}


/* Replaces the config section of the specified kernel blob.
 * Return nonzero on error. */
int UpdateKernelBlobConfig(uint8_t *kblob_data, uint64_t kblob_size,
			   uint8_t *config_data, uint64_t config_size)
{
	/* We should have already examined this blob. If not, we could do it
	 * again, but it's more likely due to an error. */
	if (kblob_data != g_kernel_blob_data ||
	    kblob_size != g_kernel_blob_size) {
		fprintf(stderr, "Trying to update some other blob\n");
		return -1;
	}

	Memset(g_config_data, 0, g_config_size);
	Memcpy(g_config_data, config_data, config_size);

	return 0;
}

/* Split a kernel partition into separate vblock and blob parts. */
uint8_t *UnpackKPart(uint8_t *kpart_data, uint64_t kpart_size,
		     uint64_t padding,
		     VbKeyBlockHeader **keyblock_ptr,
		     VbKernelPreambleHeader **preamble_ptr,
		     uint64_t *blob_size_ptr)
{
	VbKeyBlockHeader *keyblock;
	VbKernelPreambleHeader *preamble;
	uint64_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;
	uint64_t now = 0;
	uint32_t flags = 0;

	/* Sanity-check the keyblock */
	keyblock = (VbKeyBlockHeader *)kpart_data;
	Debug("Keyblock is 0x%" PRIx64 " bytes\n", keyblock->key_block_size);
	now += keyblock->key_block_size;
	if (now > kpart_size) {
		fprintf(stderr,
			"key_block_size advances past the end of the blob\n");
		return NULL;
	}
	if (now > padding) {
		fprintf(stderr,
			"key_block_size advances past %" PRIu64
			" byte padding\n",
			padding);
		return NULL;
	}

	/* LGTM */
	g_keyblock = keyblock;

	/* And the preamble */
	preamble = (VbKernelPreambleHeader *)(kpart_data + now);
	Debug("Preamble is 0x%" PRIx64 " bytes\n", preamble->preamble_size);
	now += preamble->preamble_size;
	if (now > kpart_size) {
		fprintf(stderr,
			"preamble_size advances past the end of the blob\n");
		return NULL;
	}
	if (now > padding) {
		fprintf(stderr, "preamble_size advances past %" PRIu64
			" byte padding\n", padding);
		return NULL;
	}
	/* LGTM */
	Debug(" kernel_version = %d\n", preamble->kernel_version);
	Debug(" bootloader_address = 0x%" PRIx64 "\n",
	      preamble->bootloader_address);
	Debug(" bootloader_size = 0x%" PRIx64 "\n", preamble->bootloader_size);
	Debug(" kern_blob_size = 0x%" PRIx64 "\n",
	      preamble->body_signature.data_size);

	if (VbKernelHasFlags(preamble) == VBOOT_SUCCESS)
		flags = preamble->flags;
	Debug(" flags = 0x%" PRIx32 "\n", flags);

	g_preamble = preamble;
	g_ondisk_bootloader_addr = g_preamble->bootloader_address;

	if (VbGetKernelVmlinuzHeader(preamble,
				     &vmlinuz_header_address,
				     &vmlinuz_header_size)
	    != VBOOT_SUCCESS) {
		fprintf(stderr, "Unable to retrieve Vmlinuz Header!");
		return NULL;
	}
	if (vmlinuz_header_size) {
		Debug(" vmlinuz_header_address = 0x%" PRIx64 "\n",
		      vmlinuz_header_address);
		Debug(" vmlinuz_header_size = 0x%" PRIx64 "\n",
		      vmlinuz_header_size);
		g_ondisk_vmlinuz_header_addr = vmlinuz_header_address;
	}

	Debug("kernel blob is at offset 0x%" PRIx64 "\n", now);
	g_kernel_blob_data = kpart_data + now;
	g_kernel_blob_size = preamble->body_signature.data_size;

	/* Sanity check */
	if (g_kernel_blob_size < preamble->body_signature.data_size)
		fprintf(stderr,
			"Warning: kernel file only has 0x%" PRIx64 " bytes\n",
			g_kernel_blob_size);

	/* Update the blob pointers */
	UnpackKernelBlob(g_kernel_blob_data);

	if (keyblock_ptr)
		*keyblock_ptr = keyblock;
	if (preamble_ptr)
		*preamble_ptr = preamble;
	if (blob_size_ptr)
		*blob_size_ptr = g_kernel_blob_size;

	return g_kernel_blob_data;
}

uint8_t *SignKernelBlob(uint8_t *kernel_blob, uint64_t kernel_size,
			uint64_t padding,
			int version, uint64_t kernel_body_load_address,
			VbKeyBlockHeader *keyblock, VbPrivateKey *signpriv_key,
			uint32_t flags, uint64_t *vblock_size_ptr)
{
	VbSignature *body_sig;
	VbKernelPreambleHeader *preamble;
	uint64_t min_size = padding > keyblock->key_block_size
		? padding - keyblock->key_block_size : 0;
	void *outbuf;
	uint64_t outsize;

	/* Sign the kernel data */
	body_sig = CalculateSignature(kernel_blob, kernel_size, signpriv_key);
	if (!body_sig) {
		fprintf(stderr, "Error calculating body signature\n");
		return NULL;
	}

	/* Create preamble */
	preamble = CreateKernelPreamble(version,
					kernel_body_load_address,
					g_ondisk_bootloader_addr,
					g_bootloader_size,
					body_sig,
					g_ondisk_vmlinuz_header_addr,
					g_vmlinuz_header_size,
					flags,
					min_size,
					signpriv_key);
	if (!preamble) {
		fprintf(stderr, "Error creating preamble.\n");
		return 0;
	}

	outsize = keyblock->key_block_size + preamble->preamble_size;
	outbuf = malloc(outsize);
	Memset(outbuf, 0, outsize);
	Memcpy(outbuf, keyblock, keyblock->key_block_size);
	Memcpy(outbuf + keyblock->key_block_size,
	       preamble, preamble->preamble_size);

	if (vblock_size_ptr)
		*vblock_size_ptr = outsize;
	return outbuf;
}

/* Returns zero on success */
int WriteSomeParts(const char *outfile,
		   void *part1_data, uint64_t part1_size,
		   void *part2_data, uint64_t part2_size)
{
	FILE *f;

	/* Write the output file */
	Debug("writing %s with 0x%" PRIx64 ", 0x%" PRIx64 "\n",
	      outfile, part1_size, part2_size);

	f = fopen(outfile, "wb");
	if (!f) {
		fprintf(stderr, "Can't open output file %s: %s\n",
			outfile, strerror(errno));
		return -1;
	}

	if (part1_data && part1_size) {
		if (1 != fwrite(part1_data, part1_size, 1, f)) {
			fprintf(stderr, "Can't write output file %s: %s\n",
				outfile, strerror(errno));
			fclose(f);
			unlink(outfile);
			return -1;
		}
	}

	if (part2_data && part2_size) {
		if (1 != fwrite(part2_data, part2_size, 1, f)) {
			fprintf(stderr, "Can't write output file %s: %s\n",
				outfile, strerror(errno));
			fclose(f);
			unlink(outfile);
			return -1;
		}
	}

	fclose(f);

	/* Success */
	return 0;
}

/* Returns 0 on success */
int VerifyKernelBlob(uint8_t *kernel_blob,
		     uint64_t kernel_size,
		     VbPublicKey *signpub_key,
		     const char *keyblock_outfile,
		     uint64_t min_version)
{
	VbPublicKey *data_key;
	RSAPublicKey *rsa;
	int rv = -1;
	uint64_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;

	if (0 != KeyBlockVerify(g_keyblock, g_keyblock->key_block_size,
				signpub_key, (0 == signpub_key))) {
		fprintf(stderr, "Error verifying key block.\n");
		goto done;
	}

	printf("Key block:\n");
	data_key = &g_keyblock->data_key;
	printf("  Signature:           %s\n",
	       signpub_key ? "valid" : "ignored");
	printf("  Size:                0x%" PRIx64 "\n",
	       g_keyblock->key_block_size);
	printf("  Flags:               %" PRIu64 " ",
	       g_keyblock->key_block_flags);
	if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_0)
		printf(" !DEV");
	if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_DEVELOPER_1)
		printf(" DEV");
	if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_0)
		printf(" !REC");
	if (g_keyblock->key_block_flags & KEY_BLOCK_FLAG_RECOVERY_1)
		printf(" REC");
	printf("\n");
	printf("  Data key algorithm:  %" PRIu64 " %s\n", data_key->algorithm,
	       (data_key->algorithm < kNumAlgorithms ?
		algo_strings[data_key->algorithm] : "(invalid)"));
	printf("  Data key version:    %" PRIu64 "\n", data_key->key_version);
	printf("  Data key sha1sum:    ");
	PrintPubKeySha1Sum(data_key);
	printf("\n");

	if (keyblock_outfile) {
		FILE *f = NULL;
		f = fopen(keyblock_outfile, "wb");
		if (!f)  {
			fprintf(stderr, "Can't open key block file %s: %s\n",
				keyblock_outfile, strerror(errno));
			goto done;
		}
		if (1 != fwrite(g_keyblock, g_keyblock->key_block_size, 1, f)) {
			fprintf(stderr, "Can't write key block file %s: %s\n",
				keyblock_outfile, strerror(errno));
			fclose(f);
			goto done;
		}
		fclose(f);
	}

	if (data_key->key_version < (min_version >> 16)) {
		fprintf(stderr, "Data key version %" PRIu64
			" is lower than minimum %" PRIu64 ".\n",
			data_key->key_version, (min_version >> 16));
		goto done;
	}

	rsa = PublicKeyToRSA(data_key);
	if (!rsa) {
		fprintf(stderr, "Error parsing data key.\n");
		goto done;
	}

	/* Verify preamble */
	if (0 != VerifyKernelPreamble(g_preamble,
				      g_preamble->preamble_size, rsa)) {
		fprintf(stderr, "Error verifying preamble.\n");
		goto done;
	}

	printf("Preamble:\n");
	printf("  Size:                0x%" PRIx64 "\n",
	       g_preamble->preamble_size);
	printf("  Header version:      %" PRIu32 ".%" PRIu32 "\n",
	       g_preamble->header_version_major,
	       g_preamble->header_version_minor);
	printf("  Kernel version:      %" PRIu64 "\n",
	       g_preamble->kernel_version);
	printf("  Body load address:   0x%" PRIx64 "\n",
	       g_preamble->body_load_address);
	printf("  Body size:           0x%" PRIx64 "\n",
	       g_preamble->body_signature.data_size);
	printf("  Bootloader address:  0x%" PRIx64 "\n",
	       g_preamble->bootloader_address);
	printf("  Bootloader size:     0x%" PRIx64 "\n",
	       g_preamble->bootloader_size);

	if (VbGetKernelVmlinuzHeader(g_preamble,
				     &vmlinuz_header_address,
				     &vmlinuz_header_size)
	    != VBOOT_SUCCESS) {
		fprintf(stderr, "Unable to retrieve Vmlinuz Header!");
		goto done;
	}
	if (vmlinuz_header_size) {
		printf("  Vmlinuz header address: 0x%" PRIx64 "\n",
		       vmlinuz_header_address);
		printf("  Vmlinuz header size:    0x%" PRIx64 "\n",
		       vmlinuz_header_size);
	}

	if (VbKernelHasFlags(g_preamble) == VBOOT_SUCCESS)
		printf("  Flags          :       0x%" PRIx32 "\n",
		       g_preamble->flags);

	if (g_preamble->kernel_version < (min_version & 0xFFFF)) {
		fprintf(stderr,
			"Kernel version %" PRIu64 " is lower than minimum %"
			PRIu64 ".\n", g_preamble->kernel_version,
			(min_version & 0xFFFF));
		goto done;
	}

	/* Verify body */
	if (0 != VerifyData(kernel_blob, kernel_size,
			    &g_preamble->body_signature, rsa)) {
		fprintf(stderr, "Error verifying kernel body.\n");
		goto done;
	}
	printf("Body verification succeeded.\n");

	printf("Config:\n%s\n", kernel_blob + KernelCmdLineOffset(g_preamble));

	rv = 0;
done:
	return rv;
}


uint8_t *CreateKernelBlob(uint8_t *vmlinuz_buf, uint64_t vmlinuz_size,
			  enum arch_t arch, uint64_t kernel_body_load_address,
			  uint8_t *config_data, uint64_t config_size,
			  uint8_t *bootloader_data, uint64_t bootloader_size,
			  uint64_t *blob_size_ptr)
{
	uint64_t now = 0;
	int tmp;

	/* We have all the parts. How much room do we need? */
	tmp = KernelSize(vmlinuz_buf, vmlinuz_size, arch);
	if (tmp < 0)
		return NULL;
	g_kernel_size = tmp;
	g_config_size = CROS_CONFIG_SIZE;
	g_param_size = CROS_PARAMS_SIZE;
	g_bootloader_size = roundup(bootloader_size, CROS_ALIGN);
	g_vmlinuz_header_size = vmlinuz_size-g_kernel_size;
	g_kernel_blob_size =
		roundup(g_kernel_size, CROS_ALIGN) +
		g_config_size                      +
		g_param_size                       +
		g_bootloader_size                  +
		g_vmlinuz_header_size;
	Debug("g_kernel_blob_size  0x%" PRIx64 "\n", g_kernel_blob_size);

	/* Allocate space for the blob. */
	g_kernel_blob_data = malloc(g_kernel_blob_size);
	Memset(g_kernel_blob_data, 0, g_kernel_blob_size);

	/* Assign the sub-pointers */
	g_kernel_data = g_kernel_blob_data + now;
	Debug("g_kernel_size       0x%" PRIx64 " ofs 0x%" PRIx64 "\n",
	      g_kernel_size, now);
	now += roundup(g_kernel_size, CROS_ALIGN);

	g_config_data = g_kernel_blob_data + now;
	Debug("g_config_size       0x%" PRIx64 " ofs 0x%" PRIx64 "\n",
	      g_config_size, now);
	now += g_config_size;

	g_param_data = g_kernel_blob_data + now;
	Debug("g_param_size        0x%" PRIx64 " ofs 0x%" PRIx64 "\n",
	      g_param_size, now);
	now += g_param_size;

	g_bootloader_data = g_kernel_blob_data + now;
	Debug("g_bootloader_size   0x%" PRIx64 " ofs 0x%" PRIx64 "\n",
	      g_bootloader_size, now);
	g_ondisk_bootloader_addr = kernel_body_load_address + now;
	Debug("g_ondisk_bootloader_addr   0x%" PRIx64 "\n",
	      g_ondisk_bootloader_addr);
	now += g_bootloader_size;

	if (g_vmlinuz_header_size) {
		g_vmlinuz_header_data = g_kernel_blob_data + now;
		Debug("g_vmlinuz_header_size 0x%" PRIx64 " ofs 0x%" PRIx64 "\n",
		      g_vmlinuz_header_size, now);
		g_ondisk_vmlinuz_header_addr = kernel_body_load_address + now;
		Debug("g_ondisk_vmlinuz_header_addr   0x%" PRIx64 "\n",
		      g_ondisk_vmlinuz_header_addr);
	}

	Debug("end of kern_blob at kern_blob+0x%" PRIx64 "\n", now);

	/* Copy the kernel and params bits into the correct places */
	if (0 != PickApartVmlinuz(vmlinuz_buf, vmlinuz_size,
				  arch, kernel_body_load_address)) {
		fprintf(stderr, "Error picking apart kernel file.\n");
		free(g_kernel_blob_data);
		g_kernel_blob_data = NULL;
		g_kernel_blob_size = 0;
		return NULL;
	}

	/* Copy the other bits too */
	Memcpy(g_config_data, config_data, config_size);
	Memcpy(g_bootloader_data, bootloader_data, bootloader_size);
	if (g_vmlinuz_header_size) {
		Memcpy(g_vmlinuz_header_data,
		       vmlinuz_buf,
		       g_vmlinuz_header_size);
	}

	if (blob_size_ptr)
		*blob_size_ptr = g_kernel_blob_size;
	return g_kernel_blob_data;
}

enum futil_file_type recognize_vblock1(uint8_t *buf, uint32_t len)
{
	VbKeyBlockHeader *key_block = (VbKeyBlockHeader *)buf;
	VbPublicKey *pubkey = (VbPublicKey *)buf;
	VbFirmwarePreambleHeader *fw_preamble;
	VbKernelPreambleHeader *kern_preamble;
	RSAPublicKey *rsa;

	if (VBOOT_SUCCESS == KeyBlockVerify(key_block, len, NULL, 1)) {
		rsa = PublicKeyToRSA(&key_block->data_key);
		uint32_t more = key_block->key_block_size;

		/* and firmware preamble too? */
		fw_preamble = (VbFirmwarePreambleHeader *)(buf + more);
		if (VBOOT_SUCCESS ==
		    VerifyFirmwarePreamble(fw_preamble, len - more, rsa))
			return FILE_TYPE_FW_PREAMBLE;

		/* or maybe kernel preamble? */
		kern_preamble = (VbKernelPreambleHeader *)(buf + more);
		if (VBOOT_SUCCESS ==
		    VerifyKernelPreamble(kern_preamble, len - more, rsa))
			return FILE_TYPE_KERN_PREAMBLE;

		/* no, just keyblock */
		return FILE_TYPE_KEYBLOCK;
	}

	/* Maybe just a VbPublicKey? */
	if (PublicKeyLooksOkay(pubkey, len))
		return FILE_TYPE_PUBKEY;

	return FILE_TYPE_UNKNOWN;
}

enum futil_file_type recognize_privkey(uint8_t *buf, uint32_t len)
{
	VbPrivateKey key;
	const unsigned char *start;

	if (len < sizeof(key.algorithm))
		return FILE_TYPE_UNKNOWN;

	key.algorithm = *(typeof(key.algorithm) *)buf;
	start = buf + sizeof(key.algorithm);
	key.rsa_private_key = d2i_RSAPrivateKey(NULL, &start,
						len - sizeof(key.algorithm));

	if (key.rsa_private_key) {
		RSA_free(key.rsa_private_key);
		return FILE_TYPE_PRIVKEY;
	}

	return FILE_TYPE_UNKNOWN;
}
