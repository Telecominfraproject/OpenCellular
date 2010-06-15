/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "rollback_index.h"

#include <stdint.h>

#include "utility.h"
#include "tlcl.h"
#include "tss_constants.h"

uint16_t g_firmware_key_version = 0;
uint16_t g_firmware_version = 0;
uint16_t g_kernel_key_version = 0;
uint16_t g_kernel_version = 0;

#define RETURN_ON_FAILURE(tpm_command) do {             \
    uint32_t result;                                    \
    if ((result = (tpm_command)) != TPM_SUCCESS) {      \
      return result;                                    \
    }                                                   \
  } while (0)

static uint32_t InitializeKernelVersionsSpaces(void) {
  RETURN_ON_FAILURE(TlclDefineSpace(KERNEL_VERSIONS_NV_INDEX,
                                    TPM_NV_PER_PPWRITE, KERNEL_SPACE_SIZE));
  RETURN_ON_FAILURE(TlclWrite(KERNEL_VERSIONS_NV_INDEX, KERNEL_SPACE_INIT_DATA,
                              KERNEL_SPACE_SIZE));
  return TPM_SUCCESS;
}

/* When the return value is TPM_SUCCESS, this function sets *|initialized| to 1
 * if the spaces have been fully initialized, to 0 if not.  Otherwise
 * *|initialized| is not changed.
 */
static uint32_t GetSpacesInitialized(int* initialized) {
  uint32_t space_holder;
  uint32_t result;
  result = TlclRead(TPM_IS_INITIALIZED_NV_INDEX,
                    (uint8_t*) &space_holder, sizeof(space_holder));
  switch (result) {
  case TPM_SUCCESS:
    *initialized = 1;
    break;
  case TPM_E_BADINDEX:
    *initialized = 0;
    result = TPM_SUCCESS;
    break;
  }
  return result;
}

/* Creates the NVRAM spaces, and sets their initial values as needed.
 */
static uint32_t InitializeSpaces(void) {
  uint32_t zero = 0;
  uint32_t firmware_perm = TPM_NV_PER_GLOBALLOCK | TPM_NV_PER_PPWRITE;

  debug("Initializing spaces\n");

  RETURN_ON_FAILURE(TlclSetNvLocked());

  RETURN_ON_FAILURE(TlclDefineSpace(FIRMWARE_VERSIONS_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclWrite(FIRMWARE_VERSIONS_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));

  RETURN_ON_FAILURE(InitializeKernelVersionsSpaces());

  /* The space KERNEL_VERSIONS_BACKUP_NV_INDEX is used to protect the kernel
   * versions.  The content of space KERNEL_MUST_USE_BACKUP determines whether
   * only the backup value should be trusted.
   */
  RETURN_ON_FAILURE(TlclDefineSpace(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclDefineSpace(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclDefineSpace(DEVELOPER_MODE_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclWrite(DEVELOPER_MODE_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));

  /* The space TPM_IS_INITIALIZED_NV_INDEX is used to indicate that the TPM
   * initialization has completed.  Without it we cannot be sure that the last
   * space to be created was also initialized (power could have been lost right
   * after its creation).
   */
  RETURN_ON_FAILURE(TlclDefineSpace(TPM_IS_INITIALIZED_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  return TPM_SUCCESS;
}

static uint32_t SetDistrustKernelSpaceAtNextBoot(uint32_t distrust) {
  uint32_t must_use_backup;
  RETURN_ON_FAILURE(TlclRead(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                             (uint8_t*) &must_use_backup, sizeof(uint32_t)));
  if (must_use_backup != distrust) {
     RETURN_ON_FAILURE(TlclWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                 (uint8_t*) &distrust, sizeof(uint32_t)));
  }
  return TPM_SUCCESS;
}

static uint32_t GetTPMRollbackIndices(int type) {
  uint32_t firmware_versions;
  uint32_t kernel_versions;

  /* We perform the reads, making sure they succeed. A failure means that the
   * rollback index locations are missing or somehow messed up.  We let the
   * caller deal with that.
   */
  switch (type) {
  case FIRMWARE_VERSIONS:
    RETURN_ON_FAILURE(TlclRead(FIRMWARE_VERSIONS_NV_INDEX,
                               (uint8_t*) &firmware_versions,
                               sizeof(firmware_versions)));
    g_firmware_key_version = firmware_versions >> 16;
    g_firmware_version = firmware_versions && 0xffff;
    break;
  case KERNEL_VERSIONS:
    RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_NV_INDEX,
                               (uint8_t*) &kernel_versions,
                               sizeof(kernel_versions)));
    g_kernel_key_version = kernel_versions >> 16;
    g_kernel_version = kernel_versions && 0xffff;
    break;
  }

  return TPM_SUCCESS;
}

/* Checks if the kernel version space has been mucked with.  If it has,
 * reconstructs it using the backup value.
 */
uint32_t RecoverKernelSpace(void) {
  uint32_t perms = 0;
  uint8_t buffer[KERNEL_SPACE_SIZE];
  int read_OK = 0;
  int perms_OK = 0;
  uint32_t backup_combined_versions;
  uint32_t must_use_backup;

  RETURN_ON_FAILURE(TlclRead(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                             (uint8_t*) &must_use_backup, sizeof(uint32_t)));
  /* must_use_backup is true if the previous boot entered recovery mode. */

  read_OK = TlclRead(KERNEL_VERSIONS_NV_INDEX, (uint8_t*) &buffer,
                     KERNEL_SPACE_SIZE) == TPM_SUCCESS;
  if (read_OK) {
    RETURN_ON_FAILURE(TlclGetPermissions(KERNEL_VERSIONS_NV_INDEX, &perms));
    perms_OK = perms == TPM_NV_PER_PPWRITE;
  }
  if (!must_use_backup && read_OK && perms_OK &&
      !Memcmp(buffer + sizeof(uint32_t), KERNEL_SPACE_UID,
              KERNEL_SPACE_UID_SIZE)) {
    /* Everything is fine.  This is the normal, frequent path. */
    return TPM_SUCCESS;
  }

  /* Either we detected that something went wrong, or we cannot trust the
   * PP-protected kernel space.  Attempts to fix.  It is not always necessary
   * to redefine the space, but we might as well, since this path should be
   * taken quite seldom (after recovery mode and after an attack).
   */
  RETURN_ON_FAILURE(InitializeKernelVersionsSpaces());
  RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                             (uint8_t*) &backup_combined_versions,
                             sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclWrite(KERNEL_VERSIONS_NV_INDEX,
                              (uint8_t*) &backup_combined_versions,
                              sizeof(uint32_t)));
  if (must_use_backup) {
    uint32_t zero = 0;
    RETURN_ON_FAILURE(TlclWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                (uint8_t*) &zero, 0));
    
  }
  return TPM_SUCCESS;
}

static uint32_t BackupKernelSpace(void) {
  uint32_t kernel_versions;
  uint32_t backup_versions;
  RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_NV_INDEX,
                             (uint8_t*) &kernel_versions, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                             (uint8_t*) &backup_versions, sizeof(uint32_t)));
  if (kernel_versions == backup_versions) {
    return TPM_SUCCESS;
  } else if (kernel_versions < backup_versions) {
    /* This cannot happen.  We're screwed. */
    return TPM_E_INTERNAL_INCONSISTENCY;
  }
  RETURN_ON_FAILURE(TlclWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                              (uint8_t*) &kernel_versions, sizeof(uint32_t)));
  return TPM_SUCCESS;
}

