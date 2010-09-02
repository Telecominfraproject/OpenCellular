/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
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

/*

Call from LoadFirmware()
  Normal or developer mode (not recovery)
  Wants firmware versions
  Must send in developer flag

  RollbackFirmwareSetup(IN devmode)
  (maybe) RollbackFirmwareRead()
  (maybe) RollbackFirmwareWrite()
  RollbackFirmwareLock()

Call from LoadKernel()

  RollbackKernelRecovery(IN devmode)
     (implies LockFirmwareVersions() inside the setup)

  RollbackKernelRead(OUT kernel versions)
  (maybe) RollbackKernelWrite()
  RollbackKernelLock()

  Any mode
    If recovery mode, this is the first time we've been called
      Must send in developer flag
    If not recovery mode, wants kernel versions
  Must send in developer and recovery flags
*/

/* These functions are called from S3Resume().  They cannot use
 * global variables. */
uint32_t RollbackS3Resume(void);

/* These functions are callable from LoadFirmware().  They cannot use
 * global variables. */

/* Setup must be called.  Pass developer_mode=nonzero if in developer
 * mode. */
uint32_t RollbackFirmwareSetup(int developer_mode, uint32_t* version);

/* Write may be called if the versions change */
uint32_t RollbackFirmwareWrite(uint32_t version);

/* Lock must be called */
uint32_t RollbackFirmwareLock(void);

/* These functions are callable from LoadKernel().  They may use global
 * variables. */

/* Recovery may be called.  If it is, this is the first time a
 * rollback function has been called this boot, so it needs to know if
 * we're in developer mode.  Pass developer_mode=nonzero if in developer
 * mode. */
uint32_t RollbackKernelRecovery(int developer_mode);

/* Read and write may be called if not in developer mode.  If called in
 * recovery mode, the effect is undefined. */
uint32_t RollbackKernelRead(uint32_t* version);
uint32_t RollbackKernelWrite(uint32_t version);

/* Lock must be called.  Internally, it's ignored in recovery mode. */
uint32_t RollbackKernelLock(void);

/* The following functions are here for testing only. */

/* Issue a TPM_Clear and reenable/reactivate the TPM. */
uint32_t TPMClearAndReenable(void);

#endif  /* VBOOT_REFERENCE_ROLLBACK_INDEX_H_ */
