/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Non-volatile storage bitfields
 */

#ifndef VBOOT_REFERENCE_VBOOT_2NVSTORAGE_FIELDS_H_
#define VBOOT_REFERENCE_VBOOT_2NVSTORAGE_FIELDS_H_

/*
 * Constants for NV storage.  We use this rather than structs and bitfields so
 * the data format is consistent across platforms and compilers.  Total NV
 * storage size is:
 *
 * Version 1: VB2_NVDATA_SIZE = 16 bytes
 * Version 2: VB2_NVDATA_SIZE_V2 = 64 bytes
 *
 * Unused bits/bytes must be set to 0.
 */

enum vb2_nv_offset {
	/*** The following fields are present in all versions ***/
	VB2_NV_OFFS_HEADER = 0,
	VB2_NV_OFFS_BOOT = 1,
	VB2_NV_OFFS_RECOVERY = 2,
	VB2_NV_OFFS_LOCALIZATION = 3,
	VB2_NV_OFFS_DEV = 4,
	VB2_NV_OFFS_TPM = 5,
	VB2_NV_OFFS_RECOVERY_SUBCODE = 6,
	VB2_NV_OFFS_BOOT2 = 7,
	VB2_NV_OFFS_MISC = 8,
	VB2_NV_OFFS_KERNEL_MAX_ROLLFORWARD1 = 9, /* bits 0-7 of 32 */
	VB2_NV_OFFS_KERNEL_MAX_ROLLFORWARD2 = 10, /* bits 8-15 of 32 */
	VB2_NV_OFFS_KERNEL1 = 11, /* bits 0-7 of 16 */
	VB2_NV_OFFS_KERNEL2 = 12, /* bits 8-15 of 16 */
	VB2_NV_OFFS_KERNEL_MAX_ROLLFORWARD3 = 13, /* bits 16-23 of 32 */
	VB2_NV_OFFS_KERNEL_MAX_ROLLFORWARD4 = 14, /* bits 24-31 of 32 */

	/*
	 * CRC_V1 must be last field in V1.  This byte can be reused in later
	 * versions.
	 */
	VB2_NV_OFFS_CRC_V1 = 15,

	/* The following fields are only present in V2+ */
	VB2_NV_OFFS_RESERVED_V2 = 15, /* Was CRC in V1 (unused = 0xff) */
	VB2_NV_OFFS_FW_MAX_ROLLFORWARD1 = 16, /* bits 0-7 of 32 */
	VB2_NV_OFFS_FW_MAX_ROLLFORWARD2 = 17, /* bits 8-15 of 32 */
	VB2_NV_OFFS_FW_MAX_ROLLFORWARD3 = 18, /* bits 16-23 of 32 */
	VB2_NV_OFFS_FW_MAX_ROLLFORWARD4 = 19, /* bits 24-31 of 32 */

	/* CRC must be last field */
	VB2_NV_OFFS_CRC_V2 = 63,
 };

/* Fields in VB2_NV_OFFS_HEADER (unused = 0x04) */
#define VB2_NV_HEADER_WIPEOUT		       0x08
#define VB2_NV_HEADER_KERNEL_SETTINGS_RESET    0x10
#define VB2_NV_HEADER_FW_SETTINGS_RESET        0x20
#define VB2_NV_HEADER_SIGNATURE_MASK           0xc3

/*
 * Valid signature values.  Note that V1 readers only looked at mask 0xc0, and
 * expect it to have value 0x40.  Versions != V1 must have the top two bits !=
 * 0x40 so old readers will reject the data.  Using all 1-bits or all 0-bits is
 * also discouraged, because that is indistinguishable from all-erased data on
 * some platforms.
 */
/* Version 1 = 16-byte record */
#define VB2_NV_HEADER_SIGNATURE_V1             0x40
/* Version 2 = 64-byte record */
#define VB2_NV_HEADER_SIGNATURE_V2             0x03

/* Fields in VB2_NV_OFFS_BOOT */
#define VB2_NV_BOOT_TRY_COUNT_MASK             0x0f
#define VB2_NV_BOOT_BACKUP_NVRAM               0x10
#define VB2_NV_BOOT_OPROM_NEEDED               0x20
#define VB2_NV_BOOT_DISABLE_DEV                0x40
#define VB2_NV_BOOT_DEBUG_RESET                0x80

/* Fields in VB2_NV_OFFS_BOOT2 (unused = 0x80) */
#define VB2_NV_BOOT2_RESULT_MASK               0x03
#define VB2_NV_BOOT2_TRIED                     0x04
#define VB2_NV_BOOT2_TRY_NEXT                  0x08
#define VB2_NV_BOOT2_PREV_RESULT_MASK          0x30
#define VB2_NV_BOOT2_PREV_RESULT_SHIFT 4  /* Number of bits to shift result */
#define VB2_NV_BOOT2_PREV_TRIED                0x40

/* Fields in VB2_NV_OFFS_DEV (unused = 0xc0) */
#define VB2_NV_DEV_FLAG_USB                    0x01
#define VB2_NV_DEV_FLAG_SIGNED_ONLY            0x02
#define VB2_NV_DEV_FLAG_LEGACY                 0x04
#define VB2_NV_DEV_FLAG_FASTBOOT_FULL_CAP      0x08
#define VB2_NV_DEV_FLAG_DEFAULT_BOOT           0x30
#define VB2_NV_DEV_DEFAULT_BOOT_SHIFT 4  /* Number of bits to shift */

/* Fields in VB2_NV_OFFS_TPM (unused = 0xf8) */
#define VB2_NV_TPM_CLEAR_OWNER_REQUEST         0x01
#define VB2_NV_TPM_CLEAR_OWNER_DONE            0x02
#define VB2_NV_TPM_REBOOTED                    0x04

/* Fields in VB2_NV_OFFS_MISC (unused = 0xf0) */
#define VB2_NV_MISC_UNLOCK_FASTBOOT            0x01
#define VB2_NV_MISC_BOOT_ON_AC_DETECT          0x02
#define VB2_NV_MISC_TRY_RO_SYNC		       0x04
#define VB2_NV_MISC_BATTERY_CUTOFF             0x08

#endif  /* VBOOT_REFERENCE_VBOOT_2NVSTORAGE_FIELDS_H_ */
