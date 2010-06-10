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
   * versions when entering recovery mode.  The content of space
   * KERNEL_MUST_USE_BACKUP determines whether the backup value (1) or the
   * regular value (0) should be trusted.
   */
  RETURN_ON_FAILURE(TlclDefineSpace(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclDefineSpace(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(TlclWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
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

/* Enters the recovery mode.  If |unlocked| is true, there is some problem with
 * the TPM, so do not attempt to do any more TPM operations, and particularly
 * do not set bGlobalLock.
 */
void EnterRecovery(int unlocked) {
  uint32_t combined_versions;
  uint32_t backup_versions;
  uint32_t must_use_backup;
  uint32_t result;

  if (!unlocked) {
    /* Saves the kernel versions and indicates that we should trust the saved
     * ones.
     */
    if (TlclRead(KERNEL_VERSIONS_NV_INDEX, (uint8_t*) &combined_versions,
                 sizeof(uint32_t)) != TPM_SUCCESS)
      goto recovery_mode;
    if (TlclRead(KERNEL_VERSIONS_BACKUP_NV_INDEX, (uint8_t*) &backup_versions,
                 sizeof(uint32_t)) != TPM_SUCCESS)
      goto recovery_mode;
    /* Avoids idempotent writes. */
    if (combined_versions != backup_versions) {
      result = TlclWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                         (uint8_t*) &combined_versions, sizeof(uint32_t));
      if (result == TPM_E_MAXNVWRITES) {
        goto forceclear_and_reboot;
      } else if (result != TPM_SUCCESS) {
        goto recovery_mode;
      }
    }

    if (TlclRead(KERNEL_MUST_USE_BACKUP_NV_INDEX, (uint8_t*) &must_use_backup,
                 sizeof(uint32_t)) != TPM_SUCCESS)
      goto recovery_mode;
    if (must_use_backup != 1) {
      must_use_backup = 1;
      result = TlclWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                         (uint8_t*) &must_use_backup, sizeof(uint32_t));
      if (result == TPM_E_MAXNVWRITES) {
        goto forceclear_and_reboot;
      } else if (result != TPM_SUCCESS) {
        goto recovery_mode;
      }
    }
    /* Protects the firmware and backup kernel versions. */
    if (LockFirmwareVersions() != TPM_SUCCESS)
      goto recovery_mode;
  }

 recovery_mode:
  debug("entering recovery mode");

  /* TODO(nelson): code for entering recovery mode. */

 forceclear_and_reboot:
  if (TlclForceClear() != TPM_SUCCESS) {
    goto recovery_mode;
  }
  /* TODO: reboot */
}

static uint32_t GetTPMRollbackIndices(void) {
  uint32_t firmware_versions;
  uint32_t kernel_versions;

  /* We perform the reads, making sure they succeed. A failure means that the
   * rollback index locations are missing or somehow messed up.  We let the
   * caller deal with that.
   */
  RETURN_ON_FAILURE(TlclRead(FIRMWARE_VERSIONS_NV_INDEX,
                             (uint8_t*) &firmware_versions,
                             sizeof(firmware_versions)));
  RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_NV_INDEX,
                             (uint8_t*) &kernel_versions,
                             sizeof(kernel_versions)));

  g_firmware_key_version = firmware_versions >> 16;
  g_firmware_version = firmware_versions && 0xffff;
  g_kernel_key_version = kernel_versions >> 16;
  g_kernel_version = kernel_versions && 0xffff;

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

static uint32_t SetupTPM_(void) {
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
    /* TODO: Reboot */
    return 9999;
  }
  /* We expect this to fail the first time we run on a device, indicating that
   * the TPM has not been initialized yet. */
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
  RETURN_ON_FAILURE(GetTPMRollbackIndices());

  return TPM_SUCCESS;
}

uint32_t SetupTPM(void) {
  uint32_t result = SetupTPM_();
  if (result == TPM_E_MAXNVWRITES) {
    /* ForceClears and reboots */
    RETURN_ON_FAILURE(TlclForceClear());
    /* TODO: reboot */
    return 9999;
  } else {
    return result;
  }
}

uint32_t GetStoredVersions(int type, uint16_t* key_version, uint16_t* version) {

  /* TODO: should verify that SetupTPM() has been called.  Note that
   * SetupTPM() does hardware setup AND sets global variables.  When we
   * get down into kernel verification, the hardware setup persists, but
   * we don't have access to the global variables.  So I guess we DO need
   * to call SetupTPM() there, and have it be smart enough not to redo the
   * hardware init, but it still needs to re-read the flags... */

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
