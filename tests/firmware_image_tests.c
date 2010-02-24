/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for firmware image library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "file_keys.h"
#include "firmware_image.h"
#include "rsa_utility.h"
#include "sha_utility.h"
#include "utility.h"

int TEST_EQ(int result, int expected_result, char* testname) {
  if (result == expected_result) {
    fprintf(stderr, "%s Test \e[1;32mSUCCEEDED\e[m\n", testname);
    return 1;
  }
  else {
    fprintf(stderr, "%s Test \e[0;31mFAILED\e[m\n", testname);
    return 0;
  }
}

FirmwareImage* GenerateTestFirmwareImage(int algorithm,
                                         uint8_t* sign_key,
                                         int key_version,
                                         int firmware_version,
                                         int firmware_len) {
  FirmwareImage* image = FirmwareImageNew();
  uint8_t* header_checksum;
  DigestContext ctx;

  Memcpy(image->magic, FIRMWARE_MAGIC, FIRMWARE_MAGIC_SIZE);
  image->sign_algorithm = algorithm;
  image->sign_key = (uint8_t*) Malloc(
      RSAProcessedKeySize(image->sign_algorithm));
  Memcpy(image->sign_key, sign_key, RSAProcessedKeySize(image->sign_algorithm));
  image->key_version = key_version;

  /* Update correct header length. */
  image->header_len = (sizeof(image->header_len) +
                       sizeof(image->sign_algorithm) +
                       RSAProcessedKeySize(image->sign_algorithm) +
                       sizeof(image->key_version) +
                       sizeof(image->header_checksum));

  /* Calculate SHA-512 digest on header and populate header_checksum. */
  DigestInit(&ctx, ROOT_SIGNATURE_ALGORITHM);
  DigestUpdate(&ctx, (uint8_t*) &image->header_len,
               sizeof(image->header_len));
  DigestUpdate(&ctx, (uint8_t*) &image->sign_algorithm,
               sizeof(image->sign_algorithm));
  DigestUpdate(&ctx, image->sign_key,
               RSAProcessedKeySize(image->sign_algorithm));
  DigestUpdate(&ctx, (uint8_t*) &image->key_version,
               sizeof(image->key_version));
  header_checksum = DigestFinal(&ctx);
  Memcpy(image->header_checksum, header_checksum, SHA512_DIGEST_SIZE);
  Free(header_checksum);


  /* Populate firmware and preamble with dummy data. */
  image->firmware_version = firmware_version;
  image->firmware_len = firmware_len;
  image->preamble_signature = image->firmware_signature = NULL;
  Memset(image->preamble, 'P', FIRMWARE_PREAMBLE_SIZE);
  image->firmware_data = Malloc(image->firmware_len);
  Memset(image->firmware_data, 'F', image->firmware_len);

  return image;
}

#define DEV_MODE_ENABLED 1
#define DEV_MODE_DISABLED 0

/* Normal Firmware Blob Verification Tests. */
int VerifyFirmwareTest(uint8_t* firmware_blob, uint8_t* root_key_blob) {
  int success = 1;
  if (!TEST_EQ(VerifyFirmware(root_key_blob, firmware_blob, DEV_MODE_ENABLED),
               VERIFY_FIRMWARE_SUCCESS,
               "Normal Firmware Blob Verification (Dev Mode)"))
    success = 0;

  if (!TEST_EQ(VerifyFirmware(root_key_blob, firmware_blob, DEV_MODE_DISABLED),
               VERIFY_FIRMWARE_SUCCESS,
               "Normal Firmware Blob Verification (Trusted)"))
    success = 0;
  return success;
}


/* Normal FirmwareImage Verification Tests. */
int VerifyFirmwareImageTest(FirmwareImage* image,
                            RSAPublicKey* root_key) {
  int success = 1;
  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_ENABLED),
               VERIFY_FIRMWARE_SUCCESS,
               "Normal FirmwareImage Verification (Dev Mode)"))
    success = 0;

  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_DISABLED),
               VERIFY_FIRMWARE_SUCCESS,
               "Normal FirmwareImage Verification (Trusted)"))
    success = 0;
  return success;
}

