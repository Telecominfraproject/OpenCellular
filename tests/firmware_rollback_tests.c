/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for checking firmware rollback-prevention logic.
 */

#include <stdio.h>
#include <stdlib.h>

#include "file_keys.h"
#include "firmware_image.h"
#include "rsa_utility.h"
#include "utility.h"
#include "rollback_index.h"
#include "test_common.h"

/* Tests that check for correctness of the VerifyFirmwareDriver_f() logic
 * and rollback prevention. */
void VerifyFirmwareDriverTest(void) {
  uint8_t* valid_firmwareA = NULL;
  uint8_t* valid_firmwareB = NULL;
  uint8_t* corrupt_firmwareA = NULL;
  uint8_t* corrupt_firmwareB = NULL;
  uint64_t len;
  uint8_t* root_key_pub = BufferFromFile("testkeys/key_rsa8192.keyb",
                                         &len);

  /* Initialize rollback index state. */
  g_firmware_key_version = 1;
  g_firmware_version = 1;

  valid_firmwareA = GenerateRollbackTestImage(1, 1, 0);
  valid_firmwareB = GenerateRollbackTestImage(1, 1, 0);
  corrupt_firmwareA = GenerateRollbackTestImage(1, 1, 1);
  corrupt_firmwareB = GenerateRollbackTestImage(1, 1, 1);

  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 valid_firmwareA, valid_firmwareB),
          BOOT_FIRMWARE_A_CONTINUE,
          "Firmware A (Valid with current version), "
          "Firmware B (Valid with current version)");
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 corrupt_firmwareA, valid_firmwareB),
          BOOT_FIRMWARE_B_CONTINUE,
          "Firmware A (Corrupt with current version), "
          "FirmwareB (Valid with current version)");
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 valid_firmwareA, corrupt_firmwareB),
          BOOT_FIRMWARE_A_CONTINUE,
          "Firmware A (Valid with current version), "
          "FirmwareB (Corrupt with current version)");
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub,
                                 corrupt_firmwareA, corrupt_firmwareB),
          BOOT_FIRMWARE_RECOVERY_CONTINUE,
          "Firmware A (Corrupt with current version), "
          "FirmwareB (Corrupt with current version");
  g_firmware_key_version = 2;
  g_firmware_version = 2;
  TEST_EQ(VerifyFirmwareDriver_f(root_key_pub, valid_firmwareA, valid_firmwareB),
          BOOT_FIRMWARE_RECOVERY_CONTINUE,
          "Firmware A (Valid with old version), "
          "Old Firmware B (Valid with old version)");

  Free(root_key_pub);
  Free(valid_firmwareA);
  Free(valid_firmwareB);
  Free(corrupt_firmwareA);
  Free(corrupt_firmwareB);
}

int main(int argc, char* argv[]) {
  int error_code = 0;
  VerifyFirmwareDriverTest();
  if (!gTestSuccess)
    error_code = 255;
  return error_code;
}
