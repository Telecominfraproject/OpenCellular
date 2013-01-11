/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of protection from space redefinition when an owner is NOT present.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "host_common.h"
#include "tlcl.h"
#include "tlcl_tests.h"

int main(int argc, char** argv) {
  uint32_t perm;
  uint32_t x;

  TlclLibInit();
  TPM_CHECK(TlclStartupIfNeeded());
  TPM_CHECK(TlclSelfTestFull());
  TPM_CHECK(TlclAssertPhysicalPresence());

  VbAssert(!TlclIsOwned());

  /* Ensures spaces exist. */
  TPM_CHECK(TlclRead(INDEX0, (uint8_t*) &x, sizeof(x)));
  TPM_CHECK(TlclRead(INDEX1, (uint8_t*) &x, sizeof(x)));

  /* Redefines spaces a couple of times. */
  perm = TPM_NV_PER_PPWRITE | TPM_NV_PER_GLOBALLOCK;
  TPM_CHECK(TlclDefineSpace(INDEX0, perm, 2 * sizeof(uint32_t)));
  TPM_CHECK(TlclDefineSpace(INDEX0, perm, sizeof(uint32_t)));

  perm = TPM_NV_PER_PPWRITE;
  TPM_CHECK(TlclDefineSpace(INDEX1, perm, 2 * sizeof(uint32_t)));
  TPM_CHECK(TlclDefineSpace(INDEX1, perm, sizeof(uint32_t)));

  // Sets the global lock.
  TlclSetGlobalLock();

  // Verifies that index0 cannot be redefined.
  TPM_EXPECT(TlclDefineSpace(INDEX0, perm, sizeof(uint32_t)),
             TPM_E_AREA_LOCKED);

  // Checks that index1 can.
  TPM_CHECK(TlclDefineSpace(INDEX1, perm, 2 * sizeof(uint32_t)));
  TPM_CHECK(TlclDefineSpace(INDEX1, perm, sizeof(uint32_t)));

  // Turns off PP.
  TlclLockPhysicalPresence();

  // Verifies that neither index0 nor index1 can be redefined.
  TPM_EXPECT(TlclDefineSpace(INDEX0, perm, sizeof(uint32_t)),
             TPM_E_BAD_PRESENCE);
  TPM_EXPECT(TlclDefineSpace(INDEX1, perm, sizeof(uint32_t)),
             TPM_E_BAD_PRESENCE);

  printf("TEST SUCCEEDED\n");
  exit(0);
}
