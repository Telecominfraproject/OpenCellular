/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying kernel.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_LOAD_KERNEL_FW_H_
#define VBOOT_REFERENCE_LOAD_KERNEL_FW_H_

#include "sysincludes.h"
#include "vboot_api.h"
#include "vboot_nvstorage.h"

/* Interface provided by verified boot library to BDS */

/* Boot flags for LoadKernel().boot_flags */
/* Developer switch is on */
#define BOOT_FLAG_DEVELOPER UINT64_C(0x01)
/* In recovery mode */
#define BOOT_FLAG_RECOVERY  UINT64_C(0x02)
/* Skip check of kernel buffer address */
#define BOOT_FLAG_SKIP_ADDR_CHECK UINT64_C(0x04)

typedef struct LoadKernelParams {
  /* Inputs to LoadKernel() */
  void* shared_data_blob;       /* Buffer for data shared between
                                 * LoadFirmware() and LoadKernel().  Pass the
                                 * same buffer which was passed to
                                 * LoadFirmware(). */
  uint64_t shared_data_size;    /* Size of shared data blob buffer, in bytes.
                                 * On output, this will contain the actual
                                 * data size placed into the buffer. */
  void* gbb_data;               /* Pointer to GBB data */
  uint64_t gbb_size;            /* Size of GBB data in bytes */

  VbExDiskHandle_t disk_handle; /* Disk handle for current device */
  uint64_t bytes_per_lba;       /* Bytes per lba sector on current device */
  uint64_t ending_lba;          /* Last addressable lba sector on current
                                 * device */

  void* kernel_buffer;          /* Destination buffer for kernel
                                 * (normally at 0x100000) */
  uint64_t kernel_buffer_size;  /* Size of kernel buffer in bytes */
  uint64_t boot_flags;          /* Boot flags */
  VbNvContext* nv_context;      /* Context for NV storage.  Caller is
                                 * responsible for calling VbNvSetup() and
                                 * VbNvTeardown() on the context. */

  /* Outputs from LoadKernel(); valid only if LoadKernel() returns
   * LOAD_KERNEL_SUCCESS */
  uint64_t partition_number;    /* Partition number to boot on current device
                                 * (1...M) */
  uint64_t bootloader_address;  /* Address of bootloader image in RAM */
  uint64_t bootloader_size;     /* Size of bootloader image in bytes */
  uint8_t  partition_guid[16];  /* UniquePartitionGuid for boot partition */
} LoadKernelParams;

VbError_t LoadKernel(LoadKernelParams* params);
/* Attempts to load the kernel from the current device.
 *
 * Returns VBERROR_SUCCESS if successful.  If unsuccessful, sets a recovery
 * reason via VbNvStorage and returns an error code. */


typedef struct KernelBootloaderOptions {
  /* The bootloader is loaded using the EFI LoadImage() and StartImage()
   * calls.  Pass this struct via loaded_image->load_options. */
  uint64_t drive_number;        /* Drive number of boot device (0...N) */
  uint64_t partition_number;    /* Partition number, as returned from
                                 * LoadKernel() in
                                 * LoadKernelParams.partition_number */
  uint64_t original_address;    /* Absolute bootloader start adddress,
                                 * as returned from LoadKernel() in
                                 * LoadKernelParams.bootloader_start */
  uint8_t  partition_guid[16];  /* UniquePartitionGuid for boot partition */
} KernelBootloaderOptions;


#endif  /* VBOOT_REFERENCE_LOAD_KERNEL_FW_H_ */
