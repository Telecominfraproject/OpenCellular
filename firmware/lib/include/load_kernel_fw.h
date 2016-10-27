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

struct vb2_context;

/* Interface provided by verified boot library to BDS */

/* Boot flags for LoadKernel().boot_flags */
/* Developer switch is on */
#define BOOT_FLAG_DEVELOPER    (0x01ULL)
/* In recovery mode */
#define BOOT_FLAG_RECOVERY     (0x02ULL)
/* GPT is external */
#define BOOT_FLAG_EXTERNAL_GPT (0x04ULL)

struct RollbackSpaceFwmp;

typedef struct LoadKernelParams {
	/* Inputs to LoadKernel() */
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
	/* Firmware management parameters; may be NULL if not present. */
	const struct RollbackSpaceFwmp *fwmp;

	/*
	 * Outputs from LoadKernel(); valid only if LoadKernel() returns
	 * LOAD_KERNEL_SUCCESS
	 */
	/* Partition number to boot on current device (1...M) */
	uint32_t partition_number;
	/* Address of bootloader image in RAM */
	uint64_t bootloader_address;
	/* Size of bootloader image in bytes */
	uint32_t bootloader_size;
	/* UniquePartitionGuid for boot partition */
	uint8_t  partition_guid[16];
	/* Flags passed in by signer */
	uint32_t flags;
} LoadKernelParams;

/**
 * Attempt to load the kernel from the current device.
 *
 * @param ctx		Vboot context
 * @param params	Params specific to loading the kernel
 * @param cparams	Common parameters to vboot1 APIs
 *
 * Returns VBERROR_SUCCESS if successful.  If unsuccessful, sets a recovery
 * reason via VbNvStorage and returns an error code.
 */
VbError_t LoadKernel(struct vb2_context *ctx, LoadKernelParams *params,
		     VbCommonParams *cparams);

#endif  /* VBOOT_REFERENCE_LOAD_KERNEL_FW_H_ */
