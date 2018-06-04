/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Test of early use of TPM_Extend.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "tlcl.h"
#include "tlcl_tests.h"

int main(int argc, char** argv) {
  uint8_t value_in[20];
  uint8_t value_out[20];

  TlclLibInit();
  TPM_CHECK(TlclStartup());
  TPM_CHECK(TlclContinueSelfTest());
  TPM_CHECK(TlclExtend(1, value_in, value_out));
  printf("TEST SUCCEEDED\n");
  exit(0);
}
