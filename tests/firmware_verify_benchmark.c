/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Timing benchmark for verifying a firmware image.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cryptolib.h"
#include "file_keys.h"
#include "firmware_image.h"
#include "test_common.h"
#include "timer_utils.h"
#include "utility.h"

#define FILE_NAME_SIZE 128
#define NUM_OPERATIONS 100  /* Number of verify operations to time. */

#define FIRMWARE_SIZE_SMALL 512000
#define FIRMWARE_SIZE_MEDIUM 1024000
#define FIRMWARE_SIZE_LARGE 4096000
const uint64_t g_firmware_sizes_to_test[] = {
  FIRMWARE_SIZE_SMALL,
  FIRMWARE_SIZE_MEDIUM,
  FIRMWARE_SIZE_LARGE
};
const char* g_firmware_size_labels[] = {
  "small",
  "medium",
  "large"
};
#define NUM_SIZES_TO_TEST (sizeof(g_firmware_sizes_to_test) / \
                           sizeof(g_firmware_sizes_to_test[0]))

int SpeedTestAlgorithm(int algorithm) {
  int i, j, key_size, error_code = 0;
  ClockTimerState ct;
  double msecs;
  uint64_t len;
  uint8_t* firmware_sign_key = NULL;
  uint8_t* root_key_blob = NULL;
  char firmware_sign_key_file[FILE_NAME_SIZE];
  char file_name[FILE_NAME_SIZE];
  char* sha_strings[] = {  /* Maps algorithm->SHA algorithm. */
    "sha1", "sha256", "sha512",  /* RSA-1024 */
    "sha1", "sha256", "sha512",  /* RSA-2048 */
    "sha1", "sha256", "sha512",  /* RSA-4096 */
    "sha1", "sha256", "sha512",  /* RSA-8192 */
  };
  uint8_t* verification_blobs[NUM_SIZES_TO_TEST];
  uint8_t* firmware_blobs[NUM_SIZES_TO_TEST];
  for (i = 0; i < NUM_SIZES_TO_TEST; ++i)
    firmware_blobs[i] = NULL;

  key_size = siglen_map[algorithm] * 8;  /* in bits. */
  snprintf(firmware_sign_key_file, FILE_NAME_SIZE, "testkeys/key_rsa%d.pem",
           key_size);

  snprintf(file_name, FILE_NAME_SIZE, "testkeys/key_rsa%d.keyb", key_size);
  firmware_sign_key = BufferFromFile(file_name, &len);
  if (!firmware_sign_key) {
    VBDEBUG(("Couldn't read pre-processed firmware signing key.\n"));
    error_code = 1;
    goto cleanup;
  }

  /* Generate test images. */
  for (i = 0; i < NUM_SIZES_TO_TEST; ++i) {
    firmware_blobs[i] = (uint8_t*) malloc(g_firmware_sizes_to_test[i]);
    Memset(firmware_blobs[i], 'F', g_firmware_sizes_to_test[i]);
    verification_blobs[i] = GenerateTestVerificationBlob(
        algorithm,
        firmware_sign_key,
        1,  /* firmware key version. */
        1,  /* firmware version. */
        g_firmware_sizes_to_test[i],
        "testkeys/key_rsa8192.pem",
        firmware_sign_key_file);
    if (!firmware_blobs[i]) {
      VBDEBUG(("Couldn't generate test firmware images.\n"));
      error_code = 1;
      goto cleanup;
    }
  }

  /* Get pre-processed key used for verification. */
  root_key_blob = BufferFromFile("testkeys/key_rsa8192.keyb", &len);
  if (!root_key_blob) {
    VBDEBUG(("Couldn't read pre-processed rootkey.\n"));
    error_code = 1;
    goto cleanup;
  }

  /* Now run the timing tests. */
  for (i = 0; i < NUM_SIZES_TO_TEST; ++i) {
    StartTimer(&ct);
    for (j = 0; j < NUM_OPERATIONS; ++j) {
      if (VERIFY_FIRMWARE_SUCCESS !=
          VerifyFirmware(root_key_blob,
                         verification_blobs[i],
                         firmware_blobs[i]))
        VBDEBUG(("Warning: Firmware Verification Failed.\n"));
    }
    StopTimer(&ct);
    msecs = (float) GetDurationMsecs(&ct) / NUM_OPERATIONS;
    fprintf(stderr,
            "# Firmware (%s, Algo = %s):"
            "\t%.02f ms/verification\n",
            g_firmware_size_labels[i],
            algo_strings[algorithm],
            msecs);
    fprintf(stdout, "ms_firmware_%s_rsa%d_%s:%.02f\n",
            g_firmware_size_labels[i],
            key_size,
            sha_strings[algorithm],
            msecs);
  }

 cleanup:
  for (i = 0; i < NUM_SIZES_TO_TEST; i++) {
    free(firmware_blobs[i]);
    free(verification_blobs[i]);
  }
  free(root_key_blob);
  return error_code;
}


int main(int argc, char* argv[]) {
  int i, error_code = 0;
  for (i = 0; i < kNumAlgorithms; ++i) {
    if (0 != (error_code = SpeedTestAlgorithm(i)))
      return error_code;
  }
  return 0;
}
