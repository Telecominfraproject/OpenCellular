/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "firmware_image.h"
#include "test_common.h"
#include "utility.h"

/* Normal Firmware Blob Verification Tests. */
void VerifyFirmwareTest(uint8_t* firmware_blob, uint8_t* root_key_blob) {
  TEST_EQ(VerifyFirmware(root_key_blob, firmware_blob),
          VERIFY_FIRMWARE_SUCCESS,
          "Normal Firmware Blob Verification");
}

/* Normal FirmwareImage Verification Tests. */
void VerifyFirmwareImageTest(FirmwareImage* image,
                             RSAPublicKey* root_key) {
  TEST_EQ(VerifyFirmwareImage(root_key, image),
          VERIFY_FIRMWARE_SUCCESS,
          "Normal FirmwareImage Verification");
}

/* Tampered FirmwareImage Verification Tests. */
void VerifyFirmwareImageTamperTest(FirmwareImage* image,
                                   RSAPublicKey* root_key) {
  image->firmware_version = 0;
  TEST_EQ(VerifyFirmwareImage(root_key, image),
          VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED,
          "FirmwareImage Preamble Tamper Verification");
  image->firmware_version = 1;

  image->firmware_data[0] = 'T';
  TEST_EQ(VerifyFirmwareImage(root_key, image),
          VERIFY_FIRMWARE_SIGNATURE_FAILED,
          "FirmwareImage Data Tamper Verification");
  image->firmware_data[0] = 'F';

  image->firmware_key_signature[0] = 0xFF;
  image->firmware_key_signature[1] = 0x00;
  TEST_EQ(VerifyFirmwareImage(root_key, image),
          VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED,
          "FirmwareImage Root Signature Tamper Verification");
}

int main(int argc, char* argv[]) {
  uint64_t len;
  const char* root_key_file = NULL;
  const char* firmware_key_file = NULL;
  uint8_t* firmware_sign_key_buf = NULL;
  uint8_t* root_key_blob = NULL;
  uint8_t* firmware_blob = NULL;
  uint64_t firmware_blob_len = 0;
  FirmwareImage* image = NULL;
  RSAPublicKey* root_key_pub = NULL;
  int error_code = 0;
  int algorithm;
  if(argc != 6) {
    fprintf(stderr, "Usage: %s <algorithm> <root key> <processed root pubkey>"
            " <signing key> <processed signing key>\n", argv[0]);
    return -1;
  }

  /* Read verification keys and create a test image. */
  algorithm = atoi(argv[1]);
  root_key_pub = RSAPublicKeyFromFile(argv[3]);
  root_key_blob = BufferFromFile(argv[3], &len);
  firmware_sign_key_buf = BufferFromFile(argv[5], &len);
  root_key_file = argv[2];
  firmware_key_file = argv[4];
  image = GenerateTestFirmwareImage(algorithm,
                                    firmware_sign_key_buf,
                                    1,  /* Firmware Key Version. */
                                    1,  /* Firmware Version. */
                                    1000,  /* Firmware length. */
                                    root_key_file,
                                    firmware_key_file,
                                    'F');

  if (!root_key_pub || !firmware_sign_key_buf || !image) {
    error_code = 1;
    goto failure;
  }
  firmware_blob = GetFirmwareBlob(image, &firmware_blob_len);

  /* Test Firmware blob verify operations. */
  VerifyFirmwareTest(firmware_blob, root_key_blob);

  /* Test FirmwareImage verify operations. */
  VerifyFirmwareImageTest(image, root_key_pub);
  VerifyFirmwareImageTamperTest(image, root_key_pub);

  if (!gTestSuccess)
    error_code = 255;

failure:
  Free(firmware_blob);
  FirmwareImageFree(image);
  Free(firmware_sign_key_buf);
  Free(root_key_blob);
  RSAPublicKeyFree(root_key_pub);

  return error_code;
}
