/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for tpm_bootmode functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "test_common.h"
#include "utility.h"
#include "tpm_bootmode.h"

extern const char* kBootStateSHA1Digests[];

/* Last in_digest passed to TlclExtend() */
static const uint8_t* last_in = NULL;

/* Return value to pass for TlclExtend() */
static uint32_t extend_returns = 0;

/* Mocked TlclExtend() function for testing */
uint32_t TlclExtend(int pcr_num, const uint8_t* in_digest,
                    uint8_t* out_digest) {

  /* Should be using pcr 0 */
  TEST_EQ(pcr_num, 0, "TlclExtend pcr_num");

  last_in = in_digest;

  return extend_returns;
}


/* Test setting TPM boot mode state */
static void BootStateTest(void) {
  int recdev;
  int flags;
  int index;
  char what[128];

  /* Test all permutations of developer and recovery mode */
  for (recdev = 0; recdev < 4; recdev++) {
    /* Exhaustively test all permutations of key block flags currently
     * defined in vboot_struct.h (KEY_BLOCK_FLAG_*) */
    for (flags = 0; flags < 16; flags++) {
      index = recdev * 3;
      if (6 == flags)
        index += 2;
      else if (7 == flags)
        index += 1;

      last_in = NULL;
      TEST_EQ(SetTPMBootModeState(recdev & 2, recdev & 1, flags), 0,
              "SetTPMBootModeState return");
      snprintf(what, sizeof(what), "SetTPMBootModeState %d, 0x%x",
               recdev, flags);
      TEST_PTR_EQ(last_in, kBootStateSHA1Digests[index], what);
    }
  }

  extend_returns = 1;
  TEST_EQ(SetTPMBootModeState(0, 0, 0), 1, "SetTPMBootModeState error");
}


/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[]) {
  int error_code = 0;

  BootStateTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
