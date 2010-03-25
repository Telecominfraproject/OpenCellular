/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef VBOOT_REFERENCE_TEST_COMMON_H_
#define VBOOT_REFERENCE_TEST_COMMON_H_

#include <stdint.h>

#include "firmware_image.h"
#include "kernel_image.h"

extern int gTestSuccess;
/* Return 1 if result is equal to expected_result, else return 0.
 * Also update the global gTestSuccess flag if test fails. */
int TEST_EQ(int result, int expected_result, char* testname);

/* Test firmware image generation functions. */
FirmwareImage* GenerateTestFirmwareImage(int algorithm,
                                         const uint8_t* firmware_sign_key,
                                         int firmware_key_version,
                                         int firmware_version,
                                         int firmware_len,
                                         const char* root_key_file,
                                         const char* firmware_key_file);
uint8_t* GenerateTestFirmwareBlob(int algorithm,
                                  const uint8_t* firmware_sign_key,
                                  int firmware_key_version,
                                  int firmware_version,
                                  int firmware_len,
                                  const char* root_key_file,
                                  const char* firmware_key_file);

/* Test kernel image generation functions. */
KernelImage* GenerateTestKernelImage(int firmware_sign_algorithm,
                                     int kernel_sign_algorithm,
                                     const uint8_t* kernel_sign_key,
                                     int kernel_key_version,
                                     int kernel_version,
                                     int kernel_len,
                                     const char* firmware_key_file,
                                     const char* kernel_key_file);
uint8_t* GenerateTestKernelBlob(int firmware_sign_algorithm,
                                int kernel_sign_algorithm,
                                const uint8_t* kernel_sign_key,
                                int kernel_key_version,
                                int kernel_version,
                                int kernel_len,
                                const char* firmware_key_file,
                                const char* kernel_key_file);
/* Generates a test firmware image for rollback tests with a given
 * [firmware_key_version] and [firmware_version]. If [is_corrupt] is 1,
 * then the image has invalid signatures and will fail verification. */
uint8_t* GenerateRollbackTestImage(int firmware_key_version,
                                   int firmware_version,
                                   int is_corrupt);

#endif  /* VBOOT_REFERENCE_TEST_COMMON_H_ */
