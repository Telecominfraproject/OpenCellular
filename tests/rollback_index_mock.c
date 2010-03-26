/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Mock rollback index library for testing.
 */

#include "rollback_index.h"

#include <stdio.h>
#include <stdint.h>

uint16_t g_firmware_key_version = 0;
uint16_t g_firmware_version = 0;
uint16_t g_kernel_key_version = 0;
uint16_t g_kernel_version = 0;

void SetupTPM(void) {
#ifndef NDEBUG
  fprintf(stderr, "Rollback Index Library Mock: TPM initialized.\n");
#endif
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
      g_firmware_key_version = version;
      break;
    case FIRMWARE_VERSION:
      g_firmware_version = version;
      break;
    case KERNEL_KEY_VERSION:
      g_kernel_key_version = version;
      break;
    case KERNEL_VERSION:
      g_kernel_version = version;
      break;
  }
#ifndef NDEBUG
  fprintf(stderr, "Rollback Index Library Mock: Stored Version written.\n");
#endif
  return 1;
}

void LockStoredVersion(int type) {
#ifndef NDEBUG
  fprintf(stderr, "Rollback Index Library Mock: Version Locked.\n");
#endif
}
