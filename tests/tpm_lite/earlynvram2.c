/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of early writing to the NVRAM.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <tss/tcs.h>

#include "tlcl.h"
#include "utility.h"

#define INDEX0 0xcafe

int main(int argc, char** argv) {
  uint32_t perm;
  uint32_t result;
  uint32_t x;

  TlclLibInit();
  TlclStartup();
  TlclContinueSelfTest();

  do {
    result = TlclAssertPhysicalPresence();
    printf("result of AssertPP = %d\n", result);
  } while (result == TPM_E_DOING_SELFTEST ||
           result == TPM_E_NEEDS_SELFTEST);

  if (result != TPM_SUCCESS) {
    error("AssertPP failed with error %d\n", result);
  }

  do {
    result = TlclWrite(INDEX0, (uint8_t*) &x, sizeof(x));
    printf("result of WriteValue = %d\n", result);
  } while (result == TPM_E_DOING_SELFTEST ||
           result == TPM_E_NEEDS_SELFTEST);

  if (result == TPM_E_BADINDEX) {
    VBDEBUG(("creating INDEX0\n"));
    perm = TPM_NV_PER_PPWRITE;
    TlclDefineSpace(INDEX0, perm, sizeof(uint32_t));
  } else if (result != TPM_SUCCESS) {
    error("Write failed with result %d\n", result);
  }

  printf("Test completed successfully\n");
  exit(0);
}
