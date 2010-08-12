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
static uint32_t SafeWrite(uint32_t index, const void* data, uint32_t length) {
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


/* Functions to read and write firmware and kernel spaces. */
static uint32_t ReadSpaceFirmware(RollbackSpaceFirmware* rsf) {
  return TlclRead(FIRMWARE_NV_INDEX, rsf, sizeof(RollbackSpaceFirmware));
}

static uint32_t WriteSpaceFirmware(const RollbackSpaceFirmware* rsf) {
  return SafeWrite(FIRMWARE_NV_INDEX, rsf, sizeof(RollbackSpaceFirmware));
}

static uint32_t ReadSpaceKernel(RollbackSpaceKernel* rsk) {
  return TlclRead(KERNEL_NV_INDEX, rsk, sizeof(RollbackSpaceKernel));
}

static uint32_t WriteSpaceKernel(const RollbackSpaceKernel* rsk) {
  return SafeWrite(KERNEL_NV_INDEX, rsk, sizeof(RollbackSpaceKernel));
}



/* Creates the NVRAM spaces, and sets their initial values as needed. */
static uint32_t InitializeSpaces(RollbackSpaceFirmware* rsf,
                                 RollbackSpaceKernel* rsk) {
  static const RollbackSpaceFirmware rsf_init = {
    ROLLBACK_SPACE_FIRMWARE_VERSION, 0, 0, 0};
  static const RollbackSpaceKernel rsk_init = {
    ROLLBACK_SPACE_KERNEL_VERSION, ROLLBACK_SPACE_KERNEL_UID, 0, 0};
  uint8_t nvlocked = 0;

  VBDEBUG(("TPM: Initializing spaces\n"));

  /* The TPM will not enforce the NV authorization restrictions until the
   * execution of a TPM_NV_DefineSpace with the handle of TPM_NV_INDEX_LOCK.
   * Create that space if it doesn't already exist. */
  RETURN_ON_FAILURE(TlclGetFlags(NULL, NULL, &nvlocked));
  VBDEBUG(("TPM: nvlocked=%d\n", nvlocked));
  if (!nvlocked) {
    VBDEBUG(("TPM: Enabling NV locking\n"));
    RETURN_ON_FAILURE(TlclSetNvLocked());
  }

  /* Initialize the firmware and kernel spaces */
  Memcpy(rsf, &rsf_init, sizeof(RollbackSpaceFirmware));
  /* Initialize the backup copy of the kernel space to the same data
   * as the kernel space */
  Memcpy(&rsf->kernel_backup, &rsk_init, sizeof(RollbackSpaceKernel));
  Memcpy(rsk, &rsk_init, sizeof(RollbackSpaceKernel));

  /* Define and set firmware and kernel spaces */
  RETURN_ON_FAILURE(SafeDefineSpace(FIRMWARE_NV_INDEX,
                                    TPM_NV_PER_GLOBALLOCK | TPM_NV_PER_PPWRITE,
                                    sizeof(RollbackSpaceFirmware)));
  RETURN_ON_FAILURE(WriteSpaceFirmware(rsf));
  RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_NV_INDEX, TPM_NV_PER_PPWRITE,
                                    sizeof(RollbackSpaceKernel)));
  RETURN_ON_FAILURE(WriteSpaceKernel(rsk));
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
uint32_t SetupTPM(int recovery_mode, int developer_mode,
                  RollbackSpaceFirmware* rsf) {

  RollbackSpaceKernel rsk;
  int rsf_dirty = 0;
  uint8_t new_flags = 0;

  uint8_t disable;
  uint8_t deactivated;
  uint32_t result;
  uint32_t perms;

  VBDEBUG(("TPM: SetupTPM(r%d, d%d)\n", recovery_mode, developer_mode));

  /* TODO: TlclLibInit() should be able to return failure */
  TlclLibInit();

  RETURN_ON_FAILURE(TlclStartup());
#ifdef USE_CONTINUE_SELF_TEST
  /* TODO: ContinueSelfTest() should be faster than SelfTestFull, but
   * may also not work properly in older TPM firmware.  For now, do
   * the full self test. */
  RETURN_ON_FAILURE(TlclContinueSelfTest());
#else
  RETURN_ON_FAILURE(TlclSelfTestFull());
#endif
  RETURN_ON_FAILURE(TlclAssertPhysicalPresence());

  /* Check that the TPM is enabled and activated. */
  RETURN_ON_FAILURE(TlclGetFlags(&disable, &deactivated, NULL));
  if (disable || deactivated) {
    VBDEBUG(("TPM: disabled (%d) or deactivated (%d).  Fixing...\n",
             disable, deactivated));
    RETURN_ON_FAILURE(TlclSetEnable());
    RETURN_ON_FAILURE(TlclSetDeactivated(0));
    VBDEBUG(("TPM: Must reboot to re-enable\n"));
    return TPM_E_MUST_REBOOT;
  }

  /* Read the firmware space. */
  result = ReadSpaceFirmware(rsf);
  if (TPM_E_BADINDEX == result) {
    /* This is the first time we've run, and the TPM has not been
     * initialized.  Initialize it. */
    VBDEBUG(("TPM: Not initialized yet.\n"));
    RETURN_ON_FAILURE(InitializeSpaces(rsf, &rsk));
  } else if (TPM_SUCCESS != result) {
    VBDEBUG(("TPM: Firmware space in a bad state; giving up.\n"));
    return TPM_E_CORRUPTED_STATE;
  }
  VBDEBUG(("TPM: Firmware space sv%d f%x v%x\n",
           rsf->struct_version, rsf->flags, rsf->fw_versions));

  /* Read the kernel space and verify its permissions.  If the kernel
   * space has the wrong permission, or it doesn't contain the right
   * identifier, we give up.  This will need to be fixed by the
   * recovery kernel.  We have to worry about this because at any time
   * (even with PP turned off) the TPM owner can remove and redefine a
   * PP-protected space (but not write to it). */
  RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
  RETURN_ON_FAILURE(TlclGetPermissions(KERNEL_NV_INDEX, &perms));
  if (TPM_NV_PER_PPWRITE != perms || ROLLBACK_SPACE_KERNEL_UID != rsk.uid)
    return TPM_E_CORRUPTED_STATE;
  VBDEBUG(("TPM: Kernel space sv%d v%x\n",
           rsk.struct_version, rsk.kernel_versions));

  /* If the kernel space and its backup are different, we need to copy
   * one to the other.  Which one we copy depends on whether the
   * use-backup flag is set. */
  if (0 != Memcmp(&rsk, &rsf->kernel_backup, sizeof(RollbackSpaceKernel))) {
    VBDEBUG(("TPM: kernel space and backup are different\n"));

    if (rsf->flags & FLAG_KERNEL_SPACE_USE_BACKUP) {
      VBDEBUG(("TPM: use backup kernel space\n"));
      Memcpy(&rsk, &rsf->kernel_backup, sizeof(RollbackSpaceKernel));
      RETURN_ON_FAILURE(WriteSpaceKernel(&rsk));
    } else if (rsk.kernel_versions < rsf->kernel_backup.kernel_versions) {
      VBDEBUG(("TPM: kernel versions %x < backup versions %x\n",
               rsk.kernel_versions, rsf->kernel_backup.kernel_versions));
      return TPM_E_INTERNAL_INCONSISTENCY;
    } else {
      VBDEBUG(("TPM: copy kernel space to backup\n"));
      Memcpy(&rsf->kernel_backup, &rsk, sizeof(RollbackSpaceKernel));
      rsf_dirty = 1;
    }
  }

  /* Clear ownership if developer flag has toggled */
  if ((developer_mode ? FLAG_LAST_BOOT_DEVELOPER : 0) !=
      (rsf->flags & FLAG_LAST_BOOT_DEVELOPER)) {
    VBDEBUG(("TPM: Developer flag changed; clearing owner.\n"));
    RETURN_ON_FAILURE(TPMClearAndReenable());
  }

  /* Update flags */
  if (developer_mode)
    new_flags |= FLAG_LAST_BOOT_DEVELOPER;
  if (recovery_mode) {
    new_flags |= FLAG_KERNEL_SPACE_USE_BACKUP;
    g_rollback_recovery_mode = 1;  /* Global variables are usable in
                                    * recovery mode */
  }
  if (rsf->flags != new_flags) {
    rsf->flags = new_flags;
    rsf_dirty = 1;
  }

  /* If firmware space is dirty, flush it back to the TPM */
  if (rsf_dirty) {
    VBDEBUG(("TPM: Updating firmware space.\n"));
    RETURN_ON_FAILURE(WriteSpaceFirmware(rsf));
  }

  VBDEBUG(("TPM: SetupTPM() succeeded\n"));
  return TPM_SUCCESS;
}

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))


