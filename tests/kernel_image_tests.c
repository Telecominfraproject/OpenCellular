/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for kernel image library.
 */

#include <stdio.h>
#include <stdlib.h>

#include "file_keys.h"
#include "kernel_image.h"
#include "rsa_utility.h"
#include "test_common.h"
#include "utility.h"

/* Normal Kernel Blob Verification Tests. */
void VerifyKernelTest(uint8_t* kernel_blob, uint8_t* firmware_key_blob) {
  TEST_EQ(VerifyKernel(firmware_key_blob, kernel_blob, DEV_MODE_ENABLED),
          VERIFY_KERNEL_SUCCESS,
          "Normal Kernel Blob Verification (Dev Mode)");

  TEST_EQ(VerifyKernel(firmware_key_blob, kernel_blob, DEV_MODE_DISABLED),
          VERIFY_KERNEL_SUCCESS,
          "Normal Kernel Blob Verification (Trusted)");
}


/* Normal KernelImage Verification Tests. */
void VerifyKernelImageTest(KernelImage* image,
                          RSAPublicKey* firmware_key) {
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
          VERIFY_KERNEL_SUCCESS,
          "Normal KernelImage Verification (Dev Mode)");
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
          VERIFY_KERNEL_SUCCESS,
          "Normal KernelImage Verification (Trusted)");
}

/* Tampered KernelImage Verification Tests. */
void VerifyKernelImageTamperTest(KernelImage* image,
                                 RSAPublicKey* firmware_key) {
  image->options.kernel_load_addr = 0xFFFF;
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
          VERIFY_KERNEL_CONFIG_SIGNATURE_FAILED,
          "KernelImage Config Tamper Verification (Dev Mode)");
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
          VERIFY_KERNEL_CONFIG_SIGNATURE_FAILED,
          "KernelImage Config Tamper Verification (Trusted)");
  image->options.kernel_load_addr = 0;

  image->kernel_data[0] = 'T';
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
          VERIFY_KERNEL_SIGNATURE_FAILED,
          "KernelImage Tamper Verification (Dev Mode)");
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
          VERIFY_KERNEL_SIGNATURE_FAILED,
          "KernelImage Tamper Verification (Trusted)");
  image->kernel_data[0] = 'F';

  image->kernel_key_signature[0] = 0xFF;
  image->kernel_key_signature[1] = 0x00;
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
          VERIFY_KERNEL_SUCCESS,
          "KernelImage Key Signature Tamper Verification (Dev Mode)");
  TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
          VERIFY_KERNEL_KEY_SIGNATURE_FAILED,
          "KernelImage Key Signature Tamper Verification (Trusted)");
}

int main(int argc, char* argv[]) {
  uint64_t len;
  const char* firmware_key_file = NULL;
  const char* kernel_key_file = NULL;
  uint8_t* kernel_sign_key_buf = NULL;
  uint8_t* firmware_key_blob = NULL;
  uint8_t* kernel_blob = NULL;
  uint64_t kernel_blob_len = 0;
  KernelImage* image = NULL;
  RSAPublicKey* firmware_key = NULL;
  int error_code = 0;

  if(argc != 7) {
    fprintf(stderr, "Usage: %s <firmware signing algorithm> "  /* argv[1] */
            "<kernel signing algorithm> "  /* argv[2] */
            "<firmware key> "  /* argv[3] */
            "<processed firmware pubkey> "  /* argv[4] */
            "<kernel signing key> "  /* argv[5] */
            "<processed kernel signing key>\n",  /* argv[6] */
            argv[0]);
    return -1;
  }

  /* Read verification keys and create a test image. */
  firmware_key = RSAPublicKeyFromFile(argv[4]);
  firmware_key_blob = BufferFromFile(argv[4], &len);
  kernel_sign_key_buf = BufferFromFile(argv[6], &len);
  firmware_key_file = argv[3];
  kernel_key_file = argv[5];

  if (!firmware_key || !kernel_sign_key_buf || !kernel_sign_key_buf) {
    error_code = 1;
    goto failure;
  }

  image = GenerateTestKernelImage(atoi(argv[1]),
                                  atoi(argv[2]),
                                  kernel_sign_key_buf,
                                  1,  /* Kernel Key Version */
                                  1,  /* Kernel Version */
                                  1000,  /* Kernel Size */
                                  firmware_key_file,
                                  kernel_key_file);
  if (!image) {
    error_code = 1;
    goto failure;
  }

  kernel_blob = GetKernelBlob(image, &kernel_blob_len);

  /* Test Kernel blob verify operations. */
  VerifyKernelTest(kernel_blob, firmware_key_blob);

  /* Test KernelImage verify operations. */
  VerifyKernelImageTest(image, firmware_key);
  VerifyKernelImageTamperTest(image, firmware_key);

  if (!gTestSuccess)
    error_code = 255;

failure:
  Free(kernel_blob);
  KernelImageFree(image);
  Free(kernel_sign_key_buf);
  Free(firmware_key_blob);
  RSAPublicKeyFree(firmware_key);

  return error_code;
}
