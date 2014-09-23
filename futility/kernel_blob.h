/* Copyright 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Constants describing the kernel blob content.
 */
#ifndef VBOOT_REFERENCE_KERNEL_BLOB_H_
#define VBOOT_REFERENCE_KERNEL_BLOB_H_

/* Maximum kernel command-line size */
#define CROS_CONFIG_SIZE 4096

/* Size of the x86 zeropage table */
#define CROS_PARAMS_SIZE 4096

/* Alignment of various chunks within the kernel blob */
#define CROS_ALIGN 4096

/* Sentinel RAM address indicating that no entry address is specified */
#define CROS_NO_ENTRY_ADDR     (~0)

/* RAM address where the 32-bit kernel expects to be started */
#define CROS_32BIT_ENTRY_ADDR  0x100000

/* Simplified version of x86 kernel e820 memory map entries */
#define E820_ENTRY_MAX 128
#define E820_TYPE_RAM      1
#define E820_TYPE_RESERVED 2

struct linux_kernel_e820entry {
	uint64_t start_addr;
	uint64_t segment_size;
	uint32_t segment_type;
} __attribute__ ((packed));

/* Simplified version of the x86 kernel zeropage table */
struct linux_kernel_params {
	uint8_t pad0[0x1e8 - 0x0];
	uint8_t n_e820_entry;			/* 1e8 */
	uint8_t pad1[0x1f1 - 0x1e9];
	uint8_t setup_sects;			/* 1f1 */
	uint8_t pad2[0x1fe - 0x1f2];
	uint16_t boot_flag;			/* 1fe */
	uint16_t jump;				/* 200 */
	uint32_t header;			/* 202 */
	uint16_t version;			/* 206 */
	uint8_t pad3[0x210 - 0x208];
	uint8_t type_of_loader;			/* 210 */
	uint8_t pad4[0x218 - 0x211];
	uint32_t ramdisk_image;			/* 218 */
	uint32_t ramdisk_size;			/* 21c */
	uint8_t pad5[0x228 - 0x220];
	uint32_t cmd_line_ptr;			/* 228 */
	uint32_t ramdisk_max;			/* 22c */
	uint32_t kernel_alignment;		/* 230 */
	uint8_t relocatable_kernel;		/* 234 */
	uint8_t min_alignment;			/* 235 */
	uint8_t pad6[0x2d0 - 0x236];
	struct linux_kernel_e820entry
		e820_entries[E820_ENTRY_MAX];	/* 2d0-cd0 */
} __attribute__ ((packed));

#endif /* VBOOT_REFERENCE_KERNEL_BLOB_H_ */
