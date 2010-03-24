/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "rollback_index.h"

#include <stdint.h>
#include <tss/tcs.h>

#include "tlcl.h"

uint16_t g_firmware_key_version = 0;
uint16_t g_firmware_version = 0;
uint16_t g_kernel_key_version = 0;
uint16_t g_kernel_version = 0;

static void InitializeSpaces(void) {
  uint16_t zero = 0;
  uint32_t perm = TPM_NV_PER_WRITE_STCLEAR | TPM_NV_PER_PPWRITE;

  printf("Initializing spaces\n");
  TlclSetNvLocked();  /* useful only the first time */

  TlclDefineSpace(FIRMWARE_KEY_VERSION_NV_INDEX, perm, sizeof(uint16_t));
  TlclWrite(FIRMWARE_KEY_VERSION_NV_INDEX, (uint8_t*) &zero, sizeof(uint16_t));

  TlclDefineSpace(FIRMWARE_VERSION_NV_INDEX, perm, sizeof(uint16_t));
  TlclWrite(FIRMWARE_VERSION_NV_INDEX, (uint8_t*) &zero, sizeof(uint16_t));

  TlclDefineSpace(KERNEL_KEY_VERSION_NV_INDEX, perm, sizeof(uint16_t));
  TlclWrite(KERNEL_KEY_VERSION_NV_INDEX, (uint8_t*) &zero, sizeof(uint16_t));

  TlclDefineSpace(KERNEL_VERSION_NV_INDEX, perm, sizeof(uint16_t));
  TlclWrite(KERNEL_VERSION_NV_INDEX, (uint8_t*) &zero, sizeof(uint16_t));
}

static void EnterRecovery(void) {
  /* Temporary recovery stub. Currently just initalizes spaces. */
  InitializeSpaces();
}

static int GetTPMRollbackIndices(void) {
  /* We just perform the reads, making sure they succeed. A failure means that
   * the rollback index locations are some how messed up and we must jump to
   * recovery */
  if (TPM_SUCCESS != TlclRead(FIRMWARE_KEY_VERSION_NV_INDEX,
                              (uint8_t*) &g_firmware_key_version,
                              sizeof(g_firmware_key_version)) ||
      TPM_SUCCESS != TlclRead(FIRMWARE_KEY_VERSION_NV_INDEX,
                              (uint8_t*) &g_firmware_key_version,
                              sizeof(g_firmware_key_version)) ||
      TPM_SUCCESS != TlclRead(FIRMWARE_KEY_VERSION_NV_INDEX,
                               (uint8_t*) &g_firmware_key_version,
                               sizeof(g_firmware_key_version))  ||
      TPM_SUCCESS != TlclRead(FIRMWARE_KEY_VERSION_NV_INDEX,
                              (uint8_t*) &g_firmware_key_version,
                              sizeof(g_firmware_key_version)))
    return 0;
  return 1;
}


void SetupTPM(void) {
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
  if (!GetTPMRollbackIndices()) {
    fprintf(stderr, "Ho Ho Ho! We must jump to recovery.");
    EnterRecovery();
  }
}


uint16_t GetStoredVersion(int type) {
  switch (type) {
    case FIRMWARE_KEY_VERSION:
      return g_firmware_key_version;
      break;
    case FIRMWARE_VERSION:
      return g_firmware_version;
      break;
    case KERNEL_KEY_VERSION:
      return g_kernel_key_version;
      break;
    case KERNEL_VERSION:
      return g_kernel_version;
      break;
  }
  return 0;
}

int WriteStoredVersion(int type, uint16_t version) {
  switch (type) {
    case FIRMWARE_KEY_VERSION:
      return (TPM_SUCCESS == TlclWrite(FIRMWARE_KEY_VERSION_NV_INDEX,
                                       (uint8_t*) &version,
                                       sizeof(uint16_t)));
      break;
    case FIRMWARE_VERSION:
      return (TPM_SUCCESS == TlclWrite(FIRMWARE_VERSION_NV_INDEX,
                                       (uint8_t*) &version,
                                       sizeof(uint16_t)));
      break;
    case KERNEL_KEY_VERSION:
      return (TPM_SUCCESS == TlclWrite(KERNEL_KEY_VERSION_NV_INDEX,
                                       (uint8_t*) &version,
                                       sizeof(uint16_t)));
      break;
    case KERNEL_VERSION:
      return (TPM_SUCCESS == TlclWrite(KERNEL_VERSION_NV_INDEX,
                                       (uint8_t*) &version,
                                       sizeof(uint16_t)));
      break;
  }
  return 0;
}

void LockStoredVersion(int type) {
  /* TODO(gauravsh): Add error checking here to make sure TlclWriteLock
   * did not fail. We must jump to recovery in that case.
   */
  switch (type) {
    case FIRMWARE_KEY_VERSION:
      TlclWriteLock(FIRMWARE_KEY_VERSION_NV_INDEX);
      break;
    case FIRMWARE_VERSION:
      TlclWriteLock(FIRMWARE_VERSION_NV_INDEX);
      break;
    case KERNEL_KEY_VERSION:
      TlclWriteLock(KERNEL_KEY_VERSION_NV_INDEX);
      break;
    case KERNEL_VERSION:
      TlclWriteLock(KERNEL_VERSION_NV_INDEX);
      break;
  }
}
