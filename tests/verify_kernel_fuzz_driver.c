/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Utility for aiding fuzz testing of kernel image verification code.
 */

#include <stdio.h>

#include "file_keys.h"
#include "kernel_image.h"
#include "utility.h"

int VerifySignedKernel(const char* image_file,
                       const char* firmware_key_file) {
  int error, error_code = 0;
  uint64_t len;
  uint8_t* kernel_blob = BufferFromFile(image_file, &len);
  uint8_t* firmware_key_blob = BufferFromFile(firmware_key_file, &len);

  if (!firmware_key_blob) {
    fprintf(stderr, "Couldn't read pre-processed public firmware key.\n");
    error_code = 1;
  }

  if (!error_code && !kernel_blob) {
    fprintf(stderr, "Couldn't read kernel image or malformed image.\n");
    error_code = 1;
  }

  if (!error_code && (error = VerifyKernel(firmware_key_blob, kernel_blob,
                                           0))) { /* Trusted Mode. */
    fprintf(stderr, "%s\n", VerifyKernelErrorString(error));
    error_code = 1;
  }
  Free(firmware_key_blob);
  Free(kernel_blob);
  if (error_code)
    return 0;
  return 1;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <image_to_verify> <firmware_keyb>\n", argv[0]);
    return -1;
  }
  if (VerifySignedKernel(argv[1], argv[2])) {
    fprintf(stderr, "Verification SUCCESS!\n");
    return 0;
  }
  else {
    fprintf(stderr, "Verification FAILURE!\n");
    return -1;
  }
}
