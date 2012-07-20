/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Exhaustive testing for correctness and integrity of TPM locking code from
 * all interesting initial conditions.
 *
 * This program iterates through a large number of initial states of the TPM at
 * power on, and executes the code related to initialing the TPM and managing
 * the anti-rollback indices.
 *
 * This program must be run on a system with "TPM-agnostic" BIOS: that is, the
 * system must have a TPM (as of this date, the emulator isn't good enough,
 * because it doesn't support bGlobalLock), but the firmware should not issue a
 * TPM_Startup.  In addition, the TPM drivers must be loaded (tpm_tis, tpm, and
 * tpm_bios) but tcsd should NOT be running.  However, tcsd must be installed,
 * as well as the command tpm_takeownership from the TPM tools, and tpm-nvtool
 * from third-party/tpm.
 *
 * This program must be run as root.  It issues multiple reboots, saving and
 * restoring the state from a file.  Typically it works in two phases: on one
 * reboot it sets the TPM to a certain state, and in the next reboot it runs
 * the test.
 *
 * This program may take a long time to complete.
 *
 * A companion upstart file rbtest.conf contains test setup instructions.  Look
 * around for it.
 */

#include "rollback_index.h"
#include "tlcl.h"
#include "tss_constants.h"
#include "utility.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define RETURN_ON_FAILURE(tpm_command) do {             \
    uint32_t result;                                    \
    if ((result = (tpm_command)) != TPM_SUCCESS) {      \
      return result;                                    \
    }                                                   \
  } while (0)

#define WRITE_BUCKET_NV_INDEX 0x1050
#define STATEPATH "/var/spool/rbtest.state"
#define TPM_ANY_FAILURE (-1)

#define TPM_MAX_NV_WRITE_NOOWNER 64
#define MAX_NV_WRITES_AT_BOOT 18         /* see comment below */
#define INITIALIZATION_NV_WRITES 11      /* see below */

const int high_writecount = TPM_MAX_NV_WRITE_NOOWNER;
/* The -1 below is to make sure there is at least one case when we don't hit
 * the write limit.  It is probably unnecessary, but we pay this cost to avoid
 * an off-by-one error.
 */
const int low_writecount = TPM_MAX_NV_WRITE_NOOWNER - MAX_NV_WRITES_AT_BOOT - 1;
const int low_writecount_when_initialized = TPM_MAX_NV_WRITE_NOOWNER
  - MAX_NV_WRITES_AT_BOOT + INITIALIZATION_NV_WRITES;

/*
 * This structure contains all TPM states of interest, and other testing
 * states.  It is saved and restored from a file across all reboots.
 *
 * OWNED/UNOWNED
 * ACTIVATED/DEACTIVATED
 * ENABLED/DISABLED
 * WRITE COUNT
 *
 * The write count tests hitting the write limit with an unowned TPM.  After
 * resetting the TPM we reset the write count to zero, then we perform
 * |writecount| writes to bring the count to the desired number.
 *
 * Low write counts are not interesting, because we know they cannot cause the
 * code to hit the limit during a single boot.  There are a total of N
 * SafeWrite and SafeDefineSpace call sites, where N = MAX_NV_WRITES_AT_BOOT.
 * Every call site can be reached at most once at every boot (there are no
 * loops or multiple nested calls).  So we only need to test N + 1 different
 * initial values of the NVRAM write count (between 64 - (N + 1)) and 64).
 *
 * A number of calls happen at initialization, so when the TPM_IS_INITIALIZED
 * space exists, we only need to start checking at TPM_MAX_NV_WRITE_NOOWNER -
 * MAX_NV_WRITES_AT_BOOT + INITIALIZATION_NV_WRITES.
 *
 * TPM_IS_INITIALIZED space exists/does not exist
 * KERNEL_MUST_USE_BACKUP = 0 or 1
 * KERNEL_VERSIONS exists/does not
 * KERNEL_VERSIONS space has wrong permissions
 * KERNEL_VERSIONS does not contain the replacement-prevention value
 * KERNEL_VERSIONS and KERNEL_VERSIONS_BACKUP are the same/are not
 * DEVELOPER_MODE_NV_INDEX = 0 or 1
 *
 * developer switch on/off
 * recovery switch on/off
 */