/* Checks for transitions between protected mode to developer mode.  When going
 * into developer mode, clear the TPM.
 */
static uint32_t CheckDeveloperModeTransition(uint32_t current_developer) {
  uint32_t past_developer;
  int must_clear;
  RETURN_ON_FAILURE(TlclRead(DEVELOPER_MODE_NV_INDEX,
                             (uint8_t*) &past_developer,
                             sizeof(past_developer)));
  must_clear = current_developer != past_developer;
  if (must_clear) {
    RETURN_ON_FAILURE(TlclForceClear());
  }
  if (past_developer != current_developer) {
    /* (Unauthorized) writes to the TPM succeed even when the TPM is disabled
     * and deactivated.
     */
    RETURN_ON_FAILURE(TlclWrite(DEVELOPER_MODE_NV_INDEX,
                                (uint8_t*) &current_developer,
                                sizeof(current_developer)));
  }
  return must_clear ? TPM_E_MUST_REBOOT : TPM_SUCCESS;
}

static uint32_t SetupTPM_(int mode, int developer_flag) {
  uint8_t disable;
  uint8_t deactivated;
  TlclLibInit();
  RETURN_ON_FAILURE(TlclStartup());
  RETURN_ON_FAILURE(TlclContinueSelfTest());
  RETURN_ON_FAILURE(TlclAssertPhysicalPresence());
  /* Checks that the TPM is enabled and activated. */
  RETURN_ON_FAILURE(TlclGetFlags(&disable, &deactivated));
  if (disable || deactivated) {
    RETURN_ON_FAILURE(TlclSetEnable());
    RETURN_ON_FAILURE(TlclSetDeactivated(0));
    return TPM_E_MUST_REBOOT;
  }
  /* We expect this to fail the first time we run on a device, because the TPM
   * has not been initialized yet.
   */
  if (RecoverKernelSpace() != TPM_SUCCESS) {
    int initialized = 0;
    RETURN_ON_FAILURE(GetSpacesInitialized(&initialized));
    if (initialized) {
      return TPM_E_ALREADY_INITIALIZED;
    } else {
      RETURN_ON_FAILURE(InitializeSpaces());
      RETURN_ON_FAILURE(RecoverKernelSpace());
    }
  }
  RETURN_ON_FAILURE(BackupKernelSpace());
  RETURN_ON_FAILURE(SetDistrustKernelSpaceAtNextBoot(mode == RO_RECOVERY_MODE));
  RETURN_ON_FAILURE(GetTPMRollbackIndices(FIRMWARE_VERSIONS));
  RETURN_ON_FAILURE(GetTPMRollbackIndices(KERNEL_VERSIONS));

  RETURN_ON_FAILURE(CheckDeveloperModeTransition(developer_flag));

  /* As a courtesy (I hope) to the caller, lock the firmware versions if we are
   * in recovery mode.  The normal mode may need to update the firmware
   * versions, so they cannot be locked here.
   */
  if (mode == RO_RECOVERY_MODE) {
    RETURN_ON_FAILURE(LockFirmwareVersions());
  }
  return TPM_SUCCESS;
}

