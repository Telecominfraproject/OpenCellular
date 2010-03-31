/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Splicing tests for the firmware image verification library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "firmware_image.h"
#include "test_common.h"
#include "utility.h"

#define ROOT_KEY_BASE_NAME "testkeys/key_rsa8192"
#define FIRMWARE_KEY_BASE_NAME "testkeys/key_rsa1024"

const char* kRootKeyPublicFile = ROOT_KEY_BASE_NAME ".keyb";
const char* kRootKeyFile = ROOT_KEY_BASE_NAME ".pem";
const char* kFirmwareKeyPublicFile = FIRMWARE_KEY_BASE_NAME ".keyb";
const char* kFirmwareKeyFile = FIRMWARE_KEY_BASE_NAME ".pem";

void VerifyFirmwareSplicingTest()
{
  uint64_t len;
  FirmwareImage* image1 = NULL;
  FirmwareImage* image2 = NULL;
  uint8_t* firmware_blob = NULL;
  uint8_t* firmware_sign_key_buf = NULL;
  RSAPublicKey* root_key = RSAPublicKeyFromFile(kRootKeyPublicFile);
  uint8_t* root_key_blob = BufferFromFile(kRootKeyPublicFile, &len);
  firmware_sign_key_buf= BufferFromFile(kFirmwareKeyPublicFile, &len);
  image1 = GenerateTestFirmwareImage(0, /* RSA1024/SHA1 */
                                     firmware_sign_key_buf,
                                     1,  /* Firmware Key Version. */
                                     1,  /* Firmware Version */
                                     1000,
                                     kRootKeyFile,
                                     kFirmwareKeyFile,
                                     'F');  /* Firmware data fill. */
  image2 = GenerateTestFirmwareImage(0, /* RSA1024/SHA1 */
                                     firmware_sign_key_buf,
                                     1,  /* Firmware Key Version. */
                                     2,  /* Firmware Version */
                                     1000,
                                     kRootKeyFile,
                                     kFirmwareKeyFile,
                                     'G');  /* Different Firmware data fill. */
  /* Verify that the originals verify. */
  TEST_EQ(VerifyFirmwareImage(root_key, image1),
          VERIFY_FIRMWARE_SUCCESS,
          "FirmwareImage firmware_data Original");
  TEST_EQ(VerifyFirmwareImage(root_key, image2),
          VERIFY_FIRMWARE_SUCCESS,
          "FirmwareImage firmware_data Original");

  /* Splice firmware_data + firmware signature from [image1]
   * and put it into [image2]. */
  Memcpy(image2->firmware_signature, image1->firmware_signature,
         siglen_map[0]);
  Memcpy(image2->firmware_data, image1->firmware_data,
         image2->firmware_len);

  TEST_EQ(VerifyFirmwareImage(root_key, image2),
          VERIFY_FIRMWARE_SIGNATURE_FAILED,
          "FirmwareImage firmware_data Splicing");
  firmware_blob = GetFirmwareBlob(image2, &len);
  TEST_EQ(VerifyFirmware(root_key_blob, firmware_blob),
          VERIFY_FIRMWARE_SIGNATURE_FAILED,
          "Firmware Blob firmware_data Splicing");
}

int main(int argc, char* argv[])
{
  int error_code = 0;
  VerifyFirmwareSplicingTest();
  if (!gTestSuccess)
    error_code = 255;
  return error_code;
}