/* Tampered FirmwareImage Verification Tests. */
int VerifyFirmwareImageTamperTest(FirmwareImage* image,
                                  RSAPublicKey* root_key) {
  int success = 1;
  fprintf(stderr, "[[Tampering with firmware preamble....]]\n");
  image->firmware_version = 0;
  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_ENABLED),
               VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED,
               "FirmwareImage Preamble Tamper Verification (Dev Mode)"))
    success = 0;

  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_DISABLED),
               VERIFY_FIRMWARE_PREAMBLE_SIGNATURE_FAILED,
               "FirmwareImage Preamble Tamper Verification (Trusted)"))
    success = 0;
  image->firmware_version = 1;

  image->firmware_data[0] = 'T';
  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_ENABLED),
               VERIFY_FIRMWARE_SIGNATURE_FAILED,
               "FirmwareImage Tamper Verification (Dev Mode)"))
    success = 0;
  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_DISABLED),
               VERIFY_FIRMWARE_SIGNATURE_FAILED,
               "FirmwareImage Tamper Verification (Trusted)"))
    success = 0;
  image->firmware_data[0] = 'F';


  fprintf(stderr, "[[Tampering with root key signature...]]\n");
  image->key_signature[0] = 0xFF;
  image->key_signature[1] = 0x00;
  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_ENABLED),
               VERIFY_FIRMWARE_SUCCESS,
               "FirmwareImage Root Signature Tamper Verification (Dev Mode)"))
    success = 0;
  if (!TEST_EQ(VerifyFirmwareImage(root_key, image, DEV_MODE_DISABLED),
               VERIFY_FIRMWARE_ROOT_SIGNATURE_FAILED,
               "FirmwareImage Root Signature Tamper Verification (Trusted)"))
    success = 0;

  return success;
}

int main(int argc, char* argv[]) {
  int len;
  uint8_t* sign_key_buf = NULL;
  uint8_t* root_key_blob = NULL;
  uint8_t* firmware_blob = NULL;
  FirmwareImage* image = NULL;
  RSAPublicKey* root_key = NULL;
  int error_code = 1;
  char* tmp_firmwareblob_file = ".tmpFirmwareBlob";

  if(argc != 6) {
    fprintf(stderr, "Usage: %s <algorithm> <root key> <processed root pubkey>"
            " <signing key> <processed signing key>\n", argv[0]);
    return -1;
  }

  /* Read verification keys and create a test image. */
  root_key = RSAPublicKeyFromFile(argv[3]);
  root_key_blob = BufferFromFile(argv[3], &len);
  sign_key_buf = BufferFromFile(argv[5], &len);
  image = GenerateTestFirmwareImage(atoi(argv[1]), sign_key_buf, 1,
                                    1, 1000);

  if (!root_key || !sign_key_buf || !image) {
    error_code = 1;
    goto failure;
  }

  /* Generate and populate signatures. */
  if (!AddKeySignature(image, argv[2])) {
    fprintf(stderr, "Couldn't create key signature.\n");
    error_code = 1;
    goto failure;
  }

  if (!AddFirmwareSignature(image, argv[4], image->sign_algorithm)) {
    fprintf(stderr, "Couldn't create firmware and preamble signature.\n");
    error_code = 1;
    goto failure;
  }


  /* Generate a firmware binary blob from image.
   *
   * TODO(gauravsh): There should be a function to directly generate a binary
   * blob buffer from a FirmwareImage instead of indirectly writing to a file
   * and reading it into a buffer.
   */
  if (!WriteFirmwareImage(tmp_firmwareblob_file, image)) {
    fprintf(stderr, "Couldn't create a temporary firmware blob file.\n");
    error_code = 1;
    goto failure;
  }
  firmware_blob = BufferFromFile(tmp_firmwareblob_file, &len);

  /* Test Firmware blob verify operations. */
  if (!VerifyFirmwareTest(firmware_blob, root_key_blob))
    error_code = 255;

  /* Test FirmwareImage verify operations. */
  if (!VerifyFirmwareImageTest(image, root_key))
    error_code = 255;
  if (!VerifyFirmwareImageTamperTest(image, root_key))
    error_code = 255;

failure:
  Free(firmware_blob);
  Free(image);
  Free(sign_key_buf);
  Free(root_key_blob);
  Free(root_key);

  return error_code;
}
