/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of locking, to see if locks count as writes.  (They should.)
 */

#include <stdio.h>
#include <stdlib.h>

#include "tlcl.h"

#define INDEX0 0xda70


int main(int argc, char** argv) {
  TlclLibInit();

  TlclStartup();
  TlclSelfTestFull();

  TlclAssertPhysicalPresence();

  TlclWriteLock(INDEX0);

  printf("Locked 0x%x\n", INDEX0);
  exit(0);
}
