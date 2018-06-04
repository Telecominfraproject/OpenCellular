/* Copyright (c) 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Boot descriptor block structures
 */

#ifndef VBOOT_REFERENCE_BDB_STRUCT_H_
#define VBOOT_REFERENCE_BDB_STRUCT_H_

#include <stdint.h>

/* Size of SHA256 digest in bytes */
#define BDB_SHA256_DIGEST_SIZE 32

/* Size of RSA4096 key data in bytes */
#define BDB_RSA4096_KEY_DATA_SIZE 1032

/* Size of RSA4096 signature in bytes */
#define BDB_RSA4096_SIG_SIZE 512

/* Size of ECDSA521 key data in bytes = ceil(521/8) * 2 */
#define BDB_ECDSA521_KEY_DATA_SIZE 132

/* Size of ECDSA521 signature in bytes = ceil(521/8) * 2 */
#define BDB_ECDSA521_SIG_SIZE 132

/* Size of RSA3072B key data in bytes */
#define BDB_RSA3072B_KEY_DATA_SIZE 776

/* Size of RSA3072B signature in bytes */
#define BDB_RSA3072B_SIG_SIZE 384

/*****************************************************************************/
/* Header for BDB */

/* Magic number for bdb_header.struct_magic */
#define BDB_HEADER_MAGIC 0x30426442

/* Current version of bdb_header struct */
#define BDB_HEADER_VERSION_MAJOR 1
#define BDB_HEADER_VERSION_MINOR 0

/* Expected size of bdb_header struct in bytes */
#define BDB_HEADER_EXPECTED_SIZE 32

struct bdb_header {
	/* Magic number to identify struct = BDB_HEADER_MAGIC. */
	uint32_t struct_magic;

	/* Structure version = BDB_HEADER_VERSION{MAJOR,MINOR} */
	uint8_t struct_major_version;
	uint8_t struct_minor_version;

	/* Size of structure in bytes */
	uint16_t struct_size;

	/* Recommended address in SP SRAM to load BDB.  Set to -1 to use
	 * default address. */
	uint64_t bdb_load_address;

	/* Size of the entire BDB in bytes */
	uint32_t bdb_size;

	/* Number of bytes following the BDB key which are signed by the BDB
	 * header signature. */
	uint32_t signed_size;

	/* Size of OEM area 0 in bytes, or 0 if not present */
	uint32_t oem_area_0_size;

	/* Reserved; set 0 */
	uint8_t reserved0[8];
} __attribute__((packed));

/*****************************************************************************/
/* Public key structure for BDB */

/* Magic number for bdb_key.struct_magic */
#define BDB_KEY_MAGIC 0x73334256

/* Current version of bdb_key struct */
#define BDB_KEY_VERSION_MAJOR 1
#define BDB_KEY_VERSION_MINOR 0

/* Supported hash algorithms */
enum bdb_hash_alg {
	BDB_HASH_ALG_INVALID = 0,       /* Not used; invalid */
	BDB_HASH_ALG_SHA256 = 2,	/* SHA-256 */
};

/* Supported signature algorithms */
enum bdb_sig_alg {
	BDB_SIG_ALG_INVALID = 0,        /* Not used; invalid */
	BDB_SIG_ALG_RSA4096 = 3,	/* RSA-4096, exponent 65537 */
	BDB_SIG_ALG_ECSDSA521 = 5,	/* ECDSA-521 */
	BDB_SIG_ALG_RSA3072B = 7,	/* RSA_3072, exponent 3 */
};

/*
 * Expected size of bdb_key struct in bytes, not counting variable-length key
 * data at end.
 */
#define BDB_KEY_EXPECTED_SIZE 80

struct bdb_key {
	/* Magic number to identify struct = BDB_KEY_MAGIC. */
	uint32_t struct_magic;

	/* Structure version = BDB_KEY_VERSION{MAJOR,MINOR} */
	uint8_t struct_major_version;
	uint8_t struct_minor_version;

	/* Size of structure in bytes, including variable-length key data */
	uint16_t struct_size;

	/* Hash algorithm (enum bdb_hash_alg) */
	uint8_t hash_alg;

	/* Signature algorithm (enum bdb_sig_alg) */
	uint8_t sig_alg;

	/* Reserved; set 0 */
	uint8_t reserved0[2];

