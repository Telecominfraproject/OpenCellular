/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Testing: ownership testing code and ForceClear.
 */

#include <stdio.h>

#include "tlcl.h"

int main(int argc, char** argv) {
  int owned;

  TlclLibInit();
  TlclStartup();
  TlclSelfTestFull();
  TlclAssertPhysicalPresence();

  owned = TlclIsOwned();
  printf("tpm is %sowned\n", owned?  "" : "NOT ");
  if (owned) {
    TlclForceClear();
    printf("tpm was cleared\n");
  }

  return 0;
}
