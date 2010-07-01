/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests if firmware image library deals with very large firmware. This
 * is a quick and dirty test for detecting integer overflow issues.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "firmware_image.h"
#include "test_common.h"
#include "utility.h"

/* Choose a firmware size greater than the range of 32-bits unsigned. */
#define BIG_FIRMWARE_SIZE UINT64_C(0x100000000)

#define ROOT_KEY_BASE_NAME "testkeys/key_rsa8192"
#define FIRMWARE_KEY_BASE_NAME "testkeys/key_rsa1024"

const char* kRootKeyPublicFile = ROOT_KEY_BASE_NAME ".keyb";
const char* kRootKeyFile = ROOT_KEY_BASE_NAME ".pem";
const char* kFirmwareKeyPublicFile = FIRMWARE_KEY_BASE_NAME ".keyb";
const char* kFirmwareKeyFile = FIRMWARE_KEY_BASE_NAME ".pem";

int BigFirmwareTest(void) {
  int error_code = 0;
  uint64_t len;
  uint8_t* firmware_blob = NULL;
  RSAPublicKey* root_key = RSAPublicKeyFromFile(kRootKeyPublicFile);
  uint8_t* root_key_blob = BufferFromFile(kRootKeyPublicFile, &len);
  uint8_t* firmware_sign_key_buf= BufferFromFile(kFirmwareKeyPublicFile, &len);
  VBDEBUG(("Generating Big FirmwareImage..."));
  FirmwareImage* image =
      GenerateTestFirmwareImage(0, /* RSA1024/SHA1 */
                                firmware_sign_key_buf,
                                1,  /* Firmware Key Version. */
                                1,  /* Firmware Version */
                                BIG_FIRMWARE_SIZE,
                                kRootKeyFile,
                                kFirmwareKeyFile,
                                'F');  /* Firmware data fill. */
  if (!root_key || !root_key_blob || !firmware_sign_key_buf || !image) {
    error_code = 1;
    goto cleanup;
  }
  VBDEBUG(("Done.\n"));
  TEST_EQ(VerifyFirmwareImage(root_key, image),
          VERIFY_FIRMWARE_SUCCESS,
          "Big FirmwareImage Verification");
  firmware_blob = GetFirmwareBlob(image, &len);
  TEST_EQ(VerifyFirmware(root_key_blob, image->firmware_data, firmware_blob),
          VERIFY_FIRMWARE_SUCCESS,
          "Big Firmware Blob Verification");

 cleanup:
  Free(firmware_blob);
  FirmwareImageFree(image);
  Free(firmware_sign_key_buf);
  RSAPublicKeyFree(root_key);
  return error_code;
}

int main(int argc, char* argv[1])
{
  int error_code = 0;
  error_code = BigFirmwareTest();
  if (!gTestSuccess)
    error_code = 255;
  return error_code;
}
