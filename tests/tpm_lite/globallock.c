/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of two-stage locking using bGlobalLock and PP.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "host_common.h"
#include "tlcl.h"
#include "tlcl_tests.h"

int main(int argc, char** argv) {
  uint32_t zero = 0;
  uint32_t x;

  TlclLibInit();
  TPM_CHECK(TlclStartupIfNeeded());
  TPM_CHECK(TlclSelfTestFull());
  TPM_CHECK(TlclAssertPhysicalPresence());
  TPM_CHECK(TlclRead(INDEX0, (uint8_t*) &x, sizeof(x)));
  TPM_CHECK(TlclWrite(INDEX0, (uint8_t*) &zero, sizeof(uint32_t)));
  TPM_CHECK(TlclRead(INDEX1, (uint8_t*) &x, sizeof(x)));
  TPM_CHECK(TlclWrite(INDEX1, (uint8_t*) &zero, sizeof(uint32_t)));
  TPM_CHECK(TlclSetGlobalLock());

  // Verifies that write to index0 fails.
  x = 1;
  TPM_EXPECT(TlclWrite(INDEX0, (uint8_t*) &x, sizeof(x)), TPM_E_AREA_LOCKED);
  TPM_CHECK(TlclRead(INDEX0, (uint8_t*) &x, sizeof(x)));
  VbAssert(x == 0);

  // Verifies that write to index1 is still possible.
  x = 2;
  TPM_CHECK(TlclWrite(INDEX1, (uint8_t*) &x, sizeof(x)));
  TPM_CHECK(TlclRead(INDEX1, (uint8_t*) &x, sizeof(x)));
  VbAssert(x == 2);

  // Turns off PP.
  TlclLockPhysicalPresence();

  // Verifies that write to index1 fails.
  x = 3;
  TPM_EXPECT(TlclWrite(INDEX1, (uint8_t*) &x, sizeof(x)), TPM_E_BAD_PRESENCE);
  TPM_CHECK(TlclRead(INDEX1, (uint8_t*) &x, sizeof(x)));
  VbAssert(x == 2);
  printf("TEST SUCCEEDED\n");
  exit(0);
}