	/* Key version */
	uint32_t key_version;

 	/* Description; null-terminated ASCII */
	char description[128];

	/*
	 * Key data.  Variable-length; size is struct_size -
	 * offset_of(bdb_key, key_data).
	 */
	uint8_t key_data[0];
} __attribute__((packed));

/*****************************************************************************/
/* Signature structure for BDB */

/* Magic number for bdb_sig.struct_magic */
#define BDB_SIG_MAGIC 0x6b334256

/* Current version of bdb_sig struct */
#define BDB_SIG_VERSION_MAJOR 1
#define BDB_SIG_VERSION_MINOR 0

struct bdb_sig {
	/* Magic number to identify struct = BDB_SIG_MAGIC. */
	uint32_t struct_magic;

	/* Structure version = BDB_SIG_VERSION{MAJOR,MINOR} */
	uint8_t struct_major_version;
	uint8_t struct_minor_version;

	/* Size of structure in bytes, including variable-length signature
	 * data. */
	uint16_t struct_size;

	/* Hash algorithm used for this signature (enum bdb_hash_alg) */
	uint8_t hash_alg;

	/* Signature algorithm (enum bdb_sig_alg) */
	uint8_t sig_alg;

	/* Reserved; set 0 */
	uint8_t reserved0[2];

	/* Number of bytes of data signed by this signature */
	uint32_t signed_size;

	/* Description; null-terminated ASCII */
	char description[128];

	/* Signature data.  Variable-length; size is struct_size -
	 * offset_of(bdb_sig, sig_data). */
	uint8_t sig_data[0];
} __attribute__((packed));

/*****************************************************************************/
/* Data structure for BDB */

/* Magic number for bdb_data.struct_magic */
#define BDB_DATA_MAGIC 0x31426442

/* Current version of bdb_sig struct */
#define BDB_DATA_VERSION_MAJOR 1
#define BDB_DATA_VERSION_MINOR 0

struct bdb_data {
	/* Magic number to identify struct = BDB_DATA_MAGIC. */
	uint32_t struct_magic;

	/* Structure version = BDB_DATA_VERSION{MAJOR,MINOR} */
	uint8_t struct_major_version;
	uint8_t struct_minor_version;

	/* Size of structure in bytes, NOT including hashes which follow. */
	uint16_t struct_size;

	/* Version of data (RW firmware) contained */
	uint32_t data_version;

	/* Size of OEM area 1 in bytes, or 0 if not present */
	uint32_t oem_area_1_size;

	/* Number of hashes which follow */
	uint8_t num_hashes;

	/* Size of each hash entry in bytes */
	uint8_t hash_entry_size;

	/* Reserved; set 0 */
	uint8_t reserved0[2];

	/* Number of bytes of data signed by the subkey, including this
	 * header */
	uint32_t signed_size;

	/* Reserved; set 0 */
	uint8_t reserved1[8];

	/* Description; null-terminated ASCII */
	char description[128];
} __attribute__((packed));

/* Type of data for bdb_hash.type */
enum bdb_data_type {
	/* Types of data for boot descriptor blocks */
	BDB_DATA_SP_RW = 1,		/* SP-RW firmware */
	BDB_DATA_AP_RW = 2,		/* AP-RW firmware */
	BDB_DATA_MCU = 3,		/* MCU firmware */

	/* Types of data for kernel descriptor blocks */
	BDB_DATA_KERNEL = 128,		/* Kernel */
	BDB_DATA_CMD_LINE = 129,	/* Command line */
	BDB_DATA_HEADER16 = 130,	/* 16-bit vmlinuz header */
};

/* Hash entries which follow the structure */
struct bdb_hash {
	/* Offset of data from start of partition */
	uint64_t offset;

	/* Size of data in bytes */
	uint32_t size;

	/* Partition number containing data */
	uint8_t partition;

	/* Type of data; enum bdb_data_type */
	uint8_t type;

	/* Reserved; set 0 */
	uint8_t reserved0[2];

	/* Address in RAM to load data.  -1 means use default. */
	uint64_t load_address;

	/* SHA-256 hash digest */
	uint8_t digest[BDB_SHA256_DIGEST_SIZE];
} __attribute__((packed));

/*****************************************************************************/

#endif /* VBOOT_REFERENCE_BDB_STRUCT_H_ */

