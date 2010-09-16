/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of space permissions retrieval.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "tlcl.h"
#include "tlcl_tests.h"
#include "utility.h"

int main(int argc, char** argv) {
  uint32_t perm;
  uint32_t perm_pp_gl = TPM_NV_PER_PPWRITE | TPM_NV_PER_GLOBALLOCK;
  uint32_t perm_pp = TPM_NV_PER_PPWRITE;

  TlclLibInit();
  TPM_CHECK(TlclStartupIfNeeded());
  TPM_CHECK(TlclContinueSelfTest());
  TPM_CHECK(TlclAssertPhysicalPresence());

  TPM_CHECK(TlclGetPermissions(INDEX0, &perm));
  assert((perm & perm_pp_gl) == perm_pp_gl);

  TPM_CHECK(TlclGetPermissions(INDEX1, &perm));
  assert((perm & perm_pp) == perm_pp);

  printf("TEST SUCCEEDED\n");
  exit(0);
}