typedef struct RBTState {
  /* Internal testing state */
  int advancing;  /* this is 1 if we are setting the TPM to the next initial
                     state, 0 if we are running the test. */

  /* TPM state */
  int writecount;
  int owned;
  int disable;
  int deactivated;
  int TPM_IS_INITIALIZED_exists;
  int KERNEL_MUST_USE_BACKUP;
  int KERNEL_VERSIONS_exists;
  int KERNEL_VERSIONS_wrong_permissions;
  int KERNEL_VERSIONS_wrong_value;
  int KERNEL_VERSIONS_same_as_backup;
  int DEVELOPER_MODE;   /* content of DEVELOPER_MODE space */
  int developer;        /* setting of developer mode switch */
  int recovery;         /* booting in recovery mode */
} RBTState;

RBTState RBTS;

/* Set to 1 if the TPM was cleared in this run, to avoid clearing it again
 * before we set the write count.
 */
int tpm_was_just_cleared = 0;

const char* RBTS_format =
  "advancing=%d, owned=%d, disable=%d, activated=%d, "
  "writecount=%d, TII_exists=%d, KMUB=%d, "
  "KV_exists=%d, KV_wp=%d, KV_wv=%d, KV_sab=%d, DM=%d, dm=%d, rm=%d";

static void Log(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vsyslog(LOG_INFO, format, ap);
  va_end(ap);
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}

static void reboot(void) {
  int status;
  Log("requesting reboot");
  status = system("/sbin/reboot");
  if (status != 0) {
    Log("reboot failed with status %d", status);
    exit(1);
  }
}

static uint32_t SafeWrite(uint32_t index, uint8_t* data, uint32_t length) {
  uint32_t result = TlclWrite(index, data, length);
  if (result == TPM_E_MAXNVWRITES) {
    RETURN_ON_FAILURE(TPMClearAndReenable());
    result = TlclWrite(index, data, length);
    tpm_was_just_cleared = 0;
  }
  return result;
}

static uint32_t SafeDefineSpace(uint32_t index, uint32_t perm, uint32_t size) {
  uint32_t result = TlclDefineSpace(index, perm, size);
  if (result == TPM_E_MAXNVWRITES) {
    RETURN_ON_FAILURE(TPMClearAndReenable());
    result = TlclDefineSpace(index, perm, size);
    tpm_was_just_cleared = 0;
  }
  return result;
}

static void RollbackTest_SaveState(FILE* file) {
  rewind(file);
  fprintf(file, RBTS_format,
          RBTS.advancing,
          RBTS.owned,
          RBTS.disable,
          RBTS.deactivated,
          RBTS.writecount,
          RBTS.TPM_IS_INITIALIZED_exists,
          RBTS.KERNEL_MUST_USE_BACKUP,
          RBTS.KERNEL_VERSIONS_exists,
          RBTS.KERNEL_VERSIONS_wrong_permissions,
          RBTS.KERNEL_VERSIONS_wrong_value,
          RBTS.KERNEL_VERSIONS_same_as_backup,
          RBTS.DEVELOPER_MODE,
          RBTS.developer,
          RBTS.recovery);
}

static void RollbackTest_RestoreState(FILE* file) {
  if (fscanf(file, RBTS_format,
             &RBTS.advancing,
             &RBTS.owned,
             &RBTS.disable,
             &RBTS.deactivated,
             &RBTS.writecount,
             &RBTS.TPM_IS_INITIALIZED_exists,
             &RBTS.KERNEL_MUST_USE_BACKUP,
             &RBTS.KERNEL_VERSIONS_exists,
             &RBTS.KERNEL_VERSIONS_wrong_permissions,
             &RBTS.KERNEL_VERSIONS_wrong_value,
             &RBTS.KERNEL_VERSIONS_same_as_backup,
             &RBTS.DEVELOPER_MODE,
             &RBTS.developer,
             &RBTS.recovery) != sizeof(RBTS)/sizeof(int)) {
    Log("failed to restore state");
    exit(1);
  }
}

