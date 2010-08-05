/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "rollback_index.h"

#include "tlcl.h"
#include "tss_constants.h"
#include "utility.h"

static int g_rollback_recovery_mode = 0;

/* disable MSVC warning on const logical expression (as in } while(0);) */
__pragma(warning (disable: 4127))

#define RETURN_ON_FAILURE(tpm_command) do {             \
    uint32_t result;                                    \
    if ((result = (tpm_command)) != TPM_SUCCESS) {      \
      VBDEBUG(("Rollback: %08x returned by " #tpm_command "\n", (int)result)); \
      return result;                                    \
    }                                                   \
  } while (0)

uint32_t TPMClearAndReenable(void) {
  VBDEBUG(("TPM: Clear and re-enable\n"));
  RETURN_ON_FAILURE(TlclForceClear());
  RETURN_ON_FAILURE(TlclSetEnable());
  RETURN_ON_FAILURE(TlclSetDeactivated(0));

  return TPM_SUCCESS;
}

/* Like TlclWrite(), but checks for write errors due to hitting the 64-write
 * limit and clears the TPM when that happens.  This can only happen when the
 * TPM is unowned, so it is OK to clear it (and we really have no choice).
 * This is not expected to happen frequently, but it could happen.
 */
static uint32_t SafeWrite(uint32_t index, uint8_t* data, uint32_t length) {
  uint32_t result = TlclWrite(index, data, length);
  if (result == TPM_E_MAXNVWRITES) {
    RETURN_ON_FAILURE(TPMClearAndReenable());
    return TlclWrite(index, data, length);
  } else {
    return result;
  }
}

/* Similarly to SafeWrite(), this ensures we don't fail a DefineSpace because
 * we hit the TPM write limit.  This is even less likely to happen than with
 * writes because we only define spaces once at initialization, but we'd rather
 * be paranoid about this.
 */
static uint32_t SafeDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  uint32_t result = TlclDefineSpace(index, perm, size);
  if (result == TPM_E_MAXNVWRITES) {
    RETURN_ON_FAILURE(TPMClearAndReenable());
    return TlclDefineSpace(index, perm, size);
  } else {
    return result;
  }
}

static uint32_t InitializeKernelVersionsSpaces(void) {
  RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_VERSIONS_NV_INDEX,
                                    TPM_NV_PER_PPWRITE, KERNEL_SPACE_SIZE));
  RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_NV_INDEX, KERNEL_SPACE_INIT_DATA,
                              KERNEL_SPACE_SIZE));
  return TPM_SUCCESS;
}

/* When the return value is TPM_SUCCESS, this function sets *|initialized| to 1
 * if the spaces have been fully initialized, to 0 if not.  Otherwise
 * *|initialized| is not changed.
 */
uint32_t GetSpacesInitialized(int* initialized) {
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
  uint8_t nvlocked = 0;

  VBDEBUG(("TPM: Initializing spaces\n"));

  /* Force the TPM clear, in case it previously had an owner, so that we can
   * redefine the NVRAM spaces. */
  RETURN_ON_FAILURE(TPMClearAndReenable());

  /* The TPM will not enforce the NV authorization restrictions until the 
   * execution of a TPM_NV_DefineSpace with the handle of TPM_NV_INDEX_LOCK.
   * Create that space if it doesn't already exist. */
  RETURN_ON_FAILURE(TlclGetFlags(NULL, NULL, &nvlocked));
  VBDEBUG(("TPM: nvlocked=%d\n", nvlocked));
  if (!nvlocked) {
    VBDEBUG(("TPM: Enabling NV locking\n"));
    RETURN_ON_FAILURE(TlclSetNvLocked());
  }

  RETURN_ON_FAILURE(SafeDefineSpace(FIRMWARE_VERSIONS_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(SafeWrite(FIRMWARE_VERSIONS_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));

  RETURN_ON_FAILURE(InitializeKernelVersionsSpaces());

  /* The space KERNEL_VERSIONS_BACKUP_NV_INDEX is used to protect the kernel
   * versions.  The content of space KERNEL_MUST_USE_BACKUP determines whether
   * only the backup value should be trusted.
   */
  RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));
  RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(SafeWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));
  RETURN_ON_FAILURE(SafeDefineSpace(DEVELOPER_MODE_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  RETURN_ON_FAILURE(SafeWrite(DEVELOPER_MODE_NV_INDEX,
                              (uint8_t*) &zero, sizeof(uint32_t)));

  /* The space TPM_IS_INITIALIZED_NV_INDEX is used to indicate that the TPM
   * initialization has completed.  Without it we cannot be sure that the last
   * space to be created was also initialized (power could have been lost right
   * after its creation).
   */
  RETURN_ON_FAILURE(SafeDefineSpace(TPM_IS_INITIALIZED_NV_INDEX,
                                    firmware_perm, sizeof(uint32_t)));
  return TPM_SUCCESS;
}

static uint32_t SetDistrustKernelSpaceAtNextBoot(uint32_t distrust) {
  uint32_t must_use_backup;
  RETURN_ON_FAILURE(TlclRead(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                             (uint8_t*) &must_use_backup, sizeof(uint32_t)));
  if (must_use_backup != distrust) {
     RETURN_ON_FAILURE(SafeWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                 (uint8_t*) &distrust, sizeof(uint32_t)));
  }
  return TPM_SUCCESS;
}

