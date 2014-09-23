/* Copyright 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports the kernel commandline from a given partition/image.
 */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "host_common.h"
#include "kernel_blob.h"
#include "vboot_api.h"
#include "vboot_host.h"

static uint8_t *GetKernelConfig(uint8_t *blob, size_t blob_size,
				uint64_t kernel_body_load_address)
{

	VbKeyBlockHeader *key_block;
	VbKernelPreambleHeader *preamble;
	uint32_t now = 0;
	uint32_t offset = 0;

	/* Skip the key block */
	key_block = (VbKeyBlockHeader *) blob;
	now += key_block->key_block_size;
	if (now + blob > blob + blob_size) {
		VbExError("key_block_size advances past the end of the blob\n");
		return NULL;
	}

	/* Open up the preamble */
	preamble = (VbKernelPreambleHeader *) (blob + now);
	now += preamble->preamble_size;
	if (now + blob > blob + blob_size) {
		VbExError("preamble_size advances past the end of the blob\n");
		return NULL;
	}

	/* Read body_load_address from preamble if no
	 * kernel_body_load_address */
	if (kernel_body_load_address == USE_PREAMBLE_LOAD_ADDR)
		kernel_body_load_address = preamble->body_load_address;

	/* The x86 kernels have a pointer to the kernel commandline in the
	 * zeropage table, but that's irrelevant for ARM. Both types keep the
	 * config blob in the same place, so just go find it. */
	offset = preamble->bootloader_address -
	    (kernel_body_load_address + CROS_PARAMS_SIZE +
	     CROS_CONFIG_SIZE) + now;
	if (offset > blob_size) {
		VbExError("params are outside of the memory blob: %x\n",
			  offset);
		return NULL;
	}
	return blob + offset;
}

static void *MMapFile(const char *filename, size_t *size)
{
	FILE *f;
	uint8_t *buf;
	long file_size = 0;

	f = fopen(filename, "rb");
	if (!f) {
		VBDEBUG(("Unable to open file %s\n", filename));
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	file_size = ftell(f);
	rewind(f);

	if (file_size <= 0) {
		fclose(f);
		return NULL;
	}
	*size = (size_t) file_size;

	/* Uses a host primitive as this is not meant for firmware use. */
	buf = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
	if (buf == MAP_FAILED) {
		VbExError("Failed to mmap the file %s\n", filename);
		fclose(f);
		return NULL;
	}

	fclose(f);
	return buf;
}

char *FindKernelConfig(const char *infile, uint64_t kernel_body_load_address)
{
	uint8_t *blob;
	size_t blob_size;
	uint8_t *config = NULL;
	char *newstr = NULL;

	blob = MMapFile(infile, &blob_size);
	if (!blob) {
		VbExError("Error reading input file\n");
		return 0;
	}

	config = GetKernelConfig(blob, blob_size, kernel_body_load_address);
	if (!config) {
		VbExError("Error parsing input file\n");
		munmap(blob, blob_size);
		return 0;
	}

	newstr = strndup((char *)config, CROS_CONFIG_SIZE);
	if (!newstr)
		VbExError("Can't allocate new string\n");

	munmap(blob, blob_size);

	return newstr;
}
