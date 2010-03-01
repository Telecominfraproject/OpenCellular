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
#include "sha_utility.h"
#include "utility.h"

/* ANSI Color coding sequences. */
#define COL_GREEN "\e[1;32m"
#define COL_RED "\e[0;31m"
#define COL_STOP "\e[m"

int TEST_EQ(int result, int expected_result, char* testname) {
  if (result == expected_result) {
    fprintf(stderr, "%s Test " COL_GREEN " PASSED\n" COL_STOP, testname);
    return 1;
  }
  else {
    fprintf(stderr, "%s Test " COL_RED " FAILED\n" COL_STOP, testname);
    return 0;
  }
}

KernelImage* GenerateTestKernelImage(int firmware_sign_algorithm,
                                     int kernel_sign_algorithm,
                                     uint8_t* kernel_sign_key,
                                     int kernel_key_version,
                                     int kernel_version,
                                     int kernel_len) {
  KernelImage* image = KernelImageNew();
  uint8_t* header_checksum;
  DigestContext ctx;

  Memcpy(image->magic, KERNEL_MAGIC, KERNEL_MAGIC_SIZE);
  image->header_version = 1;
  image->firmware_sign_algorithm = firmware_sign_algorithm;
  image->kernel_sign_algorithm = kernel_sign_algorithm;
  image->kernel_key_version = kernel_key_version;
  image->kernel_sign_key = (uint8_t*) Malloc(
      RSAProcessedKeySize(image->kernel_sign_algorithm));
  Memcpy(image->kernel_sign_key, kernel_sign_key,
         RSAProcessedKeySize(image->kernel_sign_algorithm));

  /* Update correct header length. */
  image->header_len = (sizeof(image->header_version) +
                       sizeof(image->header_len) +
                       sizeof(image->firmware_sign_algorithm) +
                       sizeof(image->kernel_sign_algorithm) +
                       RSAProcessedKeySize(image->kernel_sign_algorithm) +
                       sizeof(image->kernel_key_version) +
                       sizeof(image->header_checksum));

  /* Calculate SHA-512 digest on header and populate header_checksum. */
  DigestInit(&ctx, SHA512_DIGEST_ALGORITHM);
  DigestUpdate(&ctx, (uint8_t*) &image->header_version,
               sizeof(image->header_version));
  DigestUpdate(&ctx, (uint8_t*) &image->header_len,
               sizeof(image->header_len));
  DigestUpdate(&ctx, (uint8_t*) &image->firmware_sign_algorithm,
               sizeof(image->firmware_sign_algorithm));
  DigestUpdate(&ctx, (uint8_t*) &image->kernel_sign_algorithm,
               sizeof(image->kernel_sign_algorithm));
  DigestUpdate(&ctx, (uint8_t*) &image->kernel_key_version,
               sizeof(image->kernel_key_version));
  DigestUpdate(&ctx, image->kernel_sign_key,
               RSAProcessedKeySize(image->kernel_sign_algorithm));
  header_checksum = DigestFinal(&ctx);
  Memcpy(image->header_checksum, header_checksum, SHA512_DIGEST_SIZE);
  Free(header_checksum);

  /* Populate kernel options and data with dummy data. */
  image->kernel_version = kernel_version;
  image->options.version[0] = 1;
  image->options.version[1] = 1;
  image->options.kernel_len = kernel_len;
  image->options.kernel_load_addr = 0;
  image->options.kernel_entry_addr = 0;
  image->kernel_key_signature = image->kernel_signature = NULL;
  image->kernel_data = Malloc(kernel_len);
  Memset(image->kernel_data, 'F', kernel_len);

  return image;
}

#define DEV_MODE_ENABLED 1
#define DEV_MODE_DISABLED 0

/* Normal Kernel Blob Verification Tests. */
int VerifyKernelTest(uint8_t* kernel_blob, uint8_t* firmware_key_blob) {
  int success = 1;
  if (!TEST_EQ(VerifyKernel(firmware_key_blob, kernel_blob, DEV_MODE_ENABLED),
               VERIFY_KERNEL_SUCCESS,
               "Normal Kernel Blob Verification (Dev Mode)"))
    success = 0;

  if (!TEST_EQ(VerifyKernel(firmware_key_blob, kernel_blob, DEV_MODE_DISABLED),
               VERIFY_KERNEL_SUCCESS,
               "Normal Kernel Blob Verification (Trusted)"))
    success = 0;
  return success;
}