#ifdef DISABLE_ROLLBACK_TPM

/* Dummy implementations which don't support TPM rollback protection */

uint32_t RollbackFirmwareSetup(int developer_mode,
                               uint16_t* key_version, uint16_t* version) {
#ifndef CHROMEOS_ENVIRONMENT
  /* Initialize the TPM, but ignore return codes.  In ChromeOS
   * environment, don't even talk to the TPM. */
  TlclLibInit();
  TlclStartup();
  TlclSelfTestFull();
#endif

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

uint32_t RollbackFirmwareSetup(int developer_mode, uint16_t* key_version,
                               uint16_t* version) {
  RollbackSpaceFirmware rsf;

  RETURN_ON_FAILURE(SetupTPM(0, developer_mode, &rsf));
  *key_version = (uint16_t)(rsf.fw_versions >> 16);
  *version = (uint16_t)(rsf.fw_versions & 0xffff);

  VBDEBUG(("TPM: RollbackFirmwareSetup %x %x %x\n", (int)rsf.fw_versions, (int)*key_version, (int)*version));

  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareWrite(uint16_t key_version, uint16_t version) {
  RollbackSpaceFirmware rsf;
  uint32_t new_versions = ((uint32_t)key_version << 16) | version;

  VBDEBUG(("TPM: RollbackFirmwareWrite(%d, %d)\n", (int)key_version, (int)version));

  RETURN_ON_FAILURE(ReadSpaceFirmware(&rsf));
  VBDEBUG(("TPM: RollbackFirmwareWrite %x --> %x\n", (int)rsf.fw_versions, (int)new_versions));
  rsf.fw_versions = new_versions;
  return WriteSpaceFirmware(&rsf);
}

uint32_t RollbackFirmwareLock(void) {
  return TlclSetGlobalLock();
}

uint32_t RollbackKernelRecovery(int developer_mode) {
  RollbackSpaceFirmware rsf;
  uint32_t result = SetupTPM(1, developer_mode, &rsf);
  /* In recovery mode we ignore TPM malfunctions or corruptions, and leave the
   * TPM completely unlocked if and only if the dev mode switch is ON.  The
   * recovery kernel will fix the TPM (if needed) and lock it ASAP.  We leave
   * Physical Presence on in either case. */
  if (!developer_mode) {
    RETURN_ON_FAILURE(TlclSetGlobalLock());
  }
  /* We still return the result of SetupTPM even though we expect the caller to
   * ignore it.  It's useful in unit testing. */
  return result;
}

uint32_t RollbackKernelRead(uint16_t* key_version, uint16_t* version) {
  if (g_rollback_recovery_mode) {
    *key_version = 0;
    *version = 0;
  } else {
    RollbackSpaceKernel rsk;
    RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
    *key_version = (uint16_t)(rsk.kernel_versions >> 16);
    *version = (uint16_t)(rsk.kernel_versions & 0xffff);
    VBDEBUG(("TPM: RollbackKernelRead %x %x %x\n", (int)rsk.kernel_versions,
             (int)*key_version, (int)*version));
  }
  return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint16_t key_version, uint16_t version) {

  VBDEBUG(("TPM: RollbackKernelWrite(%d, %d)\n", (int)key_version,
           (int)version));

  if (g_rollback_recovery_mode) {
    return TPM_SUCCESS;
  } else {
    RollbackSpaceKernel rsk;
    uint32_t new_versions = ((uint32_t)key_version << 16) | version;

    RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
    VBDEBUG(("TPM: RollbackKernelWrite %x --> %x\n", (int)rsk.kernel_versions,
             (int)new_versions));
    rsk.kernel_versions = new_versions;
    return WriteSpaceKernel(&rsk);
  }
}

uint32_t RollbackKernelLock(void) {
  if (g_rollback_recovery_mode) {
    return TPM_SUCCESS;
  } else {
    return TlclLockPhysicalPresence();
  }
}

#endif // DISABLE_ROLLBACK_TPM
