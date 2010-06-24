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

/* TODO: global variables won't work in the boot stub, since it runs
   directly out of ROM. */
extern uint16_t g_firmware_key_version;
extern uint16_t g_firmware_version;
extern uint16_t g_kernel_key_version;
extern uint16_t g_kernel_version;

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

  RollbackFirmwareSetup(IN devmode, OUT firmware versions)
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
uint32_t RollbackFirmwareSetup(int developer_mode,
                               uint16_t* key_version, uint16_t* version);
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
 * recovery mode, these are ignored and/or return 0 versions. */
uint32_t RollbackKernelRead(uint16_t* key_version, uint16_t* version);
uint32_t RollbackKernelWrite(uint16_t key_version, uint16_t version);
/* Lock must be called.  Internally, it's ignored in recovery mode. */
uint32_t RollbackKernelLock(void);


/* SetupTPM is called on boot and on starting the RW firmware, passing the
 * appripriate MODE and DEVELOPER_FLAG parameters.  MODE can be one of
 * RO_RECOVERY_MODE, RO_NORMAL_MODE, RW_NORMAL_MODE.  DEVELOPER_FLAG is 1 when
 * the developer switch is ON, 0 otherwise.
 *
 * If SetupTPM returns TPM_SUCCESS, the caller may proceed.  If it returns
 * TPM_E_MUST_REBOOT, the caller must reboot in the current mode.  For all
 * other return values, the caller must reboot in recovery mode.
 *
 * This function has many side effects on the TPM state.  In particular, when
 * called with mode = RECOVERY_MODE, it locks the firmware versions before
 * returning.  In all other cases, the caller is responsible for locking the
 * firmware versions once it decides it doesn't need to update them.
 */
uint32_t SetupTPM(int mode, int developer_flag);
uint32_t GetStoredVersions(int type, uint16_t* key_version, uint16_t* version);
uint32_t WriteStoredVersions(int type, uint16_t key_version, uint16_t version);
uint32_t LockFirmwareVersions(void);
uint32_t LockKernelVersionsByLockingPP(void);

#endif  /* VBOOT_REFERENCE_ROLLBACK_INDEX_H_ */
