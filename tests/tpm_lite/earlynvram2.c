/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of early writing to the NVRAM.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tlcl.h"
#include "tlcl_tests.h"
#include "utility.h"

int main(int argc, char** argv) {
  uint32_t x;

  TlclLibInit();
  TPM_CHECK(TlclStartup());
  TPM_CHECK(TlclContinueSelfTest());
  TPM_CHECK(TlclAssertPhysicalPresence());
  TPM_CHECK(TlclWrite(INDEX0, (uint8_t*) &x, sizeof(x)));
  printf("TEST SUCCEEDED\n");
  return 0;
}