/* SetupTPM starts the TPM and establishes the root of trust for the
 * anti-rollback mechanism.  SetupTPM can fail for three reasons.  1 A bug. 2 a
 * TPM hardware failure. 3 An unexpected TPM state due to some attack.  In
 * general we cannot easily distinguish the kind of failure, so our strategy is
 * to reboot in recovery mode in all cases.  The recovery mode calls SetupTPM
 * again, which executes (almost) the same sequence of operations.  There is a
 * good chance that, if recovery mode was entered because of a TPM failure, the
 * failure will repeat itself.  (In general this is impossible to guarantee
 * because we have no way of creating the exact TPM initial state at the
 * previous boot.)  In recovery mode, we ignore the failure and continue, thus
 * giving the recovery kernel a chance to fix things (that's why we don't set
 * bGlobalLock).  The choice is between a knowingly insecure device and a
 * bricked device.
 *
 * As a side note, observe that we go through considerable hoops to avoid using
 * the STCLEAR permissions for the index spaces.  We do this to avoid writing
 * to the TPM flashram at every reboot or wake-up, because of concerns about
 * the durability of the NVRAM.
 */
uint32_t SetupTPM(int mode, int developer_flag) {
  switch (mode) {
  case RO_RECOVERY_MODE:
  case RO_NORMAL_MODE: {
    uint32_t result = SetupTPM_(mode, developer_flag);
    if (result == TPM_E_MAXNVWRITES) {
      /* ForceClears and reboots */
      RETURN_ON_FAILURE(TlclForceClear());
      return TPM_E_MUST_REBOOT;
    } else if (mode == RO_NORMAL_MODE) {
      return result;
    } else {
      /* In recovery mode we want to keep going even if there are errors. */
      return TPM_SUCCESS;
    }
  }
  case RW_NORMAL_MODE:
    /* There are no TPM writes here, so no need to check for write limit errors.
     */
    RETURN_ON_FAILURE(GetTPMRollbackIndices(KERNEL_VERSIONS));
  default:
    return TPM_E_INTERNAL_INCONSISTENCY;
  }
}

uint32_t GetStoredVersions(int type, uint16_t* key_version, uint16_t* version) {
  /* TODO: should verify that SetupTPM() has been called.
   *
   * Note that SetupTPM() does hardware setup AND sets global variables.  When
   * we get down into kernel verification, the hardware setup persists, but we
   * lose the global variables.
   */
  switch (type) {
    case FIRMWARE_VERSIONS:
      *key_version = g_firmware_key_version;
      *version = g_firmware_version;
      break;
    case KERNEL_VERSIONS:
      *key_version = g_kernel_key_version;
      *version = g_kernel_version;
      break;
  }

  return TPM_SUCCESS;
}

uint32_t WriteStoredVersions(int type, uint16_t key_version, uint16_t version) {
  uint32_t combined_version = (key_version << 16) & version;
  switch (type) {
    case FIRMWARE_VERSIONS:
      RETURN_ON_FAILURE(TlclWrite(FIRMWARE_VERSIONS_NV_INDEX,
                                  (uint8_t*) &combined_version,
                                  sizeof(uint32_t)));
      break;

    case KERNEL_VERSIONS:
      RETURN_ON_FAILURE(TlclWrite(KERNEL_VERSIONS_NV_INDEX,
                                  (uint8_t*) &combined_version,
                                  sizeof(uint32_t)));
  }
  return TPM_SUCCESS;
}

uint32_t LockFirmwareVersions() {
  return TlclSetGlobalLock();
}

uint32_t LockKernelVersionsByLockingPP() {
  return TlclLockPhysicalPresence();
}
