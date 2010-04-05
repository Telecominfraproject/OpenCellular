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
#include "kernel_image.h"
#include "test_common.h"
#include "utility.h"

/* Choose a kernel size greater than the range of 32-bits unsigned. */
#define BIG_KERNEL_SIZE UINT64_C(0x100000000)

#define FIRMWARE_KEY_BASE_NAME "testkeys/key_rsa2048"
#define KERNEL_KEY_BASE_NAME "testkeys/key_rsa1024"

const char* kFirmwareKeyPublicFile = FIRMWARE_KEY_BASE_NAME ".keyb";
const char* kFirmwareKeyFile = FIRMWARE_KEY_BASE_NAME ".pem";
const char* kKernelKeyPublicFile = KERNEL_KEY_BASE_NAME ".keyb";
const char* kKernelKeyFile = KERNEL_KEY_BASE_NAME ".pem";

int BigKernelTest() {
  int error_code = 0;
  uint64_t len;
  uint8_t* kernel_blob = NULL;
  RSAPublicKey* firmware_key = RSAPublicKeyFromFile(kFirmwareKeyPublicFile);
  uint8_t* firmware_key_blob = BufferFromFile(kFirmwareKeyPublicFile, &len);
  uint8_t* kernel_sign_key_buf = BufferFromFile(kKernelKeyPublicFile, &len);
  debug("Generating Big KernelImage...");
  KernelImage* image =
      GenerateTestKernelImage(3,  /* RSA2048/SHA1 */
                              0,  /* RSA1024/SHA1 */
                              kernel_sign_key_buf,
                              1,  /* Kernel Key Version. */
                              1,  /* Kernel Version */
                              BIG_KERNEL_SIZE,
                              kFirmwareKeyFile,
                              kKernelKeyFile,
                              'K');  /* Kernel Data Fill. */
  if (!firmware_key || !firmware_key_blob || !kernel_sign_key_buf || !image) {
    error_code = 1;
    goto cleanup;
  }
  debug("Done.\n");
  TEST_EQ(VerifyKernelImage(firmware_key, image, 0),
          VERIFY_FIRMWARE_SUCCESS,
          "Big KernelImage Verification");
  kernel_blob = GetKernelBlob(image, &len);
  TEST_EQ(VerifyKernel(firmware_key_blob, kernel_blob, 0),
          VERIFY_FIRMWARE_SUCCESS,
          "Big Kernel Blob Verification");

cleanup:
  Free(kernel_blob);
  KernelImageFree(image);
  Free(kernel_sign_key_buf);
  Free(firmware_key_blob);
  RSAPublicKeyFree(firmware_key);
  return error_code;
}

int main(int argc, char* argv[1])
{
  int error_code = 0;
  error_code = BigKernelTest();
  if (!gTestSuccess)
    error_code = 255;
  return error_code;
}