/* Checks if the kernel version space has been mucked with.  If it has,
 * reconstructs it using the backup value.
 */
uint32_t RecoverKernelSpace(void) {
  uint32_t perms = 0;
  uint8_t buffer[KERNEL_SPACE_SIZE];
  uint32_t backup_combined_versions;
  uint32_t must_use_backup;
  uint32_t zero = 0;

  VBDEBUG(("TPM: RecoverKernelSpace()\n"));

  RETURN_ON_FAILURE(TlclRead(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                             (uint8_t*) &must_use_backup, sizeof(uint32_t)));
  /* must_use_backup is true if the previous boot entered recovery mode. */

  VBDEBUG(("TPM: must_use_backup = %d\n", must_use_backup));

  /* If we can't read the kernel space, or it has the wrong permission, or it
   * doesn't contain the right identifier, we give up.  This will need to be
   * fixed by the recovery kernel.  We have to worry about this because at any
   * time (even with PP turned off) the TPM owner can remove and redefine a
   * PP-protected space (but not write to it).
   */
  RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_NV_INDEX, (uint8_t*) &buffer,
                             KERNEL_SPACE_SIZE));
  RETURN_ON_FAILURE(TlclGetPermissions(KERNEL_VERSIONS_NV_INDEX, &perms));
  if (perms != TPM_NV_PER_PPWRITE ||
      Memcmp(buffer + sizeof(uint32_t), KERNEL_SPACE_UID,
              KERNEL_SPACE_UID_SIZE) != 0) {
    return TPM_E_CORRUPTED_STATE;
  }

  if (must_use_backup) {
    /* We must use the backup space because in the preceding boot cycle the
     * primary space was left unlocked and cannot be trusted.
     */
    RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                               (uint8_t*) &backup_combined_versions,
                               sizeof(uint32_t)));
    RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_NV_INDEX,
                                (uint8_t*) &backup_combined_versions,
                                sizeof(uint32_t)));
    RETURN_ON_FAILURE(SafeWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                (uint8_t*) &zero, 0));
  }
  return TPM_SUCCESS;
}

static uint32_t BackupKernelSpace(void) {
  uint32_t kernel_versions;
  uint32_t backup_versions;
  VBDEBUG(("TPM: BackupKernelSpace()\n"));
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
  RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                              (uint8_t*) &kernel_versions, sizeof(uint32_t)));
  return TPM_SUCCESS;
}

/* Checks for transitions between protected mode to developer mode.  When going
 * into or out of developer mode, clear the TPM.
 */
static uint32_t CheckDeveloperModeTransition(uint32_t current_developer) {
  uint32_t past_developer;
  RETURN_ON_FAILURE(TlclRead(DEVELOPER_MODE_NV_INDEX,
                             (uint8_t*) &past_developer,
                             sizeof(past_developer)));
  if (past_developer != current_developer) {
    RETURN_ON_FAILURE(TPMClearAndReenable());
    RETURN_ON_FAILURE(SafeWrite(DEVELOPER_MODE_NV_INDEX,
                                (uint8_t*) &current_developer,
                                sizeof(current_developer)));
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
uint32_t SetupTPM(int recovery_mode, int developer_mode) {
  uint8_t disable;
  uint8_t deactivated;
  uint32_t result;

  VBDEBUG(("TPM: SetupTPM(r%d, d%d)\n", recovery_mode, developer_mode));

  /* TODO: TlclLibInit() should be able to return failure */
  TlclLibInit();

  RETURN_ON_FAILURE(TlclStartup());
#ifdef USE_CONTINUE_SELF_TEST
  /* TODO: ContinueSelfTest() should be faster than SelfTestFull, but may also
   * not work properly in older TPM firmware.  For now, do the full self test. */
  RETURN_ON_FAILURE(TlclContinueSelfTest());
#else
  RETURN_ON_FAILURE(TlclSelfTestFull());
#endif
  RETURN_ON_FAILURE(TlclAssertPhysicalPresence());
  /* Checks that the TPM is enabled and activated. */
  RETURN_ON_FAILURE(TlclGetFlags(&disable, &deactivated, NULL));
  if (disable || deactivated) {
    VBDEBUG(("TPM: disabled (%d) or deactivated (%d).  Fixing...\n", disable, deactivated));
    RETURN_ON_FAILURE(TlclSetEnable());
    RETURN_ON_FAILURE(TlclSetDeactivated(0));
    VBDEBUG(("TPM: Must reboot to re-enable\n"));
    return TPM_E_MUST_REBOOT;
  }
  result = RecoverKernelSpace();
  if (result != TPM_SUCCESS) {
     /* Check if this is the first time we run and the TPM has not been
      * initialized yet.
      */
    int initialized = 0;
    VBDEBUG(("TPM: RecoverKernelSpace() failed\n"));
    RETURN_ON_FAILURE(GetSpacesInitialized(&initialized));
    if (initialized) {
      VBDEBUG(("TPM: Already initialized, so give up\n"));
      return result;
    } else {
      VBDEBUG(("TPM: Need to initialize spaces.\n"));
      RETURN_ON_FAILURE(InitializeSpaces());
      VBDEBUG(("TPM: Retrying RecoverKernelSpace() now that spaces are initialized.\n"));
      RETURN_ON_FAILURE(RecoverKernelSpace());
    }
  }
  RETURN_ON_FAILURE(BackupKernelSpace());
  RETURN_ON_FAILURE(SetDistrustKernelSpaceAtNextBoot(recovery_mode));
  RETURN_ON_FAILURE(CheckDeveloperModeTransition(developer_mode));

  if (recovery_mode) {
    /* In recovery mode global variables are usable. */
    g_rollback_recovery_mode = 1;
  }
  VBDEBUG(("TPM: SetupTPM() succeeded\n"));
  return TPM_SUCCESS;
}

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))