/* Normal KernelImage Verification Tests. */
int VerifyKernelImageTest(KernelImage* image,
                          RSAPublicKey* firmware_key) {
  int success = 1;
  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
               VERIFY_KERNEL_SUCCESS,
               "Normal KernelImage Verification (Dev Mode)"))
    success = 0;

  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
               VERIFY_KERNEL_SUCCESS,
               "Normal KernelImage Verification (Trusted)"))
    success = 0;
  return success;
}

/* Tampered KernelImage Verification Tests. */
int VerifyKernelImageTamperTest(KernelImage* image,
                                RSAPublicKey* firmware_key) {
  int success = 1;
  fprintf(stderr, "[[Tampering with kernel config....]]\n");
  image->options.kernel_load_addr = 0xFFFF;
  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
               VERIFY_KERNEL_CONFIG_SIGNATURE_FAILED,
               "KernelImage Config Tamper Verification (Dev Mode)"))
    success = 0;
  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
               VERIFY_KERNEL_CONFIG_SIGNATURE_FAILED,
               "KernelImage Config Tamper Verification (Trusted)"))
    success = 0;
  image->options.kernel_load_addr = 0;

  image->kernel_data[0] = 'T';
  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
               VERIFY_KERNEL_SIGNATURE_FAILED,
               "KernelImage Tamper Verification (Dev Mode)"))
    success = 0;
  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
               VERIFY_KERNEL_SIGNATURE_FAILED,
               "KernelImage Tamper Verification (Trusted)"))
    success = 0;
  image->kernel_data[0] = 'F';


  fprintf(stderr, "[[Tampering with kernel key signature...]]\n");
  image->kernel_key_signature[0] = 0xFF;
  image->kernel_key_signature[1] = 0x00;
  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_ENABLED),
               VERIFY_KERNEL_SUCCESS,
               "KernelImage Key Signature Tamper Verification (Dev Mode)"))
    success = 0;
  if (!TEST_EQ(VerifyKernelImage(firmware_key, image, DEV_MODE_DISABLED),
               VERIFY_KERNEL_KEY_SIGNATURE_FAILED,
               "KernelImage Key Signature Tamper Verification (Trusted)"))
    success = 0;

  return success;
}

int main(int argc, char* argv[]) {
  uint32_t len;
  uint8_t* kernel_sign_key_buf = NULL;
  uint8_t* firmware_key_blob = NULL;
  uint8_t* kernel_blob = NULL;
  KernelImage* image = NULL;
  RSAPublicKey* firmware_key = NULL;
  int error_code = 1;
  char* tmp_kernelblob_file = ".tmpKernelBlob";

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
  if (!firmware_key || !kernel_sign_key_buf || !kernel_sign_key_buf) {
    error_code = 1;
    goto failure;
  }

  image = GenerateTestKernelImage(atoi(argv[1]),
                                  atoi(argv[2]),
                                  kernel_sign_key_buf,
                                  1,  /* Kernel Key Version */
                                  1,  /* Kernel Version */
                                  1000);  /* Kernel Size */
  if (!image) {
    error_code = 1;
    goto failure;
  }

  /* Generate and populate signatures. */
  if (!AddKernelKeySignature(image, argv[3])) {
    fprintf(stderr, "Couldn't create key signature.\n");
    error_code = 1;
    goto failure;
  }

  if (!AddKernelSignature(image, argv[5], image->kernel_sign_algorithm)) {
    fprintf(stderr, "Couldn't create firmware and preamble signature.\n");
    error_code = 1;
    goto failure;
  }

  /* Generate a firmware binary blob from image.
   *
   * TODO(gauravsh): Add a function to directly generate a binary
   * blob buffer from a KernelImage instead of indirectly writing to a file
   * and reading it into a buffer.
   */
  if (!WriteKernelImage(tmp_kernelblob_file, image)) {
    fprintf(stderr, "Couldn't create a temporary kernel blob file.\n");
    error_code = 1;
    goto failure;
  }
  kernel_blob = BufferFromFile(tmp_kernelblob_file, &len);

  /* Test Kernel blob verify operations. */
  if (!VerifyKernelTest(kernel_blob, firmware_key_blob))
    error_code = 255;

  /* Test KernelImage verify operations. */
  if (!VerifyKernelImageTest(image, firmware_key))
    error_code = 255;
  if (!VerifyKernelImageTamperTest(image, firmware_key))
    error_code = 255;

failure:
  Free(kernel_blob);
  KernelImageFree(image);
  Free(kernel_sign_key_buf);
  Free(firmware_key_blob);
  Free(firmware_key);

  return error_code;
}
