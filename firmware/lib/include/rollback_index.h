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

/* Rollback version types. */
#define FIRMWARE_VERSIONS 0
#define KERNEL_VERSIONS   1

/* Initialization mode */
#define RO_RECOVERY_MODE 0
#define RO_NORMAL_MODE   1
#define RW_NORMAL_MODE   2

/* TPM NVRAM location indices. */
#define FIRMWARE_VERSIONS_NV_INDEX      0x1001
#define KERNEL_VERSIONS_NV_INDEX        0x1002
#define TPM_IS_INITIALIZED_NV_INDEX     0x1003
#define KERNEL_VERSIONS_BACKUP_NV_INDEX 0x1004
#define KERNEL_MUST_USE_BACKUP_NV_INDEX 0x1005
#define DEVELOPER_MODE_NV_INDEX         0x1006

/* Unique ID to detect kernel space redefinition */
#define KERNEL_SPACE_UID "GRWL"        /* unique ID with secret meaning */
#define KERNEL_SPACE_UID_SIZE (sizeof(KERNEL_SPACE_UID) - 1)
#define KERNEL_SPACE_INIT_DATA ((uint8_t*) "\0\0\0\0" KERNEL_SPACE_UID)
#define KERNEL_SPACE_SIZE (sizeof(uint32_t) + KERNEL_SPACE_UID_SIZE)

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

/* These functions are callable from LoadFirmware().  They cannot use
 * global variables. */

/* Setup must be called.  Pass developer_mode=nonzero if in developer
 * mode. */
uint32_t RollbackFirmwareSetup(int developer_mode);
/* Read and Write may be called after Setup. */
uint32_t RollbackFirmwareRead(uint16_t* key_version, uint16_t* version);
/* Write may be called if the versions change */
uint32_t RollbackFirmwareWrite(uint16_t key_version, uint16_t version);

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
uint32_t RollbackKernelRead(uint16_t* key_version, uint16_t* version);
uint32_t RollbackKernelWrite(uint16_t key_version, uint16_t version);

/* Lock must be called.  Internally, it's ignored in recovery mode. */
uint32_t RollbackKernelLock(void);

/* The following functions are here for testing only. */

/* Store 1 in *|initialized| if the TPM NVRAM spaces have been initialized, 0
 * otherwise.  Return TPM errors. */
uint32_t GetSpacesInitialized(int* initialized);

/* Issue a TPM_Clear and reenable/reactivate the TPM. */
uint32_t TPMClearAndReenable(void);

#endif  /* VBOOT_REFERENCE_ROLLBACK_INDEX_H_ */
