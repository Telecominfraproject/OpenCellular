/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
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

/* Structure definitions for TPM spaces */

__pragma(pack(push, 1)) /* Support packing for MSVC. */

/* Kernel space - KERNEL_NV_INDEX, locked with physical presence. */
#define ROLLBACK_SPACE_KERNEL_VERSION 1
#define ROLLBACK_SPACE_KERNEL_UID 0x4752574C  /* 'GRWL' */
typedef struct RollbackSpaceKernel {
  uint8_t  struct_version;      /* Struct version, for backwards
                                 * compatibility */
  uint32_t uid;                 /* Unique ID to detect space redefinition */
  uint32_t kernel_versions;     /* Kernel versions */
  uint32_t reserved;            /* Reserved for future expansion */
} __attribute__((packed)) RollbackSpaceKernel;


/* Flags for firmware space */
/* Last boot was developer mode.  TPM ownership is cleared when
 * transitioning to/from developer mode. */
#define FLAG_LAST_BOOT_DEVELOPER 0x01
/* There have been one or more boots which left PP unlocked, so the
 * contents of the kernel space are untrusted and must be restored
 * from the backup copy. */
#define FLAG_KERNEL_SPACE_USE_BACKUP 0x02

#define ROLLBACK_SPACE_FIRMWARE_VERSION 1
/* Firmware space - FIRMWARE_NV_INDEX, locked with global lock. */
typedef struct RollbackSpaceFirmware {
  uint8_t  struct_version;  /* Struct version, for backwards compatibility */
  uint8_t  flags;           /* Flags (see FLAG_* above) */
  uint32_t fw_versions;     /* Firmware versions */
  uint32_t reserved;            /* Reserved for future expansion */
} __attribute__((packed)) RollbackSpaceFirmware;

__pragma(pack(pop)) /* Support packing for MSVC. */


/* All functions return TPM_SUCCESS (zero) if successful, non-zero if error */

/* These functions are called from VbInit().  They cannot use global
 * variables. */
uint32_t RollbackS3Resume(void);

/* These functions are callable from VbSelectFirmware().  They cannot use
 * global variables. */

/* Setup must be called.  Pass recovery_mode=nonzero if in recovery
 * mode.  Pass developer_mode=nonzero if in developer
 * mode. */
uint32_t RollbackFirmwareSetup(int recovery_mode, int developer_mode,
                               uint32_t* version);

/* Write may be called if the versions change */
uint32_t RollbackFirmwareWrite(uint32_t version);

/* Lock must be called */
uint32_t RollbackFirmwareLock(void);

/* These functions are callable from VbSelectAndLoadKernel().  They
 * may use global variables. */

/* Read and write may be called to read and write the kernel version. */
uint32_t RollbackKernelRead(uint32_t* version);
uint32_t RollbackKernelWrite(uint32_t version);

/* Lock must be called.  Internally, it's ignored in recovery mode. */
uint32_t RollbackKernelLock(void);

/****************************************************************************/
/* The following functions are internal apis, listed here for use by
 * unit tests only. */

/* Issue a TPM_Clear and reenable/reactivate the TPM. */
uint32_t TPMClearAndReenable(void);

/* Like TlclWrite(), but checks for write errors due to hitting the 64-write
 * limit and clears the TPM when that happens.  This can only happen when the
 * TPM is unowned, so it is OK to clear it (and we really have no choice).
 * This is not expected to happen frequently, but it could happen. */
uint32_t SafeWrite(uint32_t index, const void* data, uint32_t length);

/* Similarly to SafeWrite(), this ensures we don't fail a DefineSpace because
 * we hit the TPM write limit.  This is even less likely to happen than with
 * writes because we only define spaces once at initialization, but we'd rather
 * be paranoid about this. */
uint32_t SafeDefineSpace(uint32_t index, uint32_t perm, uint32_t size);

/* Performs one-time initializations.  Creates the NVRAM spaces, and sets their
 * initial values as needed.  Sets the nvLocked bit and ensures the physical
 * presence command is enabled and locked.
 */
uint32_t OneTimeInitializeTPM(RollbackSpaceFirmware* rsf,
                              RollbackSpaceKernel* rsk);

/* SetupTPM starts the TPM and establishes the root of trust for the
 * anti-rollback mechanism. */
uint32_t SetupTPM(int recovery_mode, int developer_mode,
                  RollbackSpaceFirmware* rsf);

#endif  /* VBOOT_REFERENCE_ROLLBACK_INDEX_H_ */
