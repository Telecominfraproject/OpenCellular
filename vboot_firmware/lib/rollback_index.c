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

static int InitializeSpaces(void) {
  uint32_t zero = 0;
  uint32_t space_holder;
  uint32_t firmware_perm = TPM_NV_PER_GLOBALLOCK | TPM_NV_PER_PPWRITE;
  uint32_t kernel_perm = TPM_NV_PER_PPWRITE;

  debug("Initializing spaces\n");

  if (TlclRead(TPM_IS_INITIALIZED_NV_INDEX,
               (uint8_t*) &space_holder, sizeof(space_holder)) == TPM_SUCCESS) {
    /* Spaces are already initialized, so this is an error */
    return 0;
  }

  TlclSetNvLocked();

  TlclDefineSpace(FIRMWARE_VERSIONS_NV_INDEX, firmware_perm, sizeof(uint32_t));
  TlclWrite(FIRMWARE_VERSIONS_NV_INDEX, (uint8_t*) &zero, sizeof(uint32_t));

  TlclDefineSpace(KERNEL_VERSIONS_NV_INDEX, kernel_perm, sizeof(uint32_t));
  TlclWrite(KERNEL_VERSIONS_NV_INDEX, (uint8_t*) &zero, sizeof(uint32_t));

  /* The space KERNEL_VERSIONS_BACKUP_NV_INDEX is used to protect the kernel
   * versions when entering recovery mode.  The content of space
   * KERNEL_BACKUP_IS_VALID determines whether the backup value (1) or the
   * regular value (0) should be trusted.
   */
  TlclDefineSpace(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                  firmware_perm, sizeof(uint32_t));
  TlclWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
            (uint8_t*) &zero, sizeof(uint32_t));
  TlclDefineSpace(KERNEL_BACKUP_IS_VALID_NV_INDEX,
                  firmware_perm, sizeof(uint32_t));
  TlclWrite(KERNEL_BACKUP_IS_VALID_NV_INDEX,
            (uint8_t*) &zero, sizeof(uint32_t));

  /* The space TPM_IS_INITIALIZED_NV_INDEX is used to indicate that the TPM
   * initialization has completed.  Without it we cannot be sure that the last
   * space to be created was also initialized (power could have been lost right
   * after its creation).
   */
  TlclDefineSpace(TPM_IS_INITIALIZED_NV_INDEX, firmware_perm, sizeof(uint32_t));
  return 1;
}

/* Enters the recovery mode.  If |unlocked| is true, there is some problem with
 * the TPM, so do not attempt to do any more TPM operations, and particularly
 * do not set bGlobalLock.
 */
static void EnterRecovery(int unlocked) {
  uint32_t combined_versions;
  uint32_t backup_versions;
  uint32_t backup_is_valid;

  if (!unlocked) {
    /* Saves the kernel versions and indicates that we should trust the saved
     * ones.
     */
    TlclRead(KERNEL_VERSIONS_NV_INDEX,
             (uint8_t*) &combined_versions, sizeof(uint32_t));
    TlclRead(KERNEL_VERSIONS_BACKUP_NV_INDEX,
             (uint8_t*) &backup_versions, sizeof(uint32_t));
    /* We could unconditional writes of both KERNEL_VERSIONS_BACKUP and
     * KERNEL_BACKUP_IS_VALID, but this is more robust.
     */
    if (combined_versions != backup_versions) {
      TlclWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                (uint8_t*) &combined_versions, sizeof(uint32_t));
    }

    TlclRead(KERNEL_BACKUP_IS_VALID_NV_INDEX,
             (uint8_t*) &backup_is_valid, sizeof(uint32_t));
    if (backup_is_valid != 1) {
      backup_is_valid = 1;
      TlclWrite(KERNEL_BACKUP_IS_VALID_NV_INDEX, (uint8_t*) &backup_is_valid,
                sizeof(uint32_t));
    }
    /* Protects the firmware and backup kernel versions. */
    LockFirmwareVersions();
  }
  debug("entering recovery mode");

  /* TODO(nelson): code for entering recovery mode. */
}

