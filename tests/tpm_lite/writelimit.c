/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of recovery when we hit the NVRAM write limit for an unowned TPM.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <tss/tcs.h>

#include "tlcl.h"
#include "utility.h"

#define INDEX0 0xda70
#define TPM_MAX_NV_WRITES_NOOWNER 64

int main(int argc, char** argv) {
  int i;
  uint32_t result;
  uint8_t disable, deactivated;  /* the TPM specs use these exact names */

  TlclLibInit();

  TlclStartup();
  TlclSelftestfull();

  TlclAssertPhysicalPresence();

  result = TlclGetFlags(&disable, &deactivated);
  printf("disable is %d, deactivated is %d\n", disable, deactivated);

  if (disable || deactivated) {
    TlclSetEnable();
    (void) TlclSetDeactivated(0);
    printf("TPM will be active after next reboot\n");
    exit(0);
  }

  for (i = 0; i < TPM_MAX_NV_WRITES_NOOWNER + 2; i++) {
    printf("writing %d\n", i);
    if ((result = TlclWrite(INDEX0, (uint8_t*)&i, sizeof(i))) != TPM_SUCCESS) {
      switch (result) {
      case TPM_E_MAXNVWRITES:
        printf("Max NV writes exceeded - forcing clear\n");
        TlclForceClear();
        printf("Please reboot and run this program again\n");
        exit(0);
      default:
        error("unexpected error code %d (0x%x)\n");
      }
    }
  }

  /* Done for now.
   */
  printf("Test completed successfully\n");
  exit(0);
}