static void RollbackTest_LogState(void) {
  Log(RBTS_format,
      RBTS.advancing,
      RBTS.owned,
      RBTS.disable,
      RBTS.deactivated,
      RBTS.writecount,
      RBTS.TPM_IS_INITIALIZED_exists,
      RBTS.KERNEL_MUST_USE_BACKUP,
      RBTS.KERNEL_VERSIONS_exists,
      RBTS.KERNEL_VERSIONS_wrong_permissions,
      RBTS.KERNEL_VERSIONS_wrong_value,
      RBTS.KERNEL_VERSIONS_same_as_backup,
      RBTS.DEVELOPER_MODE,
      RBTS.developer,
      RBTS.recovery);
}

/* Executes a TPM command from the shell.
 */
static void RollbackTest_TPMShellCommand(char* command) {
  int status;
  TlclCloseDevice();
  status = system("/usr/sbin/tcsd");
  if (status != 0) {
    Log("could not start tcsd");
    exit(1);
  }
  status = system("/usr/bin/sleep 0.1");
  status = system(command);
  if (status != 0) {
    Log("command %s returned 0x%x", command, status);
    exit(1);
  }
  status = system("/usr/bin/pkill tcsd");
  if (status != 0) {
    Log("could not kill tcsd, status 0x%x", status);
    exit(1);
  }
  status = system("/usr/bin/sleep 0.1");
  TlclOpenDevice();
}

/* Sets or clears ownership.
 */
static uint32_t RollbackTest_SetOwnership(int ownership) {
  if (ownership) {
    /* Requesting owned state */
    int owned = TlclIsOwned();
    if (!owned) {
      Log("acquiring ownership");
      RollbackTest_TPMShellCommand("/usr/sbin/tpm_takeownership -y -z");
      Log("ownership acquired");
    }
  } else {
    /* Requesting unowned state */
    Log("clearing TPM");
    RETURN_ON_FAILURE(TPMClearAndReenable());
    tpm_was_just_cleared = 1;
  }
  return TPM_SUCCESS;
}

/* Removes a space.  This is a huge pain, because spaces can be removed only
 * when the TPM is owned.
 */
static uint32_t RollbackTest_RemoveSpace(uint32_t index) {
  char command[1024];
  RollbackTest_SetOwnership(1);
  snprintf(command, sizeof(command),
           "/usr/bin/tpm-nvtool --release --index 0x%x --owner_password \"\"",
           index);
  Log("releasing space %x with command: %s", index, command);
  RollbackTest_TPMShellCommand(command);
  Log("space %x released", index);
  return TPM_SUCCESS;
}

/* Checks if the TPM is disabled/deactivated, and optionally enables/activates.
 * Does not disable/deactivate here because it might interfere with other
 * operations.
 */
static uint32_t RollbackTest_PartiallyAdjustFlags(uint8_t* disable,
                                                  uint8_t* deactivated) {
  RETURN_ON_FAILURE(TlclGetFlags(disable, deactivated));

  if (*deactivated && !RBTS.deactivated) {
    /* Needs to enable before we can activate. */
    RETURN_ON_FAILURE(TlclSetEnable());
    *disable = 0;
    /* Needs to reboot after activating. */
    RETURN_ON_FAILURE(TlclSetDeactivated(0));
    reboot();
  }
  /* We disable and deactivate at the end, if needed. */

  if (*disable && !RBTS.disable) {
    RETURN_ON_FAILURE(TlclSetEnable());
  }
  return TPM_SUCCESS;
}

/* Removes or creates the TPM_IS_INITIALIZED space.
 */
static uint32_t RollbackTest_AdjustIsInitialized(void) {
  int initialized;
  RETURN_ON_FAILURE(GetSpacesInitialized(&initialized));
  if (RBTS.TPM_IS_INITIALIZED_exists && !initialized) {
    RETURN_ON_FAILURE(SafeDefineSpace(TPM_IS_INITIALIZED_NV_INDEX,
                                      TPM_NV_PER_PPWRITE, sizeof(uint32_t)));
  }
  if (!RBTS.TPM_IS_INITIALIZED_exists && initialized) {
    RETURN_ON_FAILURE(RollbackTest_RemoveSpace(TPM_IS_INITIALIZED_NV_INDEX));
  }
  return TPM_SUCCESS;
}

/* Sets or clears KERNEL_MUST_USE_BACKUP.
 */
