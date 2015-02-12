/* Copyright 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Exports the kernel commandline from a given partition/image.
 */

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "host_common.h"
#include "kernel_blob.h"
#include "vboot_api.h"
#include "vboot_host.h"

typedef ssize_t (*ReadFullyFn)(void *ctx, void *buf, size_t count);

static ssize_t ReadFullyWithRead(void *ctx, void *buf, size_t count)
{
	ssize_t nr_read = 0;
	int fd = *((int*)ctx);
	while (nr_read < count) {
		ssize_t to_read = count - nr_read;
		ssize_t chunk = read(fd, buf + nr_read, to_read);
		if (chunk < 0) {
			return -1;
		} else if (chunk == 0) {
			break;
		}
		nr_read += chunk;
	}
	return nr_read;
}

/* Skip the stream by calling |read_fn| many times. Return 0 on success. */
static int SkipWithRead(void *ctx, ReadFullyFn read_fn, size_t count)
{
	char buf[1024];
	ssize_t nr_skipped = 0;
	while (nr_skipped < count) {
		ssize_t to_read = count - nr_skipped;
		if (to_read > sizeof(buf)) {
			to_read = sizeof(buf);
		}
		if (read_fn(ctx, buf, to_read) != to_read) {
			return -1;
		}
		nr_skipped += to_read;
	}
	return 0;
}

static char *FindKernelConfigFromStream(void *ctx, ReadFullyFn read_fn,
					uint64_t kernel_body_load_address)
{
	VbKeyBlockHeader key_block;
	VbKernelPreambleHeader preamble;
	uint32_t now = 0;
	uint32_t offset = 0;

	/* Skip the key block */
	if (read_fn(ctx, &key_block, sizeof(key_block)) != sizeof(key_block)) {
		VbExError("not enough data to fill key block header\n");
		return NULL;
	}
	ssize_t to_skip = key_block.key_block_size - sizeof(key_block);
	if (to_skip < 0 || SkipWithRead(ctx, read_fn, to_skip)) {
		VbExError("key_block_size advances past the end of the blob\n");
		return NULL;
	}
	now += key_block.key_block_size;

	/* Open up the preamble */
	if (read_fn(ctx, &preamble, sizeof(preamble)) != sizeof(preamble)) {
		VbExError("not enough data to fill preamble\n");
		return NULL;
	}
	to_skip = preamble.preamble_size - sizeof(preamble);
	if (to_skip < 0 || SkipWithRead(ctx, read_fn, to_skip)) {
		VbExError("preamble_size advances past the end of the blob\n");
		return NULL;
	}
	now += preamble.preamble_size;

	/* Read body_load_address from preamble if no
	 * kernel_body_load_address */
	if (kernel_body_load_address == USE_PREAMBLE_LOAD_ADDR)
		kernel_body_load_address = preamble.body_load_address;

	/* The x86 kernels have a pointer to the kernel commandline in the
	 * zeropage table, but that's irrelevant for ARM. Both types keep the
	 * config blob in the same place, so just go find it. */
	offset = preamble.bootloader_address -
	    (kernel_body_load_address + CROS_PARAMS_SIZE +
	     CROS_CONFIG_SIZE) + now;
	to_skip = offset - now;
	if (to_skip < 0 || SkipWithRead(ctx, read_fn, to_skip)) {
		VbExError("params are outside of the memory blob: %x\n",
			  offset);
		return NULL;
	}
	char *ret = malloc(CROS_CONFIG_SIZE);
	if (!ret) {
		VbExError("No memory\n");
		return NULL;
	}
	if (read_fn(ctx, ret, CROS_CONFIG_SIZE) != CROS_CONFIG_SIZE) {
		VbExError("Cannot read kernel config\n");
		free(ret);
		ret = NULL;
	}
	return ret;
}

char *FindKernelConfig(const char *infile, uint64_t kernel_body_load_address)
{
	char *newstr = NULL;

	int fd = open(infile, O_RDONLY | O_CLOEXEC | O_LARGEFILE);
	if (fd < 0) {
		VbExError("Cannot open %s\n", infile);
		return NULL;
	}

	void *ctx = &fd;
	ReadFullyFn read_fn = ReadFullyWithRead;

	newstr = FindKernelConfigFromStream(ctx, read_fn,
					    kernel_body_load_address);

	close(fd);

	return newstr;
}
