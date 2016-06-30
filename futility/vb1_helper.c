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

#include "2sysincludes.h"
#include "2api.h"
#include "2common.h"
#include "2rsa.h"
#include "2sha.h"
#include "file_type.h"
#include "futility.h"
#include "host_common.h"
#include "kernel_blob.h"
#include "util_misc.h"
#include "vb1_helper.h"
#include "vb2_common.h"

const char *vb1_crypto_name(uint32_t algo)
{
	return algo < kNumAlgorithms ? algo_strings[algo] : "(invalid)";
}

/****************************************************************************/
/* Here are globals containing all the bits & pieces I'm working on.
 *
 * kernel vblock    = keyblock + kernel preamble + padding to 64K (or whatever)
 * kernel blob      = 32-bit kernel + config file + params + bootloader stub +
 *                    vmlinuz_header
 * kernel partition = kernel vblock + kernel blob
 *
 * The vb2_kernel_preamble.preamble_size includes the padding.
 */

/* The keyblock, preamble, and kernel blob are kept in separate places. */
static struct vb2_keyblock *g_keyblock;
static struct vb2_kernel_preamble *g_preamble;
static uint8_t *g_kernel_blob_data;
static uint32_t g_kernel_blob_size;

/* These refer to individual parts within the kernel blob. */
static uint8_t *g_kernel_data;
static uint32_t g_kernel_size;
static uint8_t *g_config_data;
static uint32_t g_config_size;
static uint8_t *g_param_data;
static uint32_t g_param_size;
static uint8_t *g_bootloader_data;
static uint32_t g_bootloader_size;
static uint8_t *g_vmlinuz_header_data;
static uint32_t g_vmlinuz_header_size;

static uint64_t g_ondisk_bootloader_addr;
static uint64_t g_ondisk_vmlinuz_header_addr;


/*
 * Read the kernel command line from a file. Get rid of \n characters along
 * the way and verify that the line fits into a 4K buffer.
 *
 * Return the buffer contaning the line on success (and set the line length
 * using the passed in parameter), or NULL in case something goes wrong.
 */
