/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Testing: ownership testing code, ForceClear, and nvram write limit.
 */

#include <stdio.h>

#include "tlcl.h"

#define CHECK(command) do {                     \
    uint32_t r = (command);                     \
    if (r != 0) {                               \
      printf(#command "returned 0x%x\n", r);    \
    }                                           \
} while(0)

int main(int argc, char** argv) {
  uint8_t disable, deactivated;

  TlclLibInit();
  TlclStartup();
  CHECK(TlclSelfTestFull());

  CHECK(TlclAssertPhysicalPresence());
  CHECK(TlclGetFlags(&disable, &deactivated, NULL));
  printf("disable is %d, deactivated is %d\n", disable, deactivated);
  CHECK(TlclSetEnable());
  CHECK(TlclSetDeactivated(0));
  CHECK(TlclGetFlags(&disable, &deactivated, NULL));
  printf("disable is %d, deactivated is %d\n", disable, deactivated);

  return 0;
}