#ifdef DISABLE_ROLLBACK_TPM

/* Dummy implementations which don't support TPM rollback protection */

uint32_t RollbackFirmwareSetup(int developer_mode) {
#ifndef CHROMEOS_ENVIRONMENT
  /* Initialize the TPM, but ignore return codes.  In ChromeOS
   * environment, don't even talk to the TPM. */
  TlclLibInit();
  TlclStartup();
  TlclSelfTestFull();
#endif
  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareRead(uint16_t* key_version, uint16_t* version) {
  *key_version = *version = 0;
  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareWrite(uint16_t key_version, uint16_t version) {
  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareLock(void) {
  return TPM_SUCCESS;
}

uint32_t RollbackKernelRecovery(int developer_mode) {
#ifndef CHROMEOS_ENVIRONMENT
  /* Initialize the TPM, but ignore return codes.  In ChromeOS
   * environment, don't even talk to the TPM. */
  TlclLibInit();
  TlclStartup();
  TlclSelfTestFull();
#endif
  return TPM_SUCCESS;
}

uint32_t RollbackKernelRead(uint16_t* key_version, uint16_t* version) {
  *key_version = *version = 0;
  return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint16_t key_version, uint16_t version) {
  return TPM_SUCCESS;
}

uint32_t RollbackKernelLock(void) {
  return TPM_SUCCESS;
}

#else

uint32_t RollbackFirmwareSetup(int developer_mode) {
  return SetupTPM(0, developer_mode);
}

uint32_t RollbackFirmwareRead(uint16_t* key_version, uint16_t* version) {
  uint32_t firmware_versions;
  /* Gets firmware versions. */
  RETURN_ON_FAILURE(TlclRead(FIRMWARE_VERSIONS_NV_INDEX,
                             (uint8_t*) &firmware_versions,
                             sizeof(firmware_versions)));
  *key_version = (uint16_t) (firmware_versions >> 16);
  *version = (uint16_t) (firmware_versions & 0xffff);
  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareWrite(uint16_t key_version, uint16_t version) {
  uint32_t combined_version = (key_version << 16) & version;
  return SafeWrite(FIRMWARE_VERSIONS_NV_INDEX,
                   (uint8_t*) &combined_version,
                   sizeof(uint32_t));
}

uint32_t RollbackFirmwareLock(void) {
  return TlclSetGlobalLock();
}

uint32_t RollbackKernelRecovery(int developer_mode) {
  uint32_t result = SetupTPM(1, developer_mode);
  /* In recovery mode we ignore TPM malfunctions or corruptions, and leave the
   * TPM completely unlocked if and only if the dev mode switch is ON.  The
   * recovery kernel will fix the TPM (if needed) and lock it ASAP.  We leave
   * Physical Presence on in either case.
   */
  if (!developer_mode) {
    RETURN_ON_FAILURE(TlclSetGlobalLock());
  }
  /* We still return the result of SetupTPM even though we expect the caller to
   * ignore it.  It's useful in unit testing.
   */
  return result;
}

uint32_t RollbackKernelRead(uint16_t* key_version, uint16_t* version) {
  uint32_t kernel_versions;
  if (g_rollback_recovery_mode) {
    *key_version = 0;
    *version = 0;
  } else {
    /* Reads kernel versions from TPM. */
    RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_NV_INDEX,
                               (uint8_t*) &kernel_versions,
                               sizeof(kernel_versions)));
    *key_version = (uint16_t) (kernel_versions >> 16);
    *version = (uint16_t) (kernel_versions & 0xffff);
  }
  return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint16_t key_version, uint16_t version) {
  if (!g_rollback_recovery_mode) {
    uint32_t combined_version = (key_version << 16) & version;
    return SafeWrite(KERNEL_VERSIONS_NV_INDEX,
                     (uint8_t*) &combined_version,
                     sizeof(uint32_t));
  }
  return TPM_SUCCESS;
}

uint32_t RollbackKernelLock(void) {
  if (!g_rollback_recovery_mode) {
    return TlclLockPhysicalPresence();
  } else {
    return TPM_SUCCESS;
  }
}

#endif // DISABLE_ROLLBACK_TPM
