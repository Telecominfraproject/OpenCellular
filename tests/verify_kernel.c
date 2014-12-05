/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Routines for verifying a kernel or disk image
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host_common.h"
#include "util_misc.h"
#include "vboot_common.h"
#include "vboot_api.h"
#include "vboot_kernel.h"

static uint8_t *diskbuf;

static uint8_t shared_data[VB_SHARED_DATA_MIN_SIZE];
static VbSharedDataHeader *shared = (VbSharedDataHeader *)shared_data;
static VbNvContext nvc;

static LoadKernelParams params;
static VbCommonParams cparams;

VbError_t VbExDiskRead(VbExDiskHandle_t handle, uint64_t lba_start,
		       uint64_t lba_count, void *buffer)
{
	if (handle != (VbExDiskHandle_t)1)
		return VBERROR_UNKNOWN;
	if (lba_start >= params.streaming_lba_count)
		return VBERROR_UNKNOWN;
	if (lba_start + lba_count > params.streaming_lba_count)
		return VBERROR_UNKNOWN;

	memcpy(buffer, diskbuf + lba_start * 512, lba_count * 512);
	return VBERROR_SUCCESS;
}

VbError_t VbExDiskWrite(VbExDiskHandle_t handle, uint64_t lba_start,
			uint64_t lba_count, const void *buffer)
{
	if (handle != (VbExDiskHandle_t)1)
		return VBERROR_UNKNOWN;
	if (lba_start >= params.streaming_lba_count)
		return VBERROR_UNKNOWN;
	if (lba_start + lba_count > params.streaming_lba_count)
		return VBERROR_UNKNOWN;

	memcpy(diskbuf + lba_start * 512, buffer, lba_count * 512);
	return VBERROR_SUCCESS;
}

static void print_help(const char *progname)
{
	printf("\nUsage: %s <disk_image> <kernel.vbpubk>\n\n",
	       progname);
}

int main(int argc, char *argv[])
{
	VbPublicKey *kernkey;
	uint64_t disk_bytes = 0;
	int rv;

	if (argc < 3) {
		print_help(argv[0]);
		return 1;
	}

	/* Load disk file */
	/* TODO: is it better to mmap() in the long run? */
	diskbuf = ReadFile(argv[1], &disk_bytes);
	if (!diskbuf) {
		fprintf(stderr, "Can't read disk file %s\n", argv[1]);
		return 1;
	}

	/* Read public key */
	kernkey = PublicKeyRead(argv[2]);
	if (!kernkey) {
		fprintf(stderr, "Can't read key file %s\n", argv[2]);
		return 1;
	}

	/* Set up shared data blob */
	VbSharedDataInit(shared, sizeof(shared_data));
	VbSharedDataSetKernelKey(shared, kernkey);
	/* TODO: optional TPM current kernel version */

	/* Set up params */
	params.shared_data_blob = shared_data;
	params.shared_data_size = sizeof(shared_data);
	params.disk_handle = (VbExDiskHandle_t)1;
	params.bytes_per_lba = 512;
	params.streaming_lba_count = disk_bytes / 512;
	params.gpt_lba_count = params.streaming_lba_count;

	params.kernel_buffer_size = 16 * 1024 * 1024;
	params.kernel_buffer = malloc(params.kernel_buffer_size);
	if (!params.kernel_buffer) {
		fprintf(stderr, "Can't allocate kernel buffer\n");
		return 1;
	}

	/* GBB and cparams only needed by LoadKernel() in recovery mode */
	params.gbb_data = NULL;
	params.gbb_size = 0;

	/* TODO(chromium:441893): support dev-mode flag and external gpt flag */
	params.boot_flags = 0;

	/*
	 * LoadKernel() cares only about VBNV_DEV_BOOT_SIGNED_ONLY, and only in
	 * dev mode.  So just use defaults.
	 */
	VbNvSetup(&nvc);
	params.nv_context = &nvc;

	/* Try loading kernel */
	rv = LoadKernel(&params, &cparams);
	if (rv != VBERROR_SUCCESS) {
		fprintf(stderr, "LoadKernel() failed with code %d\n", rv);
		return 1;
	}

	printf("Found a good kernel.\n");
	printf("Partition number:   %d\n", (int)params.partition_number);
	printf("Bootloader address: 0x%" PRIx64 "\n",
	       params.bootloader_address);

	/* TODO: print other things (partition GUID, nv_context, shared_data) */

	printf("Yaay!\n");
	return 0;
}
