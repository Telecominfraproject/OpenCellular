/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Data structure definitions for verified boot, for on-disk / in-eeprom
 * data.
 */

#ifndef VBOOT_REFERENCE_VBOOT_2STRUCT_H_
#define VBOOT_REFERENCE_VBOOT_2STRUCT_H_
#include <stdint.h>

/*
 * Note: Many of the structs have pairs of 32-bit fields and reserved fields.
 * This is to be backwards-compatible with older verified boot data which used
 * 64-bit fields (when we thought that hey, UEFI is 64-bit so all our fields
 * should be too).
 */

/* Packed public key data */
struct vb2_packed_key {
	/* Offset of key data from start of this struct */
	uint32_t key_offset;
	uint32_t reserved0;

	/* Size of key data in bytes (NOT strength of key in bits) */
	uint32_t key_size;
	uint32_t reserved1;

	/* Signature algorithm used by the key */
	uint32_t algorithm;
	uint32_t reserved2;

	/* Key version */
	uint32_t key_version;
	uint32_t reserved3;

	/* TODO: when redoing this struct, add a text description of the key */
} __attribute__((packed));

#define EXPECTED_VBPUBLICKEY_SIZE 32

/* Signature data (a secure hash, possibly signed) */
struct vb2_signature {
	/* Offset of signature data from start of this struct */
	uint32_t sig_offset;
	uint32_t reserved0;

	/* Size of signature data in bytes */
	uint32_t sig_size;
	uint32_t reserved1;

	/* Size of the data block which was signed in bytes */
	uint32_t data_size;
	uint32_t reserved2;

	/*
	 * TODO: when redoing this struct, add a text description of the
	 * signature (including what key was used) and an algorithm type field.
	 */
} __attribute__((packed));

#define EXPECTED_VBSIGNATURE_SIZE 24

#define KEY_BLOCK_MAGIC "CHROMEOS"
#define KEY_BLOCK_MAGIC_SIZE 8

#define KEY_BLOCK_HEADER_VERSION_MAJOR 2
#define KEY_BLOCK_HEADER_VERSION_MINOR 1

/*
 * The following flags set where the key is valid.  Not used by firmware
 * verification; only kernel verification.
 */
#define VB2_KEY_BLOCK_FLAG_DEVELOPER_0  0x01 /* Developer switch off */
#define VB2_KEY_BLOCK_FLAG_DEVELOPER_1  0x02 /* Developer switch on */
#define VB2_KEY_BLOCK_FLAG_RECOVERY_0   0x04 /* Not recovery mode */
#define VB2_KEY_BLOCK_FLAG_RECOVERY_1   0x08 /* Recovery mode */

/*
 * Key block, containing the public key used to sign some other chunk of data.
 *
 * This should be followed by:
 *   1) The data_key key data, pointed to by data_key.key_offset.
 *   2) The checksum data for (vb2_keyblock + data_key data), pointed to
 *      by keyblock_checksum.sig_offset.
 *   3) The signature data for (vb2_keyblock + data_key data), pointed to
 *      by keyblock_signature.sig_offset.
 */
struct vb2_keyblock {
	/* Magic number */
	uint8_t magic[KEY_BLOCK_MAGIC_SIZE];

	/* Version of this header format */
	uint32_t header_version_major;

	/* Version of this header format */
	uint32_t header_version_minor;

	/*
	 * Length of this entire key block, including keys, signatures, and
	 * padding, in bytes
	 */
	uint32_t keyblock_size;
	uint32_t reserved0;

	/*
	 * Signature for this key block (header + data pointed to by data_key)
	 * For use with signed data keys
	 */
	struct vb2_signature keyblock_signature;

	/*
	 * SHA-512 checksum for this key block (header + data pointed to by
	 * data_key) For use with unsigned data keys.
	 *
	 * Note that the vb2 lib currently only supports signed blocks.
	 */
	struct vb2_signature keyblock_checksum_unused;

	/* Flags for key (VB2_KEY_BLOCK_FLAG_*) */
	uint32_t keyblock_flags;
	uint32_t reserved1;

	/* Key to verify the chunk of data */
	struct vb2_packed_key data_key;
} __attribute__((packed));

#define EXPECTED_VB2KEYBLOCKHEADER_SIZE 112

/****************************************************************************/

/* Firmware preamble header */
#define FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR 2
#define FIRMWARE_PREAMBLE_HEADER_VERSION_MINOR 1