static uint32_t RollbackTest_AdjustMustUseBackup(void) {
  uint32_t must_use_backup;
  RETURN_ON_FAILURE(TlclRead(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                             (uint8_t*) &must_use_backup,
                             sizeof(must_use_backup)));
  if (RBTS.KERNEL_MUST_USE_BACKUP != must_use_backup) {
    RETURN_ON_FAILURE(SafeWrite(KERNEL_MUST_USE_BACKUP_NV_INDEX,
                                (uint8_t*) &must_use_backup,
                                sizeof(must_use_backup)));
  }
  return TPM_SUCCESS;
}

/* Adjusts KERNEL_VERSIONS space.
 */
static uint32_t RollbackTest_AdjustKernelVersions(int* wrong_value) {
  uint8_t kdata[KERNEL_SPACE_SIZE];
  int exists;
  uint32_t result;

  result = TlclRead(KERNEL_VERSIONS_NV_INDEX, kdata, sizeof(kdata));
  if (result != TPM_SUCCESS && result != TPM_E_BADINDEX) {
    return result;
  }
  *wrong_value = Memcmp(kdata + sizeof(uint32_t), KERNEL_SPACE_UID,
                        KERNEL_SPACE_UID_SIZE);     /* for later use */
  exists = result == TPM_SUCCESS;
  if (RBTS.KERNEL_VERSIONS_exists && !exists) {
    RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_VERSIONS_NV_INDEX,
                                      TPM_NV_PER_PPWRITE, KERNEL_SPACE_SIZE));
  }
  if (!RBTS.KERNEL_VERSIONS_exists && exists) {
    RETURN_ON_FAILURE(RollbackTest_RemoveSpace(KERNEL_VERSIONS_NV_INDEX));
  }
  return TPM_SUCCESS;
}

/* Adjusts permissions of KERNEL_VERSIONS space.  Updates |wrong_value| to
 * reflect that currently the space contains the wrong value (i.e. does not
 * contain the GRWL identifier).
 */
static uint32_t RollbackTest_AdjustKernelPermissions(int* wrong_value) {
  uint32_t perms;

  /* Wrong permissions */
  RETURN_ON_FAILURE(TlclGetPermissions(KERNEL_VERSIONS_NV_INDEX, &perms));
  if (RBTS.KERNEL_VERSIONS_wrong_permissions && perms == TPM_NV_PER_PPWRITE) {
    /* Redefines with wrong permissions. */
    RETURN_ON_FAILURE(RollbackTest_RemoveSpace(KERNEL_VERSIONS_NV_INDEX));
    RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_VERSIONS_NV_INDEX,
                                      TPM_NV_PER_PPWRITE |
                                      TPM_NV_PER_GLOBALLOCK,
                                      KERNEL_SPACE_SIZE));
    *wrong_value = 1;
  }
  if (!RBTS.KERNEL_VERSIONS_wrong_permissions &&
      perms != TPM_NV_PER_PPWRITE) {
    /* Redefines with right permissions. */
    RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_VERSIONS_NV_INDEX,
                                      TPM_NV_PER_PPWRITE, 0));
    RETURN_ON_FAILURE(SafeDefineSpace(KERNEL_VERSIONS_NV_INDEX,
                                      TPM_NV_PER_PPWRITE,
                                      KERNEL_SPACE_SIZE));
    *wrong_value = 1;
  }
  return TPM_SUCCESS;
}

static uint32_t RollbackTest_AdjustKernelValue(int wrong_value) {
  if (!RBTS.KERNEL_VERSIONS_wrong_value && wrong_value) {
    RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_NV_INDEX,
                                KERNEL_SPACE_INIT_DATA, KERNEL_SPACE_SIZE));
  }
  if (RBTS.KERNEL_VERSIONS_wrong_value && !wrong_value) {
    RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_NV_INDEX,
                                (uint8_t*) "mickey mouse",
                                KERNEL_SPACE_SIZE));
  }
  return TPM_SUCCESS;
}

/* Adjusts value of KERNEL_VERSIONS_BACKUP space.
 */
