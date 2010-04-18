/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for checking firmware rollback-prevention logic.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "firmware_image.h"
#include "utility.h"
#include "rollback_index.h"
#include "test_common.h"

const char* kRootKeyPublicFile = "testkeys/key_rsa8192.keyb";
uint8_t kValidFirmwareData[1] = { 'F' };
uint8_t kCorruptFirmwareData[1] = { 'X' };

/* Tests that check for correctness of the VerifyFirmwareDriver_f() logic
 * and rollback prevention. */
void VerifyFirmwareDriverTest(void) {
  uint8_t* verification_blobA = NULL;
  uint8_t* verification_blobB = NULL;
  uint64_t len;
  uint8_t* root_key_pub = BufferFromFile(kRootKeyPublicFile, &len);

  /* Initialize rollback index state. */
  g_firmware_key_version = 1;
  g_firmware_version = 1;

  verification_blobA = GenerateRollbackTestVerificationBlob(1, 1);
  verification_blobB = GenerateRollbackTestVerificationBlob(1, 1);

  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 verification_blobA,
                                 kValidFirmwareData,
                                 verification_blobB,
                                 kValidFirmwareData),
          BOOT_FIRMWARE_A_CONTINUE,
          "Firmware A (Valid with current version), "
          "Firmware B (Valid with current version)");
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 verification_blobA,
                                 kCorruptFirmwareData,
                                 verification_blobB,
                                 kValidFirmwareData),
          BOOT_FIRMWARE_B_CONTINUE,
          "Firmware A (Corrupt with current version), "
          "Firmware B (Valid with current version)");
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 verification_blobA,
                                 kValidFirmwareData,
                                 verification_blobB,
                                 kCorruptFirmwareData),
          BOOT_FIRMWARE_A_CONTINUE,
          "Firmware A (Valid with current version), "
          "Firmware B (Corrupt with current version)");
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 verification_blobA,
                                 kCorruptFirmwareData,
                                 verification_blobB,
                                 kCorruptFirmwareData),
          BOOT_FIRMWARE_RECOVERY_CONTINUE,
          "Firmware A (Corrupt with current version), "
          "Firmware B (Corrupt with current version");
  g_firmware_key_version = 2;
  g_firmware_version = 2;
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 verification_blobA,
                                 kValidFirmwareData,
                                 verification_blobB,
                                 kValidFirmwareData),
          BOOT_FIRMWARE_RECOVERY_CONTINUE,
          "Firmware A (Valid with old version), "
          "Old Firmware B (Valid with old version)");

  Free(root_key_pub);
  Free(verification_blobA);
  Free(verification_blobB);
}

int main(int argc, char* argv[]) {
  int error_code = 0;
  VerifyFirmwareDriverTest();
  if (!gTestSuccess)
    error_code = 255;
  return error_code;
}
