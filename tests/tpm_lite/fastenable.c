/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Testing: ForceClear and behavior of disable and permanent deactivated flags.
 *
 * ForceClear sets the permanent disable and deactivated flags to their default
 * value of TRUE.  The specs say nothing about STCLEAR flags, so they should be
 * left alone.  This test checks that both flags may be reset without a reboot,
 * resulting in a fully enabled and activated TPM.  (We know that because
 * ForceClear requires that the TPM be enabled and activated to run.)
 */

#include <stdio.h>
#include <tss/tcs.h>

#include "tlcl.h"
#include "utility.h"

#define CHECK(command) do { if ((command) != TPM_SUCCESS) \
      error(#command "\n"); }                             \
  while(0)

int main(int argc, char** argv) {
  uint8_t disable, deactivated;
  int i;

  TlclLibInit();
  CHECK(TlclStartup());
  CHECK(TlclSelfTestFull());

  CHECK(TlclAssertPhysicalPresence());
  printf("PP asserted\n");

  CHECK(TlclGetFlags(&disable, &deactivated, NULL));
  printf("disable is %d, deactivated is %d\n", disable, deactivated);

  for (i = 0; i < 2; i++) {

    CHECK(TlclForceClear());
    printf("tpm is cleared\n");

    CHECK(TlclGetFlags(&disable, &deactivated, NULL));
    printf("disable is %d, deactivated is %d\n", disable, deactivated);

    CHECK(TlclSetEnable());
    printf("disable flag is cleared\n");

    CHECK(TlclGetFlags(&disable, &deactivated, NULL));
    printf("disable is %d, deactivated is %d\n", disable, deactivated);

    CHECK(TlclSetDeactivated(0));
    printf("deactivated flag is cleared\n");

    CHECK(TlclGetFlags(&disable, &deactivated, NULL));
    printf("disable is %d, deactivated is %d\n", disable, deactivated);
  }

  return 0;
}
