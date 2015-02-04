/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying kernel.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_LOAD_KERNEL_FW_H_
#define VBOOT_REFERENCE_LOAD_KERNEL_FW_H_

#include "vboot_api.h"
#include "vboot_nvstorage.h"

/* Interface provided by verified boot library to BDS */

/* Boot flags for LoadKernel().boot_flags */
/* Developer switch is on */
#define BOOT_FLAG_DEVELOPER    (0x01ULL)
/* In recovery mode */
#define BOOT_FLAG_RECOVERY     (0x02ULL)
/* GPT is external */
#define BOOT_FLAG_EXTERNAL_GPT (0x04ULL)

typedef struct LoadKernelParams {
	/* Inputs to LoadKernel() */
	/*
	 * Buffer for data shared between LoadFirmware() and LoadKernel().
	 * Pass the same buffer which was passed to LoadFirmware().
	 */
	void *shared_data_blob;
	/*
	 * Size of shared data blob buffer, in bytes.  On output, this will
	 * contain the actual data size placed into the buffer.
	 */
	uint64_t shared_data_size;
	/* Pointer to GBB data */
	void *gbb_data;
	/* Size of GBB data in bytes */
	uint64_t gbb_size;
	/* Disk handle for current device */
	VbExDiskHandle_t disk_handle;
	/* Bytes per lba sector on current device */
	uint64_t bytes_per_lba;
	/* Number of LBA-addressable sectors on the main device */
	uint64_t streaming_lba_count;
	/* Random-access GPT size */
	uint64_t gpt_lba_count;
	/* Destination buffer for kernel (normally at 0x100000) */
	void *kernel_buffer;
	/* Size of kernel buffer in bytes */
	uint64_t kernel_buffer_size;
	/* Boot flags */
	uint64_t boot_flags;
	/*
	 * Context for NV storage.  Caller is responsible for calling
	 * VbNvSetup() and VbNvTeardown() on the context.
	 */
	VbNvContext *nv_context;

	/*
	 * Outputs from LoadKernel(); valid only if LoadKernel() returns
	 * LOAD_KERNEL_SUCCESS
	 */
	/* Partition number to boot on current device (1...M) */
	uint64_t partition_number;
	/* Address of bootloader image in RAM */
	uint64_t bootloader_address;
	/* Size of bootloader image in bytes */
	uint64_t bootloader_size;
	/* UniquePartitionGuid for boot partition */
	uint8_t  partition_guid[16];
	/* Flags passed in by signer */
	uint32_t flags;
} LoadKernelParams;

/**
 * Attempt to load the kernel from the current device.
 *
 * Returns VBERROR_SUCCESS if successful.  If unsuccessful, sets a recovery
 * reason via VbNvStorage and returns an error code.
 */
VbError_t LoadKernel(LoadKernelParams *params, VbCommonParams *cparams);

/*
 * The bootloader is loaded using the EFI LoadImage() and StartImage() calls.
 * Pass this struct via loaded_image->load_options.
 */
typedef struct KernelBootloaderOptions {
	/* Drive number of boot device (0...N) */
	uint64_t drive_number;
	/*
	 * Partition number, as returned from LoadKernel() in
	 * LoadKernelParams.partition_number
	 */
	uint64_t partition_number;
	/*
	 * Absolute bootloader start adddress, as returned from LoadKernel() in
	 * LoadKernelParams.bootloader_start
	 */
	uint64_t original_address;
	  /* UniquePartitionGuid for boot partition */
	uint8_t partition_guid[16];
} KernelBootloaderOptions;

#endif  /* VBOOT_REFERENCE_LOAD_KERNEL_FW_H_ */
