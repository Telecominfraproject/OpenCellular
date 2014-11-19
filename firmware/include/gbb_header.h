/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure of Google Binary Block (GBB)
 */
#ifndef VBOOT_REFERENCE_GBB_HEADER_H_
#define VBOOT_REFERENCE_GBB_HEADER_H_
#include <stdint.h>

#define GBB_HEADER_SIZE    128

#define GBB_SIGNATURE      "$GBB"
#define GBB_SIGNATURE_SIZE 4

/*
 * GBB version constants.
 *
 * If the major version is different than the reader can handle, it shouldn't
 * attempt to parse the GBB.
 *
 * If the minor version is different, the reader can still parse it.  If the
 * minor version is greater than expected, new fields were added in a way which
 * does not interfere with the old fields.  If it's less than expected, some of
 * the fields expected by the reader aren't initialized, and the reader should
 * return default values for those fields.
 */
#define GBB_MAJOR_VER      1
#define GBB_MINOR_VER      2
/* v1.2 - added field to hold sha256 digest of the HWID */

/* Maximum length of a HWID in bytes, counting terminating null. */
#define GBB_HWID_MAX_SIZE  256

/* Flags for .flags field */
/* Reduce the dev screen delay to 2 sec from 30 sec to speedup factory. */
#define GBB_FLAG_DEV_SCREEN_SHORT_DELAY   0x00000001
/*
 * BIOS should load option ROMs from arbitrary PCI devices. We'll never enable
 * this ourselves because it executes non-verified code, but if a customer
 * wants to void their warranty and set this flag in the read-only flash, they
 * should be able to do so.
 */
#define GBB_FLAG_LOAD_OPTION_ROMS         0x00000002
/*
 * The factory flow may need the BIOS to boot a non-ChromeOS kernel if the
 * dev-switch is on. This flag allows that.
 */
#define GBB_FLAG_ENABLE_ALTERNATE_OS      0x00000004
/* Force dev switch on, regardless of physical/keyboard dev switch position. */
#define GBB_FLAG_FORCE_DEV_SWITCH_ON      0x00000008
/* Allow booting from USB in dev mode even if dev_boot_usb=0. */
#define GBB_FLAG_FORCE_DEV_BOOT_USB       0x00000010
/* Disable firmware rollback protection. */
#define GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK  0x00000020
/* Allow Enter key to trigger dev->tonorm screen transition */
#define GBB_FLAG_ENTER_TRIGGERS_TONORM    0x00000040
/* Allow booting Legacy OSes in dev mode even if dev_boot_legacy=0. */
#define GBB_FLAG_FORCE_DEV_BOOT_LEGACY    0x00000080
/* Allow booting using alternate keys for FAFT servo testing */
#define GBB_FLAG_FAFT_KEY_OVERIDE         0x00000100
/* Disable EC software sync */
#define GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC 0x00000200
/* Default to booting legacy OS when dev screen times out */
#define GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY  0x00000400
/* Disable PD software sync */
#define GBB_FLAG_DISABLE_PD_SOFTWARE_SYNC 0x00000800
/* Disable shutdown on lid closed */
#define GBB_FLAG_DISABLE_LID_SHUTDOWN     0x00001000

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef struct GoogleBinaryBlockHeader
{
	/* Fields present in version 1.0 */
	uint8_t  signature[GBB_SIGNATURE_SIZE]; /* GBB_SIGNATURE "$GBB" */
	uint16_t major_version;   /* See GBB_MAJOR_VER */
	uint16_t minor_version;   /* See GBB_MINOR_VER */
	uint32_t header_size;     /* size of GBB header in bytes */
	uint32_t flags;  /* Flags (see GBB_FLAG_*), should be 0 for 1.0. */
	/* Offsets (from start of header) and sizes (in bytes) of components */
	uint32_t hwid_offset;		/* HWID */
	uint32_t hwid_size;
	uint32_t rootkey_offset;	/* Root key */
	uint32_t rootkey_size;
	uint32_t bmpfv_offset;		/* BMP FV */
	uint32_t bmpfv_size;
	uint32_t recovery_key_offset;	/* Recovery key */
	uint32_t recovery_key_size;
	/* Added in version 1.2 */
	uint8_t  hwid_digest[32];	/* sha256 */

	uint8_t  pad[48]; /* To match GBB_HEADER_SIZE.  Initialize to 0. */
} __attribute__((packed)) GoogleBinaryBlockHeader;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* VBOOT_REFERENCE_GBB_HEADER_H_ */