static uint32_t RollbackTest_AdjustKernelBackup(void) {
  /* Same as backup */
  uint32_t kv, kvbackup;
  RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_NV_INDEX,
                             (uint8_t*) &kv, sizeof(kv)));
  RETURN_ON_FAILURE(TlclRead(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                             (uint8_t*) &kvbackup, sizeof(kvbackup)));
  if (RBTS.KERNEL_VERSIONS_same_as_backup && kv != kvbackup) {
    kvbackup = kv;
    RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                                (uint8_t*) &kvbackup, sizeof(kvbackup)));
  }
  if (!RBTS.KERNEL_VERSIONS_same_as_backup && kv == kvbackup) {
    kvbackup = kv + 1;
    RETURN_ON_FAILURE(SafeWrite(KERNEL_VERSIONS_BACKUP_NV_INDEX,
                                (uint8_t*) &kvbackup, sizeof(kvbackup)));
  }
  return TPM_SUCCESS;
}

/* Adjust the value in the developer mode transition space.
 */
static uint32_t RollbackTest_AdjustDeveloperMode(void) {
  uint32_t dev;
  /* Developer mode transitions */
  RETURN_ON_FAILURE(TlclRead(DEVELOPER_MODE_NV_INDEX,
                             (uint8_t*) &dev, sizeof(dev)));

  if (RBTS.developer != dev) {
    dev = RBTS.developer;
    RETURN_ON_FAILURE(SafeWrite(DEVELOPER_MODE_NV_INDEX,
                                (uint8_t*) &dev, sizeof(dev)));
  }
  return TPM_SUCCESS;
}

/* Changes the unowned write count.
 */
static uint32_t RollbackTest_AdjustWriteCount(void) {
  int i;
  if (!RBTS.owned) {
    /* Sets the unowned write count, but only if we think that it will make a
     * difference for the test. In other words: we're trying to reduce the
     * number if initial states with some reasoning that we hope is correct.
     */
    if (RBTS.writecount > low_writecount) {
      if (!tpm_was_just_cleared) {
        /* Unknown write count: must clear the TPM to reset to 0 */
        RETURN_ON_FAILURE(TPMClearAndReenable());
      }
      for (i = 0; i < RBTS.writecount; i++) {
        /* Changes the value to ensure that the TPM won't optimize away
         * writes.
         */
        uint8_t b = (uint8_t) i;
        RETURN_ON_FAILURE(SafeWrite(WRITE_BUCKET_NV_INDEX, &b, 1));
      }
    }
  }
  return TPM_SUCCESS;
}

/* Sets the TPM to the right state for the next test run.
 *
 * Functionally correct ordering is tricky.  Optimal ordering is even trickier
 * (no claim to this).  May succeed only partially and require a reboot to
 * continue (if the TPM was deactivated at boot).
 */
static uint32_t RollbackTest_SetTPMState(int initialize) {
  uint8_t disable, deactivated;
  int wrong_value = 0;

  /* Initializes if needed */
  if (initialize) {
    TlclLibInit();
    /* Don't worry if we're already started. */
    (void) TlclStartup();
    RETURN_ON_FAILURE(TlclContinueSelfTest());
    RETURN_ON_FAILURE(TlclAssertPhysicalPresence());
  }

  RETURN_ON_FAILURE(RollbackTest_PartiallyAdjustFlags(&disable, &deactivated));
  RETURN_ON_FAILURE(RollbackTest_AdjustIsInitialized());
  RETURN_ON_FAILURE(RollbackTest_AdjustMustUseBackup());
  RETURN_ON_FAILURE(RollbackTest_AdjustKernelVersions(&wrong_value));

  if (RBTS.KERNEL_VERSIONS_exists) {
    /* Adjusting these states only makes sense when the kernel versions space
     * exists. */
    RETURN_ON_FAILURE(RollbackTest_AdjustKernelPermissions(&wrong_value));
    RETURN_ON_FAILURE(RollbackTest_AdjustKernelValue(wrong_value));
    RETURN_ON_FAILURE(RollbackTest_AdjustKernelBackup());
  }

  RETURN_ON_FAILURE(RollbackTest_AdjustDeveloperMode());
  RETURN_ON_FAILURE(RollbackTest_SetOwnership(RBTS.owned));
  /* Do not remove spaces between SetOwnership and AdjustWriteCount, as that
   * might change the ownership state.  Also do not issue any writes from now
   * on, because AdjustWriteCount tries to avoid unneccessary clears, and after
   * that, any writes will obviously change the write count.
   */
  RETURN_ON_FAILURE(RollbackTest_AdjustWriteCount());

  /* Finally, disables and/or deactivates.  Must deactivate before disabling
   */
  if (!deactivated && RBTS.deactivated) {
    RETURN_ON_FAILURE(TlclSetDeactivated(1));
  }
  /* It's better to do this last, even though most commands we use work with
   * the TPM disabled.
   */
  if (!disable && RBTS.disable) {
    RETURN_ON_FAILURE(TlclClearEnable());
  }
  return TPM_SUCCESS;
}

