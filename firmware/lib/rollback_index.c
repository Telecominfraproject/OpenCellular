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

/* TPM PCR to use for storing dev mode measurements */
#define DEV_REC_MODE_PCR 0
/* Input digests for PCR extend */
#define DEV_OFF_REC_OFF_SHA1_DIGEST ((uint8_t*) "\x14\x89\xf9\x23\xc4\xdc\xa7" \
                                     "\x29\x17\x8b\x3e\x32\x33\x45\x85\x50" \
                                     "\xd8\xdd\xdf\x29") /* SHA1("\x00\x00") */

#define DEV_OFF_REC_ON_SHA1_DIGEST ((uint8_t*) "\x3f\x29\x54\x64\x53\x67\x8b" \
                                    "\x85\x59\x31\xc1\x74\xa9\x7d\x6c\x08" \
                                    "\x94\xb8\xf5\x46") /* SHA1("\x00\x01") */

#define DEV_ON_REC_OFF_SHA1_DIGEST ((uint8_t*) "\x0e\x35\x6b\xa5\x05\x63\x1f" \
                                    "\xbf\x71\x57\x58\xbe\xd2\x7d\x50\x3f" \
                                    "\x8b\x26\x0e\x3a") /* SHA1("\x01\x00") */

#define DEV_ON_REC_ON_SHA1_DIGEST ((uint8_t*) "\x91\x59\xcb\x8b\xce\xe7\xfc" \
                                    "\xb9\x55\x82\xf1\x40\x96\x0c\xda\xe7" \
                                    "\x27\x88\xd3\x26") /* SHA1("\x01\x01") */

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

#ifndef DISABLE_ROLLBACK_TPM
static uint32_t ReadSpaceKernel(RollbackSpaceKernel* rsk) {
  return TlclRead(KERNEL_NV_INDEX, rsk, sizeof(RollbackSpaceKernel));
}
#endif

static uint32_t WriteSpaceKernel(const RollbackSpaceKernel* rsk) {
  return SafeWrite(KERNEL_NV_INDEX, rsk, sizeof(RollbackSpaceKernel));
}

/* Performs one-time initializations.  Creates the NVRAM spaces, and sets their
 * initial values as needed.  Sets the nvLocked bit and ensures the physical
 * presence command is enabled and locked.
 */