static int GetTPMRollbackIndices(void) {
  uint32_t backup_is_valid;
  uint32_t firmware_versions;
  uint32_t kernel_versions;

  if (TlclRead(KERNEL_BACKUP_IS_VALID_NV_INDEX, (uint8_t*) &backup_is_valid,
               sizeof(uint32_t)) != TPM_SUCCESS) {
    EnterRecovery(1);
  }
  if (backup_is_valid) {
    /* We reach this path if the previous boot went into recovery mode and we
     * made a copy of the kernel versions to protect them.
     */
    uint32_t protected_combined_versions;
    uint32_t unsafe_combined_versions;
    uint32_t result;
    uint32_t zero = 0;
    if (TlclRead(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                 (uint8_t*) &protected_combined_versions,
                 sizeof(uint32_t)) != TPM_SUCCESS) {
      EnterRecovery(1);
    }
    result = TlclRead(KERNEL_VERSIONS_NV_INDEX,
                      (uint8_t*) &unsafe_combined_versions, sizeof(uint32_t));
    if (result == TPM_E_BADINDEX) {
      /* Jeez, someone removed the space.  This is either hostile or extremely
       * incompetent.  Foo to them.  Politeness and lack of an adequate
       * character set prevent me from expressing my true feelings.
       */
      TlclDefineSpace(KERNEL_VERSIONS_NV_INDEX, TPM_NV_PER_PPWRITE,
                      sizeof(uint32_t));
    } else if (result != TPM_SUCCESS) {
      EnterRecovery(1);
    }
    if (result == TPM_E_BADINDEX ||
        protected_combined_versions != unsafe_combined_versions) {
      TlclWrite(KERNEL_VERSIONS_NV_INDEX,
                (uint8_t*) &protected_combined_versions, sizeof(uint32_t));
    }
    /* We recovered the backed-up versions and now we can reset the
     * BACKUP_IS_VALID flag.
     */
    TlclWrite(KERNEL_BACKUP_IS_VALID_NV_INDEX, (uint8_t*) &zero, 0);

    /* TODO(nelson): ForceClear and reboot if unowned. */
  }

  /* We perform the reads, making sure they succeed. A failure means that the
   * rollback index locations are missing or somehow messed up.  We let the
   * caller deal with that.
   */
  if (TPM_SUCCESS != TlclRead(FIRMWARE_VERSIONS_NV_INDEX,
                              (uint8_t*) &firmware_versions,
                              sizeof(firmware_versions)) ||
      TPM_SUCCESS != TlclRead(KERNEL_VERSIONS_NV_INDEX,
                              (uint8_t*) &kernel_versions,
                              sizeof(kernel_versions)))
    return 0;

  g_firmware_key_version = firmware_versions >> 16;
  g_firmware_version = firmware_versions && 0xffff;
  g_kernel_key_version = kernel_versions >> 16;
  g_kernel_version = kernel_versions && 0xffff;

  return 1;
}


void SetupTPM(void) {
  uint8_t disable;
  uint8_t deactivated;
  TlclLibinit();
  TlclStartup();
  /* TODO(gauravsh): The call to self test  should probably be deferred.
   * As per semenzato@chromium.org -
   * TlclStartup should be called before the firmware initializes the memory
   * controller, so the selftest can run in parallel with that. Here we should
   * just call TlclSelftestFull to make sure the self test has
   * completed---unless we want to rely on the NVRAM operations being available
   * before the selftest completes. */
  TlclSelftestfull();
  TlclAssertPhysicalPresence();
  /* Check that the TPM is enabled and activated. */
  if(TlclGetFlags(&disable, &deactivated) != TPM_SUCCESS) {
    debug("failed to get TPM flags");
    EnterRecovery(1);
  }
  if (disable || deactivated) {
    TlclSetEnable();
    if (TlclSetDeactivated(0) != TPM_SUCCESS) {
      debug("failed to activate TPM");
      EnterRecovery(1);
    }
  }
  /* We expect this to fail the first time we run on a device, indicating that
   * the TPM has not been initialized yet. */
  if (!GetTPMRollbackIndices()) {
    debug("failed to get rollback indices");
    if (!InitializeSpaces()) {
      /* If InitializeSpaces() fails (possibly because it had been executed
       * already), something is wrong. */
      EnterRecovery(1);
    }
  }
}

void GetStoredVersions(int type, uint16_t* key_version, uint16_t* version) {
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
}

int WriteStoredVersions(int type, uint16_t key_version, uint16_t version) {
  uint32_t combined_version = (key_version << 16) & version;
  switch (type) {
    case FIRMWARE_VERSIONS:
      return (TPM_SUCCESS == TlclWrite(FIRMWARE_VERSIONS_NV_INDEX,
                                       (uint8_t*) &combined_version,
                                       sizeof(uint32_t)));
      break;
    case KERNEL_VERSIONS:
      return (TPM_SUCCESS == TlclWrite(KERNEL_VERSIONS_NV_INDEX,
                                       (uint8_t*) &combined_version,
                                       sizeof(uint32_t)));
      break;
  }
  /* TODO(nelson): ForceClear and reboot if unowned. */

  return 0;
}

void LockFirmwareVersions() {
  if (TlclSetGlobalLock() != TPM_SUCCESS) {
    debug("failed to set global lock");
    EnterRecovery(1);
  }
}

void LockKernelVersionsByLockingPP() {
  if (TlclLockPhysicalPresence() != TPM_SUCCESS) {
    debug("failed to turn off PP");
    EnterRecovery(1);
  }
}