#define ADVANCE(rbts_field, min, max) do { \
    if (RBTS.rbts_field == max) {          \
      RBTS.rbts_field = min;               \
    } else {                               \
      RBTS.rbts_field++;                   \
      return 0;                            \
    }                                      \
  } while (0)

#define ADVANCEB(field) ADVANCE(field, 0, 1)

static int RollbackTest_AdvanceState(void) {
  /* This is a generalized counter.  It advances an element of the RTBS
   * structure, and when it hits its maximum value, it resets the element and
   * moves on to the next element, similar to the way a decimal counter
   * increases each digit from 0 to 9 and back to 0 with a carry.
   *
   * Tip: put the expensive state changes at the end.
   */
  ADVANCEB(developer);
  ADVANCEB(recovery);
  ADVANCEB(TPM_IS_INITIALIZED_exists);
  ADVANCEB(KERNEL_MUST_USE_BACKUP);
  if (RBTS.owned) {
    ADVANCEB(KERNEL_VERSIONS_exists);
    ADVANCEB(KERNEL_VERSIONS_wrong_permissions);
    ADVANCEB(KERNEL_VERSIONS_wrong_value);
    ADVANCEB(KERNEL_VERSIONS_same_as_backup);
  }
  ADVANCEB(DEVELOPER_MODE);
  /* The writecount is meaningful only when the TPM is not owned. */
  if (!RBTS.owned) {
    ADVANCE(writecount, low_writecount, high_writecount);
    if (RBTS.TPM_IS_INITIALIZED_exists) {
      /* We don't have to go through the full range in this case. */
      if (RBTS.writecount < low_writecount_when_initialized) {
        RBTS.writecount = low_writecount_when_initialized;
      }
    }
  }
  ADVANCEB(deactivated);
  ADVANCEB(disable);
  ADVANCEB(owned);
  if (RBTS.owned == 0) {
    /* overflow */
    return 1;
  }
  return 0;
}

static void RollbackTest_InitializeState(void) {
  FILE* file = fopen(STATEPATH, "w");
  if (file == NULL) {
    fprintf(stderr, "could not open %s for writing\n", STATEPATH);
    exit(1);
  }
  RBTS.writecount = low_writecount;
  RollbackTest_SaveState(file);
}

uint32_t RollbackTest_Test(void) {
  uint16_t key_version, version;

  if (RBTS.recovery) {
    if (RBTS.developer) {
      /* Developer Recovery mode */
      RETURN_ON_FAILURE(RollbackKernelRecovery(1));
    } else {
      /* Normal Recovery mode */
      RETURN_ON_FAILURE(RollbackKernelRecovery(0));
    }
  } else {
    if (RBTS.developer) {
      /* Developer mode */
      key_version = 0;
      version = 0;
      RETURN_ON_FAILURE(RollbackFirmwareSetup(1));
      RETURN_ON_FAILURE(RollbackFirmwareLock());
    } else {
      /* Normal mode */
      key_version = 0;
      version = 0;
      RETURN_ON_FAILURE(RollbackFirmwareSetup(0));
      RETURN_ON_FAILURE(RollbackFirmwareLock());
      RETURN_ON_FAILURE(RollbackKernelRead(&key_version, &version));
      RETURN_ON_FAILURE(RollbackKernelWrite(key_version, version));
      RETURN_ON_FAILURE(RollbackKernelLock());
    }
  }
  return TPM_SUCCESS;
}

/* One-time call to create the WRITE_BUCKET space.
 */
static uint32_t RollbackTest_InitializeTPM(void) {
  TlclLibInit();
  RETURN_ON_FAILURE(TlclStartup());
  RETURN_ON_FAILURE(TlclContinueSelfTest());
  RETURN_ON_FAILURE(TlclAssertPhysicalPresence());
  RETURN_ON_FAILURE(SafeDefineSpace(WRITE_BUCKET_NV_INDEX,
                                    TPM_NV_PER_PPWRITE, 1));
  RETURN_ON_FAILURE(RollbackTest_SetTPMState(0));
  return TPM_SUCCESS;
}

