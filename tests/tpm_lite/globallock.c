/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of two-stage locking using bGlobalLock and PP.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <tss/tcs.h>

#include "tlcl.h"
#include "utility.h"

#define INDEX0 0xcafe
#define INDEX1 0xcaff

int main(int argc, char** argv) {
  uint32_t zero = 0;
  uint32_t perm;
  uint32_t result;
  uint32_t x;

  TlclLibInit();

  TlclStartup();
  TlclSelftestfull();

  TlclAssertPhysicalPresence();

  result = TlclRead(INDEX0, (uint8_t*) &x, sizeof(x));
  if (result == TPM_E_BADINDEX) {
    perm = TPM_NV_PER_PPWRITE | TPM_NV_PER_GLOBALLOCK;
    TlclDefineSpace(INDEX0, perm, sizeof(uint32_t));
  }
  result = TlclWrite(INDEX0, (uint8_t*) &zero, sizeof(uint32_t));
  assert(result == TPM_SUCCESS);

  result = TlclRead(INDEX1, (uint8_t*) &x, sizeof(x));
  if (result == TPM_E_BADINDEX) {
    perm = TPM_NV_PER_PPWRITE;
    TlclDefineSpace(INDEX1, perm, sizeof(uint32_t));
  }
  result = TlclWrite(INDEX1, (uint8_t*) &zero, sizeof(uint32_t));
  assert(result == TPM_SUCCESS);

  // Sets the global lock.
  TlclSetGlobalLock();

  // Verifies that write to index0 fails.
  x = 1;
  result = TlclWrite(INDEX0, (uint8_t*) &x, sizeof(x));
  if (result != TPM_E_AREA_LOCKED) {
    error("INDEX0 is not locked\n");
    exit(1);
  }

  // Verifies that write to index1 is still possible.
  x = 2;
  result = TlclWrite(INDEX1, (uint8_t*) &x, sizeof(x));
  if (result != TPM_SUCCESS) {
    error("failure to write at INDEX1\n");
    exit(2);
  }

  // Turns off PP.
  TlclLockPhysicalPresence();

  // Verifies that write to index1 fails.
  x = 3;
  result = TlclWrite(INDEX1, (uint8_t*) &x, sizeof(x));
  if (result != TPM_E_BAD_PRESENCE) {
    error("INDEX1 is not locked\n");
    exit(3);
  }

  printf("Test completed successfully\n");
  exit(0);
}
