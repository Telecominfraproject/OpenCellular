/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware API for loading and verifying kernel.
 * (Firmware Portion)
 */

#ifndef VBOOT_REFERENCE_LOAD_KERNEL_FW_H_
#define VBOOT_REFERENCE_LOAD_KERNEL_FW_H_

#include <stdint.h>

/* Interface provided by verified boot library to BDS */

/* Return codes for LoadKernel() */
#define LOAD_KERNEL_SUCCESS 0
#define LOAD_KERNEL_NOT_FOUND 1
#define LOAD_KERNEL_INVALID 2

typedef struct LoadKernelParams {
  /* Inputs to LoadKernel() */
  uint64_t bytes_per_lba;       /* Bytes per lba sector on current device */
  uint64_t ending_lba;          /* Last addressable lba sector on current
                                 * device */
  void *kernel_buffer;          /* Destination buffer for kernel
                                 * (normally at 0x100000) */
  uint64_t kernel_buffer_size;  /* Size of kernel buffer in bytes */
  uint8_t in_developer_mode;    /* Did device boot in developer mode?
                                 * 0 = normal or recovery mode
                                 * 1 = developer mode */

  /* Outputs from LoadKernel(); valid only if LoadKernel() returns
   * LOAD_KERNEL_SUCCESS */
  uint64_t partition_number;    /* Partition number to boot on current device
                                 * (1...M) */
  void *bootloader_start;       /* Start of bootloader image */
  uint64_t bootloader_size;     /* Size of bootloader image in bytes */
} LoadKernelParams;

uintn_t LoadKernel(LoadKernelParams* params);
/* Attempts to load the kernel from the current device.
 *
 * Returns LOAD_KERNEL_SUCCESS if successful, error code on failure. */


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
} KernelBootloaderOptions;


#endif  /* VBOOT_REFERENCE_LOAD_KERNEL_FW_H_ */