static uint32_t OneTimeInitializeTPM(RollbackSpaceFirmware* rsf,
                                     RollbackSpaceKernel* rsk) {
  static const RollbackSpaceFirmware rsf_init = {
    ROLLBACK_SPACE_FIRMWARE_VERSION, 0, 0, 0};
  static const RollbackSpaceKernel rsk_init = {
    ROLLBACK_SPACE_KERNEL_VERSION, ROLLBACK_SPACE_KERNEL_UID, 0, 0};
  TPM_PERMANENT_FLAGS pflags;
  uint32_t result;

  VBDEBUG(("TPM: One-time initialization\n"));

  result = TlclGetPermanentFlags(&pflags);
  if (result != TPM_SUCCESS)
    return result;

  /* TPM may come from the factory without physical presence finalized.  Fix
   * if necessary. */
  VBDEBUG(("TPM: physicalPresenceLifetimeLock=%d\n",
           pflags.physicalPresenceLifetimeLock));
  if (!pflags.physicalPresenceLifetimeLock) {
    VBDEBUG(("TPM: Finalizing physical presence\n"));
    RETURN_ON_FAILURE(TlclFinalizePhysicalPresence());
  }

  /* The TPM will not enforce the NV authorization restrictions until the
   * execution of a TPM_NV_DefineSpace with the handle of TPM_NV_INDEX_LOCK.
   * Here we create that space if it doesn't already exist. */
  VBDEBUG(("TPM: nvLocked=%d\n", pflags.nvLocked));
  if (!pflags.nvLocked) {
    VBDEBUG(("TPM: Enabling NV locking\n"));
    RETURN_ON_FAILURE(TlclSetNvLocked());
  }

  /* Clear TPM owner, in case the TPM is already owned for some reason. */
  VBDEBUG(("TPM: Clearing owner\n"));
  RETURN_ON_FAILURE(TPMClearAndReenable());

  /* Initializes the firmware and kernel spaces */
  Memcpy(rsf, &rsf_init, sizeof(RollbackSpaceFirmware));
  Memcpy(rsk, &rsk_init, sizeof(RollbackSpaceKernel));

  /* Defines and sets firmware and kernel spaces */
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

  int rsf_dirty = 0;
  uint8_t new_flags = 0;
  uint8_t disable;
  uint8_t deactivated;
  uint32_t result;

  VBDEBUG(("TPM: SetupTPM(r%d, d%d)\n", recovery_mode, developer_mode));

  if (recovery_mode)
    g_rollback_recovery_mode = 1;  /* Global variables are usable in
                                    * recovery mode */

  RETURN_ON_FAILURE(TlclLibInit());

  RETURN_ON_FAILURE(TlclStartup());
  /* Use ContinueSelfTest rather than SelfTestFull().  It enables
   * access to the subset of TPM commands we need in the firmware, and
   * allows the full self test to run in paralle with firmware
   * startup.  By the time we get to the OS, self test will have
   * completed. */
  RETURN_ON_FAILURE(TlclContinueSelfTest());
  result = TlclAssertPhysicalPresence();
  if (result != 0) {
    /* It is possible that the TPM was delivered with the physical presence
     * command disabled.  This tries enabling it, then tries asserting PP
     * again.
     */
    RETURN_ON_FAILURE(TlclPhysicalPresenceCMDEnable());
    RETURN_ON_FAILURE(TlclAssertPhysicalPresence());
  }

  /* Checks that the TPM is enabled and activated. */
  RETURN_ON_FAILURE(TlclGetFlags(&disable, &deactivated, NULL));
  if (disable || deactivated) {
    VBDEBUG(("TPM: disabled (%d) or deactivated (%d).  Fixing...\n",
             disable, deactivated));
    RETURN_ON_FAILURE(TlclSetEnable());
    RETURN_ON_FAILURE(TlclSetDeactivated(0));
    VBDEBUG(("TPM: Must reboot to re-enable\n"));
    return TPM_E_MUST_REBOOT;
  }

  /* Reads the firmware space. */
  result = ReadSpaceFirmware(rsf);
  if (TPM_E_BADINDEX == result) {
    RollbackSpaceKernel rsk;

    /* This is the first time we've run, and the TPM has not been
     * initialized.  This initializes it. */
    VBDEBUG(("TPM: Not initialized yet.\n"));
    RETURN_ON_FAILURE(OneTimeInitializeTPM(rsf, &rsk));
  } else if (TPM_SUCCESS != result) {
    VBDEBUG(("TPM: Firmware space in a bad state; giving up.\n"));
    return TPM_E_CORRUPTED_STATE;
  }
  VBDEBUG(("TPM: Firmware space sv%d f%x v%x\n",
           rsf->struct_version, rsf->flags, rsf->fw_versions));

  /* Clears ownership if developer flag has toggled */
  if ((developer_mode ? FLAG_LAST_BOOT_DEVELOPER : 0) !=
      (rsf->flags & FLAG_LAST_BOOT_DEVELOPER)) {
    VBDEBUG(("TPM: Developer flag changed; clearing owner.\n"));
    RETURN_ON_FAILURE(TPMClearAndReenable());
  }

  /* Updates flags */
  if (developer_mode)
    new_flags |= FLAG_LAST_BOOT_DEVELOPER;
  if (rsf->flags != new_flags) {
    rsf->flags = new_flags;
    rsf_dirty = 1;
  }

  /* If firmware space is dirty, this flushes it back to the TPM */
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

uint32_t RollbackS3Resume(void) {
#ifndef CHROMEOS_ENVIRONMENT
  /* Initialize the TPM, but ignore return codes.  In ChromeOS
   * environment, don't even talk to the TPM. */
  TlclLibInit();
  TlclResume();
#endif
  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareSetup(int developer_mode, uint32_t* version) {
#ifndef CHROMEOS_ENVIRONMENT
  /* Initializes the TPM, but ignores return codes.  In ChromeOS
   * environment, doesn't even talk to the TPM. */
  TlclLibInit();
  TlclStartup();
  TlclContinueSelfTest();
#endif

  *version = 0;
  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareWrite(uint32_t version) {
  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareLock(void) {
  return TPM_SUCCESS;
}

uint32_t RollbackKernelRecovery(int developer_mode) {
#ifndef CHROMEOS_ENVIRONMENT
  /* Initializes the TPM, but ignore return codes.  In ChromeOS
   * environment, doesn't even talk to the TPM. */
  TlclLibInit();
  TlclStartup();
  TlclSelfTestFull();
#endif
  return TPM_SUCCESS;
}

uint32_t RollbackKernelRead(uint32_t* version) {
  *version = 0;
  return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint32_t version) {
  return TPM_SUCCESS;
}

uint32_t RollbackKernelLock(void) {
  return TPM_SUCCESS;
}

#else

uint32_t RollbackS3Resume(void) {
  uint32_t result;
  RETURN_ON_FAILURE(TlclLibInit());
  result = TlclResume();
  if (result == TPM_E_INVALID_POSTINIT) {
    /* We're on a platform where the TPM maintains power in S3, so
       it's already initialized. */
    return TPM_SUCCESS;
  }
  return result;
}


uint32_t RollbackFirmwareSetup(int developer_mode, uint32_t* version) {
  RollbackSpaceFirmware rsf;
  uint8_t out_digest[20];  /* For PCR extend output */

  RETURN_ON_FAILURE(SetupTPM(0, developer_mode, &rsf));
  *version = rsf.fw_versions;
  VBDEBUG(("TPM: RollbackFirmwareSetup %x\n", (int)rsf.fw_versions));
  if (developer_mode)
    RETURN_ON_FAILURE(TlclExtend(DEV_REC_MODE_PCR, DEV_ON_REC_OFF_SHA1_DIGEST,
                                 out_digest));
  else
    RETURN_ON_FAILURE(TlclExtend(DEV_REC_MODE_PCR, DEV_OFF_REC_OFF_SHA1_DIGEST,
                                 out_digest));
  VBDEBUG(("TPM: RollbackFirmwareSetup dev mode PCR out_digest %02x %02x %02x "
           "%02x\n", out_digest, out_digest+1, out_digest+2, out_digest+3));

  return TPM_SUCCESS;
}

uint32_t RollbackFirmwareWrite(uint32_t version) {
  RollbackSpaceFirmware rsf;

  RETURN_ON_FAILURE(ReadSpaceFirmware(&rsf));
  VBDEBUG(("TPM: RollbackFirmwareWrite %x --> %x\n", (int)rsf.fw_versions,
           (int)version));
  rsf.fw_versions = version;
  return WriteSpaceFirmware(&rsf);
}

uint32_t RollbackFirmwareLock(void) {
  return TlclSetGlobalLock();
}

uint32_t RollbackKernelRecovery(int developer_mode) {
  uint32_t rvs, rve;
  RollbackSpaceFirmware rsf;
  uint8_t out_digest[20];  /* For PCR extend output */

  /* In recovery mode we ignore TPM malfunctions or corruptions, and *
   * leave the TPM complelely unlocked; we call neither
   * TlclSetGlobalLock() nor TlclLockPhysicalPresence().  The recovery
   * kernel will fix the TPM (if needed) and lock it ASAP.  We leave
   * Physical Presence on in either case. */
  rvs = SetupTPM(1, developer_mode, &rsf);
  if (developer_mode)
    rve = TlclExtend(DEV_REC_MODE_PCR, DEV_ON_REC_ON_SHA1_DIGEST, out_digest);
  else
    rve = TlclExtend(DEV_REC_MODE_PCR, DEV_OFF_REC_ON_SHA1_DIGEST, out_digest);
  VBDEBUG(("TPM: RollbackKernelRecovery dev mode PCR out_digest %02x %02x %02x "
           "%02x\n", out_digest, out_digest+1, out_digest+2, out_digest+3));
  return (TPM_SUCCESS == rvs) ? rve : rvs;
}

uint32_t RollbackKernelRead(uint32_t* version) {
  if (g_rollback_recovery_mode) {
    *version = 0;
  } else {
    RollbackSpaceKernel rsk;
    uint32_t perms;

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

    *version = rsk.kernel_versions;
    VBDEBUG(("TPM: RollbackKernelRead %x\n", (int)rsk.kernel_versions));
  }
  return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint32_t version) {
  if (g_rollback_recovery_mode) {
    return TPM_SUCCESS;
  } else {
    RollbackSpaceKernel rsk;
    RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
    VBDEBUG(("TPM: RollbackKernelWrite %x --> %x\n", (int)rsk.kernel_versions,
             (int)version));
    rsk.kernel_versions = version;
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