/* Flags for VbFirmwarePreambleHeader.flags */
/* Reserved; do not use */
#define VB2_FIRMWARE_PREAMBLE_RESERVED0 0x00000001

/* Premable block for rewritable firmware, version 2.1.
 *
 * The firmware preamble header should be followed by:
 *   1) The kernel_subkey key data, pointed to by kernel_subkey.key_offset.
 *   2) The signature data for the firmware body, pointed to by
 *      body_signature.sig_offset.
 *   3) The signature data for (header + kernel_subkey data + body signature
 *      data), pointed to by preamble_signature.sig_offset.
 */
struct vb2_fw_preamble {
	/*
	 * Size of this preamble, including keys, signatures, and padding, in
	 * bytes
	 */
	uint32_t preamble_size;
	uint32_t reserved0;

	/*
	 * Signature for this preamble (header + kernel subkey + body
	 * signature)
	 */
	struct vb2_signature preamble_signature;

	/* Version of this header format */
	uint32_t header_version_major;
	uint32_t header_version_minor;

	/* Firmware version */
	uint32_t firmware_version;
	uint32_t reserved1;

	/* Key to verify kernel key block */
	struct vb2_packed_key kernel_subkey;

	/* Signature for the firmware body */
	struct vb2_signature body_signature;

	/*
	 * Fields added in header version 2.1.  You must verify the header
	 * version before reading these fields!
	 */

	/*
	 * Flags; see VB2_FIRMWARE_PREAMBLE_*.  Readers should return 0 for
	 * header version < 2.1.
	 */
	uint32_t flags;
} __attribute__((packed));

#define EXPECTED_VB2FIRMWAREPREAMBLEHEADER2_1_SIZE 108

/****************************************************************************/

/* Flags for vb2_shared_data.flags */
enum vb2_shared_data_flags {
	/* User has explicitly and physically requested recovery */
	VB2_SD_FLAG_MANUAL_RECOVERY = (1 << 0),

	/* Developer mode is enabled */
	VB2_SD_DEV_MODE_ENABLED = (1 << 1),

	/*
	 * TODO: might be nice to add flags for why dev mode is enabled - via
	 * gbb, virtual dev switch, or forced on for testing.
	 */
};

/* Flags for vb2_shared_data.status */
enum vb2_shared_data_status {
	/* Reinitialized NV data due to invalid checksum */
	VB2_SD_STATUS_NV_REINIT = (1 << 0),

	/* NV data has been initialized */
	VB2_SD_STATUS_NV_INIT = (1 << 1),

	/* Secure data initialized */
	VB2_SD_STATUS_SECDATA_INIT = (1 << 2),

	/* Chose a firmware slot */
	VB2_SD_STATUS_CHOSE_SLOT = (1 << 3),
};

/*
 * Data shared between vboot API calls.  Stored at the start of the work
 * buffer.
 */
struct vb2_shared_data {
	/* Flags; see enum vb2_shared_data_flags */
	uint32_t flags;

	/* Flags from GBB header */
	uint32_t gbb_flags;

	/* Reason we are in recovery mode this boot, or 0 if we aren't */
	uint32_t recovery_reason;

	/* Firmware slot used last boot (0=A, 1=B) */
	uint32_t last_fw_slot;

	/* Result of last boot (enum vb2_fw_result) */
	uint32_t last_fw_result;

	/* Firmware slot used this boot */
	uint32_t fw_slot;

	/*
	 * Version for this slot (top 16 bits = key, lower 16 bits = firmware).
	 *
	 * TODO: Make this a union to allow getting/setting those versions
	 * separately?
	 */
	uint32_t fw_version;

	/*
	 * Status flags for this boot; see enum vb2_shared_data_status.  Status
	 * is "what we've done"; flags above are "decisions we've made".
	 */
	uint32_t status;

	/**********************************************************************
	 * Temporary variables used during firmware verification.  These don't
	 * really need to persist through to the OS, but there's nowhere else
	 * we can put them.
	 */

	/* Root key offset and size from GBB header */
	uint32_t gbb_rootkey_offset;
	uint32_t gbb_rootkey_size;

	/* Offset of preamble from start of vblock */
	uint32_t vblock_preamble_offset;

	/*
	 * Offset and size of packed data key in work buffer.  Size is 0 if
	 * data key is not stored in the work buffer.
	 */
	uint32_t workbuf_data_key_offset;
	uint32_t workbuf_data_key_size;

