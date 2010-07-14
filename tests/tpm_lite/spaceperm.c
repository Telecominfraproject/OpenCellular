/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of space permissions retrieval.  The spaces 0xcafe and 0xcaff must have
 * already been defined (by running, for instance, the "redefine" test).
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tss/tcs.h>

#include "tlcl.h"
#include "utility.h"

#define INDEX0 0xcafe
#define INDEX1 0xcaff

int main(int argc, char** argv) {
  uint32_t perm;
  uint32_t perm_pp_gl = TPM_NV_PER_PPWRITE | TPM_NV_PER_GLOBALLOCK;
  uint32_t perm_pp = TPM_NV_PER_PPWRITE;
  uint32_t result;

  TlclLibInit();
  TlclStartup();
  TlclContinueSelfTest();
  TlclAssertPhysicalPresence();

  result = TlclGetPermissions(INDEX0, &perm);
  assert(result == TPM_SUCCESS);
  printf("permissions for INDEX0 = 0x%x\n", perm);
  assert((perm & perm_pp_gl) == perm_pp_gl);

  result = TlclGetPermissions(INDEX1, &perm);
  assert(result == TPM_SUCCESS);
  printf("permissions for INDEX1 = 0x%x\n", perm);
  assert((perm & perm_pp) == perm_pp);

  printf("Test completed successfully\n");
  exit(0);
}
