/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Create two spaces for uses in tests.  OK if they already exist.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tlcl.h"
#include "tlcl_tests.h"
#include "utility.h"

int main(int argc, char** argv) {
  uint32_t perm;
  uint32_t result;
  uint32_t x;

  TlclLibInit();

  TPM_CHECK(TlclStartupIfNeeded());
  TPM_CHECK(TlclSelfTestFull());
  TPM_CHECK(TlclAssertPhysicalPresence());
  TPM_CHECK(TlclForceClear());
  TPM_CHECK(TlclSetEnable());
  TPM_CHECK(TlclSetDeactivated(0));

  result = TlclRead(INDEX0, (uint8_t*) &x, sizeof(x));
  if (result == TPM_E_BADINDEX) {
    perm = TPM_NV_PER_PPWRITE | TPM_NV_PER_GLOBALLOCK;
    TPM_CHECK(TlclDefineSpace(INDEX0, perm, sizeof(uint32_t)));
  }

  result = TlclRead(INDEX1, (uint8_t*) &x, sizeof(x));
  if (result == TPM_E_BADINDEX) {
    perm = TPM_NV_PER_PPWRITE;
    TPM_CHECK(TlclDefineSpace(INDEX1, perm, sizeof(uint32_t)));
  }

  printf("TEST SUCCEEDED\n");
  exit(0);
}
