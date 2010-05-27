// Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Constants describing the kernel blob content.

#ifndef VBOOT_REFERENCE_KERNEL_BLOB_H_
#define VBOOT_REFERENCE_KERNEL_BLOB_H_


// Maximum kernel command-line size
#define CROS_CONFIG_SIZE 4096

// Size of the x86 zeropage table
#define CROS_PARAMS_SIZE 4096

// Alignment of various chunks within the kernel blob
#define CROS_ALIGN 4096

// RAM address where the 32-bit kernel expects to be started
#define CROS_32BIT_ENTRY_ADDR  0x100000

// Simplified version of the vmlinuz file header
struct linux_kernel_header
{
  uint8_t  pad0[0x01f1 - 0x0];
  uint8_t  setup_sects;                 // 1f1
  uint8_t  pad1[0x0230 - 0x1f2];
} __attribute__ ((packed));


// Simplified version of the x86 kernel zeropage table
struct linux_kernel_params
{
  uint8_t  pad0[0x01f1 - 0x0];
  uint8_t  setup_sects;                 // 1f1
  uint8_t  pad1[0x1fe - 0x1f2];
  uint16_t boot_flag;                   // 1fe
  uint8_t  pad2[0x210 - 0x200];
  uint8_t  type_of_loader;              // 210
  uint8_t  pad3[0x218 - 0x211];
  uint32_t ramdisk_image;               // 218
  uint32_t ramdisk_size;		// 21c
  uint8_t  pad4[0x228 - 0x220];
  uint32_t cmd_line_ptr;                // 228
  uint8_t  pad5[0x0cd0 - 0x22c];
} __attribute__ ((packed));


#endif  // VBOOT_REFERENCE_KERNEL_BLOB_H_
