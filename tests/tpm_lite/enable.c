/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Testing: ownership testing code, ForceClear, and nvram write limit.
 */

#include "tlcl.h"

int main(int argc, char** argv) {

  TlclLibInit();
  TlclStartup();
  TlclSelfTestFull();

  TlclAssertPhysicalPresence();
  TlclSetEnable();
  (void) TlclSetDeactivated(0);  // activates the TPM at the next boot

  return 0;
}
