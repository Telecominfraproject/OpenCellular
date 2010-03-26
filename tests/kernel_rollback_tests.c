/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for checking kernel rollback-prevention logic.
 */

#include <stdio.h>
#include <stdlib.h>

#include "file_keys.h"
#include "kernel_image.h"
#include "rsa_utility.h"
#include "rollback_index.h"
#include "test_common.h"
#include "utility.h"

/* Tests that check for correctness of the VerifyFirmwareDriver_f() logic
 * and rollback prevention. */
void VerifyKernelDriverTest(void) {
  uint64_t len;
  uint8_t* firmware_key_pub = BufferFromFile("testkeys/key_rsa1024.keyb",
                                             &len);
  /* Initialize kernel blobs, including their associated parition
   * table attributed. */
  kernel_entry valid_kernelA =  {
    GenerateRollbackTestKernelBlob(1, 1, 0),
    15,  /* Highest Priority. */
    5,  /* Enough for tests. */
    0  /* Assume we haven't boot off it yet. */
  };
  kernel_entry corrupt_kernelA = {
    GenerateRollbackTestKernelBlob(1, 1, 1),
    15,  /* Highest Priority. */
    5,  /* Enough for tests. */
    0  /* Assume we haven't boot off it yet. */
  };
  kernel_entry valid_kernelB = {
    GenerateRollbackTestKernelBlob(1, 1, 0),
    1,  /* Lower Priority. */
    5,  /* Enough for tests. */
    0  /* Assume we haven't boot off it yet. */
  };
  kernel_entry corrupt_kernelB = {
    GenerateRollbackTestKernelBlob(1, 1, 1),
    1,  /* Lower Priority. */
    5,  /* Enough for tests. */
    0  /* Assume we haven't boot off it yet. */
  };

  /* Initialize rollback index state. */
  g_kernel_key_version = 1;
  g_kernel_version = 1;

  /* Note: This test just checks the rollback prevention mechanism and not
   * the full blown kernel boot logic. Updates to the kernel attributes
   * in the paritition table are not tested.
   */
  fprintf(stderr, "Kernel A boot priority(15) > Kernel B boot priority(1)\n");
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &valid_kernelA, &valid_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_A_CONTINUE,
          "(Valid Kernel A (current version)\n"
          " Valid Kernel B (current version) runs A):");
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &corrupt_kernelA, &valid_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_B_CONTINUE,
          "(Corrupt Kernel A (current version)\n"
          " Valid Kernel B (current version) runs B):");
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &valid_kernelA, &corrupt_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_A_CONTINUE,
          "(Valid Kernel A (current version)\n"
          " Corrupt Kernel B (current version) runs A):");
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &corrupt_kernelA, &corrupt_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_RECOVERY_CONTINUE,
          "(Corrupt Kernel A (current version)\n"
          " Corrupt Kernel B (current version) runs Recovery):");

  fprintf(stderr, "\nSwapping boot priorities...\n"
         "Kernel B boot priority(15) > Kernel A boot priority(1)\n");
  valid_kernelA.boot_priority = corrupt_kernelA.boot_priority = 1;
  valid_kernelB.boot_priority = corrupt_kernelB.boot_priority = 15;
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &valid_kernelA, &valid_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_B_CONTINUE,
          "(Valid Kernel A (current version)\n"
          " Valid Kernel B (current version) runs B):");
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &corrupt_kernelA, &valid_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_B_CONTINUE,
          "(Corrupt Kernel A (current version)\n"
          " Valid Kernel B (current version) runs B):");
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &valid_kernelA, &corrupt_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_A_CONTINUE,
          "(Valid Kernel A (current version)\n"
          " Corrupt Kernel B (current version) runs A):");
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &corrupt_kernelA, &corrupt_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_RECOVERY_CONTINUE,
          "(Corrupt Kernel A (current version)\n"
          " Corrupt Kernel B (current version) runs Recovery):");

  fprintf(stderr, "\nUpdating stored version information. Obsoleting "
          "exiting kernel images.\n");
  g_kernel_key_version = 2;
  g_kernel_version = 2;
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &valid_kernelA, &valid_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_RECOVERY_CONTINUE,
          "(Valid Kernel A (old version)\n"
          " Valid Kernel B (old version) runs Recovery):");

  fprintf(stderr, "\nGenerating updated Kernel A blob with "
          "new version.\n");
  Free(valid_kernelA.kernel_blob);
  valid_kernelA.kernel_blob = GenerateRollbackTestKernelBlob(3, 3, 0);
  TEST_EQ(VerifyKernelDriver_f(firmware_key_pub,
                               &valid_kernelA, &valid_kernelB,
                               DEV_MODE_DISABLED),
          BOOT_KERNEL_A_CONTINUE,
          "(Valid Kernel A (new version)\n"
          " Valid Kernel B (old version) runs A):");

  Free(firmware_key_pub);
  Free(valid_kernelA.kernel_blob);
  Free(valid_kernelB.kernel_blob);
  Free(corrupt_kernelA.kernel_blob);
  Free(corrupt_kernelB.kernel_blob);
}

int main(int argc, char* argv[]) {
  int error_code = 0;
  VerifyKernelDriverTest();
  if (!gTestSuccess)
    error_code = 255;
  return error_code;
}
