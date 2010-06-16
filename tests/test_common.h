/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */

#ifndef VBOOT_REFERENCE_TEST_COMMON_H_
#define VBOOT_REFERENCE_TEST_COMMON_H_

#include <stdint.h>

#include "kernel_image.h"

extern int gTestSuccess;

/* Return 1 if result is equal to expected_result, else return 0.
 * Also update the global gTestSuccess flag if test fails. */
int TEST_EQ(int result, int expected_result, char* testname);
/* Return 0 if result is equal to not_expected_result, else return 1.
 * Also update the global gTestSuccess flag if test fails. */
int TEST_NEQ(int result, int not_expected_result, char* testname);

/* Test kernel image generation functions. */
KernelImage* GenerateTestKernelImage(int firmware_sign_algorithm,
                                     int kernel_sign_algorithm,
                                     const uint8_t* kernel_sign_key,
                                     int kernel_key_version,
                                     int kernel_version,
                                     uint64_t kernel_len,
                                     const char* firmware_key_file,
                                     const char* kernel_key_file,
                                     uint8_t kernel_data_fill_char);
uint8_t* GenerateTestKernelBlob(int firmware_sign_algorithm,
                                int kernel_sign_algorithm,
                                const uint8_t* kernel_sign_key,
                                int kernel_key_version,
                                int kernel_version,
                                uint64_t kernel_len,
                                const char* firmware_key_file,
                                const char* kernel_key_file);

/* Generates a test kernel iamge for rollback tests with a given
 * [kernel_key_version} and [kernel_version]. If [is_corrupt] is 1,
 * then the image has invalid signatures and will fail verification. */
uint8_t* GenerateRollbackTestKernelBlob(int kernel_key_version,
                                        int kernel_version,
                                        int is_corrupt);
#endif  /* VBOOT_REFERENCE_TEST_COMMON_H_ */
