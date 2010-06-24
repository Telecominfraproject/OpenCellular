/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Mock rollback index library for testing.
 */

#include "rollback_index.h"
#include "tss_constants.h"

#include <stdio.h>

uint16_t g_firmware_key_version = 0;
uint16_t g_firmware_version = 0;
uint16_t g_kernel_key_version = 0;
uint16_t g_kernel_version = 0;

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

uint32_t SetupTPM(int mode, int developer_flag) {
#ifndef NDEBUG
  debug("Rollback Index Library Mock: TPM initialized.\n");
#endif
  return TPM_SUCCESS;
}

uint32_t GetStoredVersions(int type, uint16_t* key_version, uint16_t* version) {
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
  switch (type) {
    case FIRMWARE_VERSIONS:
      g_firmware_key_version = key_version;
      g_firmware_version = version;
      break;
    case KERNEL_VERSIONS:
      g_kernel_key_version = key_version;
      g_kernel_version = version;
      break;
  }
#ifndef NDEBUG
  debug("Rollback Index Library Mock: Stored Versions written.\n");
#endif
  return TPM_SUCCESS;
}

uint32_t LockFirmwareVersions(void) {
#ifndef NDEBUG
  debug("Rollback Index Library Mock: Firmware Versions Locked.\n");
#endif
  return TPM_SUCCESS;
}
 
uint32_t LockKernelVersionsByLockingPP(void) {
#ifndef NDEBUG
  debug("Rollback Index Library Mock: Kernel Versions Locked.\n");
#endif
  return TPM_SUCCESS;
}
