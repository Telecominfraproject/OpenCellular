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

/* Generates a test firmware image for rollback tests with a given
 * [firmware_key_version] and [firmware_version]. If [is_corrupt] is 1,
 * then the image has invalid signatures and will fail verification. */
uint8_t* GenerateRollbackTestImage(int firmware_key_version,
                                   int firmware_version,
                                   int is_corrupt) {
  FirmwareImage* image = NULL;
  uint8_t* firmware_blob = NULL;
  const char* firmare_sign_key_pub_file = "testkeys/key_rsa1024.keyb";
  uint8_t* firmware_sign_key = NULL;
  const char* root_key_file = "testkeys/key_rsa8192.pem";
  const char* firmware_key_file = "testkeys/key_rsa1024.pem";
  uint64_t len;
  firmware_sign_key = BufferFromFile(firmare_sign_key_pub_file,
                                     &len);
  if (!firmware_sign_key)
    return NULL;

  image = FirmwareImageNew();
  if (!image)
    return NULL;

  Memcpy(image->magic, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE);
  image->firmware_sign_algorithm = 0;  /* RSA1024/SHA1 */
  image->firmware_sign_key = (uint8_t*) Malloc(
      RSAProcessedKeySize(image->firmware_sign_algorithm));
  Memcpy(image->firmware_sign_key, firmware_sign_key,
         RSAProcessedKeySize(image->firmware_sign_algorithm));
  image->firmware_key_version = firmware_key_version;
  Free(firmware_sign_key);

  /* Update correct header length. */
  image->header_len = GetFirmwareHeaderLen(image);

  /* Calculate SHA-512 digest on header and populate header_checksum. */
  CalculateFirmwareHeaderChecksum(image, image->header_checksum);

  /* Populate firmware and preamble with dummy data. */
  image->firmware_version = firmware_version;
  image->firmware_len = 1;
  image->preamble_signature = image->firmware_signature = NULL;
  Memset(image->preamble, 'P', FIRMWARE_PREAMBLE_SIZE);
  image->firmware_data = Malloc(image->firmware_len);
  Memset(image->firmware_data, 'F', image->firmware_len);

  /* Generate and populate signatures. */
  if (!AddFirmwareKeySignature(image, root_key_file)) {
    fprintf(stderr, "Couldn't create key signature.\n");
    FirmwareImageFree(image);
    return NULL;
  }

  if (!AddFirmwareSignature(image, firmware_key_file)) {
    fprintf(stderr, "Couldn't create firmware and preamble signature.\n");
    FirmwareImageFree(image);
    return NULL;
  }
  if (is_corrupt) {
    /* Invalidate image. */
    Memset(image->firmware_data, 'X', image->firmware_len);
  }

  firmware_blob = GetFirmwareBlob(image, &len);
  FirmwareImageFree(image);
  return firmware_blob;
}

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