	/*
	 * Offset and size of firmware preamble in work buffer.  Size if 0 if
	 * preamble is not stored in the work buffer.
	 */
	uint32_t workbuf_preamble_offset;
	uint32_t workbuf_preamble_size;

	/*
	 * Offset and size of hash context in work buffer.  Size if 0 if
	 * hash context is not stored in the work buffer.
	 */
	uint32_t workbuf_hash_offset;
	uint32_t workbuf_hash_size;

	/* Current tag we're hashing */
	uint32_t hash_tag;

	/* Amount of data we still expect to hash */
	uint32_t hash_remaining_size;

} __attribute__((packed));

/****************************************************************************/

/* Signature at start of the GBB
 * Note that if you compile in the signature as is, you are likely to break any
 * tools that search for the signature. */
#define VB2_GBB_SIGNATURE "$GBB"
#define VB2_GBB_SIGNATURE_SIZE 4
#define VB2_GBB_XOR_CHARS "****"
/* TODO: can we write a macro to produce this at compile time? */
#define VB2_GBB_XOR_SIGNATURE { 0x0e, 0x6d, 0x68, 0x68 }

/* VB2 GBB struct version */
#define VB2_GBB_MAJOR_VER      1
#define VB2_GBB_MINOR_VER      1

/* Flags for vb2_gbb_header.flags */
enum vb2_gbb_flag {
	/*
	 * Reduce the dev screen delay to 2 sec from 30 sec to speed up
	 * factory.
	 */
	VB2_GBB_FLAG_DEV_SCREEN_SHORT_DELAY = (1 << 0),

	/*
	 * BIOS should load option ROMs from arbitrary PCI devices. We'll never
	 * enable this ourselves because it executes non-verified code, but if
	 * a customer wants to void their warranty and set this flag in the
	 * read-only flash, they should be able to do so.
	 */
	VB2_GBB_FLAG_LOAD_OPTION_ROMS = (1 << 1),

	/*
	 * The factory flow may need the BIOS to boot a non-ChromeOS kernel if
	 * the dev-switch is on. This flag allows that.
	 */
	VB2_GBB_FLAG_ENABLE_ALTERNATE_OS = (1 << 2),

	/*
	 * Force dev switch on, regardless of physical/keyboard dev switch
	 * position.
	 */
	VB2_GBB_FLAG_FORCE_DEV_SWITCH_ON = (1 << 3),

	/* Allow booting from USB in dev mode even if dev_boot_usb=0. */
	VB2_GBB_FLAG_FORCE_DEV_BOOT_USB = (1 << 4),

	/* Disable firmware rollback protection. */
	VB2_GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK = (1 << 5),

	/* Allow Enter key to trigger dev->tonorm screen transition */
	VB2_GBB_FLAG_ENTER_TRIGGERS_TONORM = (1 << 6),

	/* Allow booting Legacy OSes in dev mode even if dev_boot_legacy=0. */
	VB2_GBB_FLAG_FORCE_DEV_BOOT_LEGACY = (1 << 7),

	/* Allow booting using alternate keys for FAFT servo testing */
	VB2_GBB_FLAG_FAFT_KEY_OVERIDE = (1 << 8),

	/* Disable EC software sync */
	VB2_GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC = (1 << 9),

	/* Default to booting legacy OS when dev screen times out */
	VB2_GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY = (1 << 10),
};

struct vb2_gbb_header {
	/* Fields present in version 1.1 */
	uint8_t  signature[VB2_GBB_SIGNATURE_SIZE]; /* VB2_GBB_SIGNATURE */
	uint16_t major_version;   /* See VB2_GBB_MAJOR_VER */
	uint16_t minor_version;   /* See VB2_GBB_MINOR_VER */
	uint32_t header_size;     /* Size of GBB header in bytes */
	uint32_t flags;           /* Flags (see enum vb2_gbb_flag) */

	/* Offsets (from start of header) and sizes (in bytes) of components */
	uint32_t hwid_offset;		/* HWID */
	uint32_t hwid_size;
	uint32_t rootkey_offset;	/* Root key */
	uint32_t rootkey_size;
	uint32_t bmpfv_offset;		/* BMP FV */
	uint32_t bmpfv_size;
	uint32_t recovery_key_offset;	/* Recovery key */
	uint32_t recovery_key_size;

	uint8_t  pad[80]; /* To match GBB_HEADER_SIZE.  Initialize to 0. */
} __attribute__((packed));

#endif  /* VBOOT_REFERENCE_VBOOT_2STRUCT_H_ */