static void RollbackTest_Initialize(void) {
  Log("initializing");
  RollbackTest_InitializeState();
  if (RollbackTest_InitializeTPM() != TPM_SUCCESS) {
    Log("couldn't initialize TPM");
    exit(1);
  }
}

/* Advances the desired TPM state and sets the TPM to the new state.
 */
static void RollbackTest_Advance(FILE* file) {
  uint32_t result;
  Log("advancing state");
  if (RollbackTest_AdvanceState()) {
    Log("done");
    exit(0);
  }
  result = RollbackTest_SetTPMState(1);
  if (result == TPM_SUCCESS) {
    RBTS.advancing = 0;
    RollbackTest_SaveState(file);
    reboot();
  } else {
    Log("SetTPMState failed with 0x%x\n", result);
    exit(1);
  }
}

/* Performs the test for the current TPM state, and verify that the outcome
 * matches the expectations.
 */
static void RollbackTest_RunOneTest(FILE* file) {
  uint32_t result;
  uint32_t expected_result = TPM_SUCCESS;

  if (!RBTS.KERNEL_VERSIONS_exists ||
      RBTS.KERNEL_VERSIONS_wrong_permissions ||
      RBTS.KERNEL_VERSIONS_wrong_value) {
    expected_result = TPM_E_CORRUPTED_STATE;
  }

  if (!RBTS.KERNEL_VERSIONS_exists && !RBTS.TPM_IS_INITIALIZED_exists) {
    /* The space will be recreated */
    expected_result = TPM_SUCCESS;
  }

  if ((!RBTS.TPM_IS_INITIALIZED_exists || !RBTS.KERNEL_VERSIONS_exists)
      && RBTS.owned) {
    /* Cannot create spaces without owner authorization */
    expected_result = TPM_E_OWNER_SET;
  }

  if (RBTS.TPM_IS_INITIALIZED_exists && !RBTS.KERNEL_VERSIONS_exists) {
    expected_result = TPM_ANY_FAILURE;
  }

  result = RollbackTest_Test();

  if (result == expected_result ||
      (result != TPM_SUCCESS && expected_result == TPM_ANY_FAILURE)) {
    Log("test succeeded with 0x%x\n", result);
    RBTS.advancing = 1;
    RollbackTest_SaveState(file);
    reboot();
  } else {
    Log("test failed with 0x%x, expecting 0x%x\n", result, expected_result);
    exit(1);
  }
}

static FILE* RollbackTest_OpenState(void) {
  FILE* file = fopen(STATEPATH, "r+");
  if (file == NULL) {
    Log("%s could not be opened", STATEPATH);
    exit(1);
  }
  return file;
}

/* Sync saved state with TPM state.
 */
static void RollbackTest_Sync(void) {
  FILE *file = RollbackTest_OpenState();
  uint32_t result;
  RollbackTest_RestoreState(file);
  Log("Syncing state");
  result = RollbackTest_SetTPMState(1);
  if (result != TPM_SUCCESS) {
    Log("Sync failed with %x", result);
    exit(1);
  }
}

/* Runs one testing iteration and advances the testing state.
 */
static void RollbackTest_Run(void) {
  FILE* file = RollbackTest_OpenState();
  RollbackTest_RestoreState(file);
  RollbackTest_LogState();
  if (RBTS.advancing) {
    RollbackTest_Advance(file);
  } else {
    RollbackTest_RunOneTest(file);
  }
}

int main(int argc, char** argv) {

  openlog("rbtest", LOG_CONS | LOG_PERROR, LOG_USER);

  if (geteuid() != 0) {
    fprintf(stderr, "rollback-test: must run as root\n");
    exit(1);
  }

  if (argc == 2 && strcmp(argv[1], "initialize") == 0) {
    RollbackTest_Initialize();
  } else if (argc == 2 && strcmp(argv[1], "sync") == 0) {
    RollbackTest_Sync();
  } else if (argc == 1) {
    RollbackTest_Run();
  } else {
    fprintf(stderr, "usage: rollback-test [ initialize ]\n");
    exit(1);
  }
  return 0;
}