uint8_t *ReadConfigFile(const char *config_file, uint32_t *config_size)
{
	uint8_t *config_buf;
	int i;

	if (VB2_SUCCESS != vb2_read_file(config_file, &config_buf, config_size))
		return NULL;
	Debug(" config file size=0x%x\n", *config_size);
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
static uint32_t roundup(uint32_t val, uint32_t alignment)
{
	uint32_t rem = val % alignment;
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
uint64_t kernel_cmd_line_offset(const struct vb2_kernel_preamble *preamble)
{
	return preamble->bootloader_address - preamble->body_load_address -
	    CROS_CONFIG_SIZE - CROS_PARAMS_SIZE;
}

/* Returns the size of the 32-bit kernel, or negative on error. */
static int KernelSize(uint8_t *kernel_buf,
		      uint32_t kernel_size,
		      enum arch_t arch)
{
	uint32_t kernel32_start = 0;
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
static int PickApartVmlinuz(uint8_t *kernel_buf,
			    uint32_t kernel_size,
			    enum arch_t arch,
			    uint64_t kernel_body_load_address)
{
	uint32_t kernel32_start = 0;
	uint32_t kernel32_size = kernel_size;
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
	uint32_t now;
	uint32_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;

	/* We have to work backwards from the end, because the preamble
	   only describes the bootloader and vmlinuz stubs. */

	/* Vmlinuz Header is at the end */
	vb2_kernel_get_vmlinuz_header(g_preamble,
				      &vmlinuz_header_address,
				      &vmlinuz_header_size);
	if (vmlinuz_header_size) {
		now = vmlinuz_header_address - g_preamble->body_load_address;
		g_vmlinuz_header_size = vmlinuz_header_size;
		g_vmlinuz_header_data = kernel_blob_data + now;

		Debug("vmlinuz_header_size     = 0x%x\n",
		      g_vmlinuz_header_size);
		Debug("vmlinuz_header_ofs      = 0x%x\n", now);
	}

	/* Where does the bootloader stub begin? */
	now = g_preamble->bootloader_address - g_preamble->body_load_address;

	/* Bootloader is at the end */
	g_bootloader_size = g_preamble->bootloader_size;
	g_bootloader_data = kernel_blob_data + now;
	/* TODO: What to do if this is beyond the end of the blob? */

	Debug("bootloader_size     = 0x%x\n", g_bootloader_size);
	Debug("bootloader_ofs      = 0x%x\n", now);

	/* Before that is the params */
	now -= CROS_PARAMS_SIZE;
	g_param_size = CROS_PARAMS_SIZE;
	g_param_data = kernel_blob_data + now;
	Debug("param_ofs           = 0x%x\n", now);

	/* Before that is the config */
	now -= CROS_CONFIG_SIZE;
	g_config_size = CROS_CONFIG_SIZE;
	g_config_data = kernel_blob_data + now;
	Debug("config_ofs          = 0x%x\n", now);

	/* The kernel starts at offset 0 and extends up to the config */
	g_kernel_data = kernel_blob_data;
	g_kernel_size = now;
	Debug("kernel_size         = 0x%x\n", g_kernel_size);
}


/* Replaces the config section of the specified kernel blob.
 * Return nonzero on error. */
int UpdateKernelBlobConfig(uint8_t *kblob_data, uint32_t kblob_size,
			   uint8_t *config_data, uint32_t config_size)
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
uint8_t *unpack_kernel_partition(uint8_t *kpart_data,
				 uint32_t kpart_size,
				 uint32_t padding,
				 struct vb2_keyblock **keyblock_ptr,
				 struct vb2_kernel_preamble **preamble_ptr,
				 uint32_t *blob_size_ptr)
{
	struct vb2_kernel_preamble *preamble;
	uint32_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;
	uint32_t now = 0;

	/* Sanity-check the keyblock */
	struct vb2_keyblock *keyblock = (struct vb2_keyblock *)kpart_data;
	Debug("Keyblock is 0x%x bytes\n", keyblock->keyblock_size);
	now += keyblock->keyblock_size;
	if (now > kpart_size) {
		fprintf(stderr,
			"keyblock_size advances past the end of the blob\n");
		return NULL;
	}
	if (now > padding) {
		fprintf(stderr,
			"keyblock_size advances past %u byte padding\n",
			padding);
		return NULL;
	}

	/* LGTM */
	g_keyblock = keyblock;

	/* And the preamble */
	preamble = (struct vb2_kernel_preamble *)(kpart_data + now);
	Debug("Preamble is 0x%x bytes\n", preamble->preamble_size);
	now += preamble->preamble_size;
	if (now > kpart_size) {
		fprintf(stderr,
			"preamble_size advances past the end of the blob\n");
		return NULL;
	}
	if (now > padding) {
		fprintf(stderr, "preamble_size advances past %u"
			" byte padding\n", padding);
		return NULL;
	}
	/* LGTM */
	Debug(" kernel_version = %d\n", preamble->kernel_version);
	Debug(" bootloader_address = 0x%" PRIx64 "\n",
	      preamble->bootloader_address);
	Debug(" bootloader_size = 0x%x\n", preamble->bootloader_size);
	Debug(" kern_blob_size = 0x%x\n", preamble->body_signature.data_size);

	uint32_t flags = vb2_kernel_get_flags(preamble);
	Debug(" flags = 0x%x\n", flags);

	g_preamble = preamble;
	g_ondisk_bootloader_addr = g_preamble->bootloader_address;

	vb2_kernel_get_vmlinuz_header(preamble,
				      &vmlinuz_header_address,
				      &vmlinuz_header_size);
	if (vmlinuz_header_size) {
		Debug(" vmlinuz_header_address = 0x%" PRIx64 "\n",
		      vmlinuz_header_address);
		Debug(" vmlinuz_header_size = 0x%x\n", vmlinuz_header_size);
		g_ondisk_vmlinuz_header_addr = vmlinuz_header_address;
	}

	Debug("kernel blob is at offset 0x%x\n", now);
	g_kernel_blob_data = kpart_data + now;
	g_kernel_blob_size = preamble->body_signature.data_size;

	/* Sanity check */
	if (g_kernel_blob_size < preamble->body_signature.data_size)
		fprintf(stderr,
			"Warning: kernel file only has 0x%x bytes\n",
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

uint8_t *SignKernelBlob(uint8_t *kernel_blob,
			uint32_t kernel_size,
			uint32_t padding,
			int version,
			uint64_t kernel_body_load_address,
			struct vb2_keyblock *keyblock,
			struct vb2_private_key *signpriv_key,
			uint32_t flags,
			uint32_t *vblock_size_ptr)
{
	/* Make sure the preamble fills up the rest of the required padding */
	uint32_t min_size = padding > keyblock->keyblock_size
		? padding - keyblock->keyblock_size : 0;

	/* Sign the kernel data */
	struct vb2_signature *body_sig = vb2_calculate_signature(kernel_blob,
								 kernel_size,
								 signpriv_key);
	if (!body_sig) {
		fprintf(stderr, "Error calculating body signature\n");
		return NULL;
	}

	/* Create preamble */
	struct vb2_kernel_preamble *preamble =
		vb2_create_kernel_preamble(version,
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

	uint32_t outsize = keyblock->keyblock_size + preamble->preamble_size;
	void *outbuf = calloc(outsize, 1);
	memcpy(outbuf, keyblock, keyblock->keyblock_size);
	memcpy(outbuf + keyblock->keyblock_size,
	       preamble, preamble->preamble_size);

	if (vblock_size_ptr)
		*vblock_size_ptr = outsize;
	return outbuf;
}

/* Returns zero on success */
int WriteSomeParts(const char *outfile,
		   void *part1_data, uint32_t part1_size,
		   void *part2_data, uint32_t part2_size)
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
		     uint32_t kernel_size,
		     struct vb2_packed_key *signpub_key,
		     const char *keyblock_outfile,
		     uint32_t min_version)
{
	int rv = -1;
	uint32_t vmlinuz_header_size = 0;
	uint64_t vmlinuz_header_address = 0;

	uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
	struct vb2_workbuf wb;
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	if (signpub_key) {
		struct vb2_public_key pubkey;
		if (VB2_SUCCESS !=
		    vb2_unpack_key(&pubkey,
				   (uint8_t *)signpub_key,
				   signpub_key->key_offset +
				   signpub_key->key_size)) {
			fprintf(stderr, "Error unpacking signing key.\n");
			goto done;
		}
		if (VB2_SUCCESS !=
		    vb2_verify_keyblock(g_keyblock, g_keyblock->keyblock_size,
					&pubkey, &wb)) {
			fprintf(stderr, "Error verifying key block.\n");
			goto done;
		}
	} else if (VB2_SUCCESS !=
		   vb2_verify_keyblock_hash(g_keyblock,
					    g_keyblock->keyblock_size,
					    &wb)) {
		fprintf(stderr, "Error verifying key block.\n");
		goto done;
	}

	printf("Key block:\n");
	struct vb2_packed_key *data_key = &g_keyblock->data_key;
	printf("  Signature:           %s\n",
	       signpub_key ? "valid" : "ignored");
	printf("  Size:                0x%x\n", g_keyblock->keyblock_size);
	printf("  Flags:               %u ", g_keyblock->keyblock_flags);
	if (g_keyblock->keyblock_flags & KEY_BLOCK_FLAG_DEVELOPER_0)
		printf(" !DEV");
	if (g_keyblock->keyblock_flags & KEY_BLOCK_FLAG_DEVELOPER_1)
		printf(" DEV");
	if (g_keyblock->keyblock_flags & KEY_BLOCK_FLAG_RECOVERY_0)
		printf(" !REC");
	if (g_keyblock->keyblock_flags & KEY_BLOCK_FLAG_RECOVERY_1)
		printf(" REC");
	printf("\n");
	printf("  Data key algorithm:  %u %s\n", data_key->algorithm,
	       (data_key->algorithm < kNumAlgorithms ?
		algo_strings[data_key->algorithm] : "(invalid)"));
	printf("  Data key version:    %u\n", data_key->key_version);
	printf("  Data key sha1sum:    %s\n",
	       packed_key_sha1_string(data_key));

	if (keyblock_outfile) {
		FILE *f = NULL;
		f = fopen(keyblock_outfile, "wb");
		if (!f)  {
			fprintf(stderr, "Can't open key block file %s: %s\n",
				keyblock_outfile, strerror(errno));
			goto done;
		}
		if (1 != fwrite(g_keyblock, g_keyblock->keyblock_size, 1, f)) {
			fprintf(stderr, "Can't write key block file %s: %s\n",
				keyblock_outfile, strerror(errno));
			fclose(f);
			goto done;
		}
		fclose(f);
	}

	if (data_key->key_version < (min_version >> 16)) {
		fprintf(stderr, "Data key version %u < minimum %u.\n",
			data_key->key_version, (min_version >> 16));
		goto done;
	}

	struct vb2_public_key pubkey;
	if (VB2_SUCCESS !=
	    vb2_unpack_key(&pubkey, (uint8_t *)data_key,
			   data_key->key_offset + data_key->key_size)) {
		fprintf(stderr, "Error parsing data key.\n");
		goto done;
	}

	/* Verify preamble */
	if (VB2_SUCCESS != vb2_verify_kernel_preamble(
			(struct vb2_kernel_preamble *)g_preamble,
			g_preamble->preamble_size, &pubkey, &wb)) {
		fprintf(stderr, "Error verifying preamble.\n");
		goto done;
	}

	printf("Preamble:\n");
	printf("  Size:                0x%x\n", g_preamble->preamble_size);
	printf("  Header version:      %u.%u\n",
	       g_preamble->header_version_major,
	       g_preamble->header_version_minor);
	printf("  Kernel version:      %u\n", g_preamble->kernel_version);
	printf("  Body load address:   0x%" PRIx64 "\n",
	       g_preamble->body_load_address);
	printf("  Body size:           0x%x\n",
	       g_preamble->body_signature.data_size);
	printf("  Bootloader address:  0x%" PRIx64 "\n",
	       g_preamble->bootloader_address);
	printf("  Bootloader size:     0x%x\n", g_preamble->bootloader_size);

	vb2_kernel_get_vmlinuz_header(g_preamble,
				      &vmlinuz_header_address,
				      &vmlinuz_header_size);
	if (vmlinuz_header_size) {
		printf("  Vmlinuz header address: 0x%" PRIx64 "\n",
		       vmlinuz_header_address);
		printf("  Vmlinuz header size:    0x%x\n",
		       (uint32_t)vmlinuz_header_size);
	}

	printf("  Flags          :       0x%x\n",
	       vb2_kernel_get_flags(g_preamble));

	if (g_preamble->kernel_version < (min_version & 0xFFFF)) {
		fprintf(stderr,
			"Kernel version %u is lower than minimum %u.\n",
			g_preamble->kernel_version, (min_version & 0xFFFF));
		goto done;
	}

	/* Verify body */
	if (VB2_SUCCESS !=
	    vb2_verify_data(kernel_blob, kernel_size,
			    &g_preamble->body_signature,
			    &pubkey, &wb)) {
		fprintf(stderr, "Error verifying kernel body.\n");
		goto done;
	}
	printf("Body verification succeeded.\n");

	printf("Config:\n%s\n",
	       kernel_blob + kernel_cmd_line_offset(g_preamble));

	rv = 0;
done:
	return rv;
}


uint8_t *CreateKernelBlob(uint8_t *vmlinuz_buf, uint32_t vmlinuz_size,
			  enum arch_t arch, uint64_t kernel_body_load_address,
			  uint8_t *config_data, uint32_t config_size,
			  uint8_t *bootloader_data, uint32_t bootloader_size,
			  uint32_t *blob_size_ptr)
{
	uint32_t now = 0;
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

enum futil_file_type ft_recognize_vblock1(uint8_t *buf, uint32_t len)
{
	uint8_t workbuf[VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE];
	struct vb2_workbuf wb;
	vb2_workbuf_init(&wb, workbuf, sizeof(workbuf));

	/* Vboot 2.0 signature checks destroy the buffer, so make a copy */
	uint8_t *buf2 = malloc(len);
	memcpy(buf2, buf, len);
	struct vb2_keyblock *keyblock = (struct vb2_keyblock *)buf2;
	if (VB2_SUCCESS != vb2_verify_keyblock_hash(keyblock, len, &wb)) {
		free(buf2);
		return FILE_TYPE_UNKNOWN;
	}

	/* Try unpacking the data key from the keyblock */
	struct vb2_public_key data_key;
	if (VB2_SUCCESS !=
	    vb2_unpack_key(&data_key, (const uint8_t *)&keyblock->data_key,
			   keyblock->data_key.key_offset +
			   keyblock->data_key.key_size)) {
		/* It looks like a bad keyblock, but still a keyblock */
		free(buf2);
		return FILE_TYPE_KEYBLOCK;
	}

	uint32_t more = keyblock->keyblock_size;

	/* Followed by firmware preamble too? */
	struct vb2_fw_preamble *pre2 = (struct vb2_fw_preamble *)(buf2 + more);
	if (VB2_SUCCESS ==
	    vb2_verify_fw_preamble(pre2, len - more, &data_key, &wb)) {
		free(buf2);
		return FILE_TYPE_FW_PREAMBLE;
	}

	/* Recopy since firmware preamble check destroyed the buffer */
	memcpy(buf2, buf, len);

	/* Or maybe kernel preamble? */
	struct vb2_kernel_preamble *kern_preamble =
		(struct vb2_kernel_preamble *)(buf2 + more);
	if (VB2_SUCCESS ==
	    vb2_verify_kernel_preamble(kern_preamble, len - more,
				       &data_key, &wb)) {
		free(buf2);
		return FILE_TYPE_KERN_PREAMBLE;
	}

	free(buf2);

	/* No, just keyblock */
	return FILE_TYPE_KEYBLOCK;
}

enum futil_file_type ft_recognize_vb1_key(uint8_t *buf, uint32_t len)
{
	/* Maybe just a packed public key? */
	const struct vb2_packed_key *pubkey = (struct vb2_packed_key *)buf;
	if (packed_key_looks_ok(pubkey, len))
		return FILE_TYPE_PUBKEY;

	/* How about a private key? */
	if (len < sizeof(uint64_t))
		return FILE_TYPE_UNKNOWN;
	const unsigned char *start = buf + sizeof(uint64_t);
	struct rsa_st *rsa =
		d2i_RSAPrivateKey(NULL, &start, len - sizeof(uint64_t));
	if (rsa) {
		RSA_free(rsa);
		return FILE_TYPE_PRIVKEY;
	}

	return FILE_TYPE_UNKNOWN;
}
