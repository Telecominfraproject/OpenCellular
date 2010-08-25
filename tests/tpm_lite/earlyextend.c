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

#define INDEX0 0xcafe

int main(int argc, char** argv) {
  uint8_t value_in[20];
  uint8_t value_out[20];
  uint32_t result;

  TlclLibInit();
  TlclStartup();
  TlclContinueSelfTest();

  do {
    result = TlclExtend(1, value_in, value_out);
    printf("result of Extend = %d\n", result);
  } while (result == TPM_E_DOING_SELFTEST ||
           result == TPM_E_NEEDS_SELFTEST);

  printf("Test completed successfully\n");
  exit(0);
}
