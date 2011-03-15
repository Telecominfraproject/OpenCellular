/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Timing test for various TPM operations.  This is mostly a sanity check to
 * make sure the part doesn't have ridicolously bad timing on simple
 * operations.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "tlcl.h"
#include "tlcl_tests.h"
#include "utility.h"

/* Runs [op] and ensures it returns success and doesn't run longer than
 * [time_limit] in milliseconds.
 */
#define TTPM_CHECK(op, time_limit) do {                                 \
    struct timeval before, after;                                       \
    int time;                                                           \
    uint32_t __result;                                                  \
    gettimeofday(&before, NULL);                                        \
    __result = op;                                                      \
    if (__result != TPM_SUCCESS) {                                      \
      printf(#op ": error 0x%x\n", __result);                           \
      errors++;                                                          \
    }                                                                   \
    gettimeofday(&after, NULL);                                         \
    time = (int) ((after.tv_sec - before.tv_sec) * 1000 +               \
                  (after.tv_usec - before.tv_usec) / 1000);             \
    printf(#op ": %d ms\n", time);                                      \
    if (time > time_limit) {                                            \
      printf(#op " exceeded " #time_limit " ms\n");                     \
      time_limit_exceeded = 1;                                          \
    }                                                                   \
  } while (0)

int main(int argc, char** argv) {
  uint32_t x;
  uint8_t in[20], out[20];
  int time_limit_exceeded = 0;
  int errors = 0;

  TlclLibInit();
  TTPM_CHECK(0, 50);
  TTPM_CHECK(TlclStartupIfNeeded(), 50);
  TTPM_CHECK(TlclContinueSelfTest(), 100);
  TTPM_CHECK(TlclSelfTestFull(), 1000);
  TTPM_CHECK(TlclAssertPhysicalPresence(), 100);
  TTPM_CHECK(TlclWrite(INDEX0, (uint8_t*) &x, sizeof(x)), 100);
  TTPM_CHECK(TlclRead(INDEX0, (uint8_t*) &x, sizeof(x)), 100);
  TTPM_CHECK(TlclExtend(0, in, out), 200);
  TTPM_CHECK(TlclSetGlobalLock(), 50);
  TTPM_CHECK(TlclLockPhysicalPresence(), 100);
  if (time_limit_exceeded || errors > 0) {
    printf("TEST FAILED\n");
    exit(1);
  } else {
    printf("TEST SUCCEEDED\n");
    return 0;
  }
}
