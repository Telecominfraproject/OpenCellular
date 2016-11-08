/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#ifndef VBOOT_REFERENCE_ROLLBACK_INDEX_H_
#define VBOOT_REFERENCE_ROLLBACK_INDEX_H_

#include "sysincludes.h"
#include "tss_constants.h"

/* TPM NVRAM location indices. */
#define FIRMWARE_NV_INDEX               0x1007
#define KERNEL_NV_INDEX                 0x1008
/* This is just an opaque space for backup purposes */
#define BACKUP_NV_INDEX                 0x1009
#define BACKUP_NV_SIZE 16
#define FWMP_NV_INDEX			0x100a
#define FWMP_NV_MAX_SIZE 128
#define REC_HASH_NV_INDEX                0x100b
#define REC_HASH_NV_SIZE                 VB2_SHA256_DIGEST_SIZE

/* Structure definitions for TPM spaces */

/* Kernel space - KERNEL_NV_INDEX, locked with physical presence. */
#define ROLLBACK_SPACE_KERNEL_VERSION 2
#define ROLLBACK_SPACE_KERNEL_UID 0x4752574C  /* 'GRWL' */

typedef struct RollbackSpaceKernel {
	/* Struct version, for backwards compatibility */
	uint8_t struct_version;
	/* Unique ID to detect space redefinition */
	uint32_t uid;
	/* Kernel versions */
	uint32_t kernel_versions;
	/* Reserved for future expansion */
	uint8_t reserved[3];
	/* Checksum (v2 and later only) */
	uint8_t crc8;
} __attribute__((packed)) RollbackSpaceKernel;

/* Flags for firmware space */
/*
 * Last boot was developer mode.  TPM ownership is cleared when transitioning
 * to/from developer mode.
 */
#define FLAG_LAST_BOOT_DEVELOPER 0x01
/*
 * Some systems may not have a dedicated dev-mode switch, but enter and leave
 * dev-mode through some recovery-mode magic keypresses. For those systems, the
 * dev-mode "switch" state is in this bit (0=normal, 1=dev). To make it work, a
 * new flag is passed to VbInit(), indicating that the system lacks a physical
 * dev-mode switch. If a physical switch is present, this bit is ignored.
 */
#define FLAG_VIRTUAL_DEV_MODE_ON 0x02

/* Firmware space - FIRMWARE_NV_INDEX, locked with global lock. */
#define ROLLBACK_SPACE_FIRMWARE_VERSION 2

typedef struct RollbackSpaceFirmware {
	/* Struct version, for backwards compatibility */
	uint8_t struct_version;
	/* Flags (see FLAG_* above) */
	uint8_t flags;
	/* Firmware versions */
	uint32_t fw_versions;
	/* Reserved for future expansion */
	uint8_t reserved[3];
	/* Checksum (v2 and later only) */
	uint8_t crc8;
} __attribute__((packed)) RollbackSpaceFirmware;

#define FWMP_HASH_SIZE 32 /* Enough for SHA-256 */

/* Firmware management parameters */
struct RollbackSpaceFwmp {
	/* CRC-8 of fields following struct_size */
	uint8_t crc;
	/* Structure size in bytes */
	uint8_t struct_size;
	/* Structure version */
	uint8_t struct_version;
	/* Reserved; ignored by current reader */
	uint8_t reserved0;
	/* Flags; see enum fwmp_flags */
	uint32_t flags;
	/* Hash of developer kernel key */
	uint8_t dev_key_hash[FWMP_HASH_SIZE];
} __attribute__((packed));

#define ROLLBACK_SPACE_FWMP_VERSION 0x10  /* 1.0 */

enum fwmp_flags {
	FWMP_DEV_DISABLE_BOOT		= (1 << 0),
	FWMP_DEV_DISABLE_RECOVERY	= (1 << 1),
	FWMP_DEV_ENABLE_USB		= (1 << 2),
	FWMP_DEV_ENABLE_LEGACY		= (1 << 3),
	FWMP_DEV_ENABLE_OFFICIAL_ONLY	= (1 << 4),
	FWMP_DEV_USE_KEY_HASH		= (1 << 5),
};

/* All functions return TPM_SUCCESS (zero) if successful, non-zero if error */

/*
 * These functions are callable from VbSelectAndLoadKernel().  They may use
 * global variables.
 */

/**
 * Read stored kernel version.
 */
uint32_t RollbackKernelRead(uint32_t *version);

/**
 * Write stored kernel version.
 */
uint32_t RollbackKernelWrite(uint32_t version);

/**
 * Lock must be called.  Internally, it's ignored in recovery mode.
 */
uint32_t RollbackKernelLock(int recovery_mode);

/**
 * Read and validate firmware management parameters.
 *
 * Absence of a FWMP is not an error; in this case, fwmp will be cleared.
 *
 * Returns non-zero if error.
 */
uint32_t RollbackFwmpRead(struct RollbackSpaceFwmp *fwmp);

/****************************************************************************/

/*
 * The following functions are internal apis, listed here for use by unit tests
 * only.
 */

/**
 * Issue a TPM_Clear and reenable/reactivate the TPM.
 */
uint32_t TPMClearAndReenable(void);

/**
 * Like TlclWrite(), but checks for write errors due to hitting the 64-write
 * limit and clears the TPM when that happens.  This can only happen when the
 * TPM is unowned, so it is OK to clear it (and we really have no choice).
 * This is not expected to happen frequently, but it could happen.
 */
uint32_t SafeWrite(uint32_t index, const void *data, uint32_t length);

/**
 * Utility function to turn the virtual dev-mode flag on or off. 0=off, 1=on.
 */
uint32_t SetVirtualDevMode(int val);

#endif  /* VBOOT_REFERENCE_ROLLBACK_INDEX_H_ */
