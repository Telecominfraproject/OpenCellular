/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Vboot 2.0 data structures (compatible with vboot1)
 *
 * Note: Many of the structs have pairs of 32-bit fields and reserved fields.
 * This is to be backwards-compatible with older verified boot data which used
 * 64-bit fields (when we thought that hey, UEFI is 64-bit so all our fields
 * should be too).
 *
 * Offsets should be padded to 32-bit boundaries, since some architectures
 * have trouble with accessing unaligned integers.
 */

#ifndef VBOOT_REFERENCE_VB2_STRUCT_H_
#define VBOOT_REFERENCE_VB2_STRUCT_H_
#include <stdint.h>

/* Packed public key data */
struct vb2_packed_key {
	/* Offset of key data from start of this struct */
	uint32_t key_offset;
	uint32_t reserved0;

	/* Size of key data in bytes (NOT strength of key in bits) */
	uint32_t key_size;
	uint32_t reserved1;

	/* Signature algorithm used by the key (enum vb2_crypto_algorithm) */
	uint32_t algorithm;
	uint32_t reserved2;

	/* Key version */
	uint32_t key_version;
	uint32_t reserved3;

	/* TODO: when redoing this struct, add a text description of the key */
} __attribute__((packed));

#define EXPECTED_VB2_PACKED_KEY_SIZE 32


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
} __attribute__((packed));

#define EXPECTED_VB2_SIGNATURE_SIZE 24


#define KEY_BLOCK_MAGIC "CHROMEOS"
#define KEY_BLOCK_MAGIC_SIZE 8

#define KEY_BLOCK_HEADER_VERSION_MAJOR 2
#define KEY_BLOCK_HEADER_VERSION_MINOR 1

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

#define EXPECTED_VB2_KEYBLOCK_SIZE 112


/* Firmware preamble header */
#define FIRMWARE_PREAMBLE_HEADER_VERSION_MAJOR 2
#define FIRMWARE_PREAMBLE_HEADER_VERSION_MINOR 1

/* Flags for VbFirmwarePreambleHeader.flags */
/* Reserved; do not use */
#define VB2_FIRMWARE_PREAMBLE_RESERVED0 0x00000001
/* Do not allow use of any hardware crypto accelerators. */
#define VB2_FIRMWARE_PREAMBLE_DISALLOW_HWCRYPTO 0x00000002

/* Premable block for rewritable firmware, vboot1 version 2.1.
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

#define EXPECTED_VB2_FW_PREAMBLE_SIZE 108

#endif  /* VBOOT_REFERENCE_VB2_STRUCT_H_ */
