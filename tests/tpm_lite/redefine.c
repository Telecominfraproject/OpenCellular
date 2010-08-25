/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of protection from space redefinition.
 *
 * This test is actually not that interesting because, if I am right, space
 * redefinition is not allowed with PP only.  It requires
 * TPM_TAG_RQU_AUTH1_COMMAND with owner authentication.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tlcl.h"
#include "utility.h"

#define INDEX0 0xcafe
#define INDEX1 0xcaff

int main(int argc, char** argv) {
  uint32_t perm;
  uint32_t result;
  uint32_t x;

  TlclLibInit();
  TlclStartup();
  TlclSelfTestFull();
  TlclAssertPhysicalPresence();

  result = TlclRead(INDEX0, (uint8_t*) &x, sizeof(x));
  if (result == TPM_E_BADINDEX) {
    VBDEBUG(("creating INDEX0\n"));
  } else {
    VBDEBUG(("redefining INDEX0\n"));
  }
  perm = TPM_NV_PER_PPWRITE | TPM_NV_PER_GLOBALLOCK;
  TlclDefineSpace(INDEX0, perm, sizeof(uint32_t));

  result = TlclRead(INDEX0, (uint8_t*) &x, sizeof(x));
  if (result == TPM_E_BADINDEX) {
    VBDEBUG(("redefining INDEX1\n"));
  } else {
    VBDEBUG(("creating INDEX1\n"));
  }
  perm = TPM_NV_PER_PPWRITE;
  TlclDefineSpace(INDEX1, perm, sizeof(uint32_t));

  // Sets the global lock.
  TlclSetGlobalLock();

  // Verifies that index0 cannot be redefined.
  result = TlclDefineSpace(INDEX0, perm, sizeof(uint32_t));
  if (result == TPM_SUCCESS) {
    error("unexpected success redefining INDEX0\n");
    exit(1);
  }

  // Turns off PP.
  TlclLockPhysicalPresence();

  // Verifies that neither index0 nor index1 cannot be redefined.
  result = TlclDefineSpace(INDEX0, perm, sizeof(uint32_t));
  if (result == TPM_SUCCESS) {
    error("unexpected success redefining INDEX0\n");
    exit(1);
  }
  result = TlclDefineSpace(INDEX1, perm, sizeof(uint32_t));
  if (result == TPM_SUCCESS) {
    error("unexpected success redefining INDEX1\n");
    exit(1);
  }

  printf("Test completed successfully\n");
  exit(0);
}
