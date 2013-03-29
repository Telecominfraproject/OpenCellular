/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for statful_util functions.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "stateful_util.h"
#include "test_common.h"
#include "utility.h"
#include "vboot_common.h"


/* Test StatefulInit */
static void StatefulInitTest(void) {
  MemcpyState s;
  char buf[128];

  memset(&s, 0, sizeof(s));
  s.overrun = 1;
  StatefulInit(&s, buf, 128);
  TEST_EQ(0, s.overrun, "StatefulInit() overrun");
  TEST_EQ(128, s.remaining_len, "StatefulInit() len");
  TEST_PTR_EQ(buf, s.remaining_buf, "StatefulInit() buf");
}


/* Test StatefulSkip */
static void StatefulSkipTest(void) {
  MemcpyState s;
  char buf[128];

  /* Small skip */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(&s, StatefulSkip(&s, 5), "StatefulSkip(5) retval");
  TEST_EQ(128 - 5, s.remaining_len, "StatefulSkip(5) len");
  TEST_PTR_EQ(buf + 5, s.remaining_buf, "StatefulSkip(5) buf");
  TEST_EQ(0, s.overrun, "StatefulSkip(5) overrun");

  /* Use entire buffer */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(&s, StatefulSkip(&s, 128), "StatefulSkip(all) retval");
  TEST_EQ(0, s.remaining_len, "StatefulSkip(all) len");
  TEST_PTR_EQ(buf + 128, s.remaining_buf, "StatefulSkip(all) buf");
  TEST_EQ(0, s.overrun, "StatefulSkip(all) overrun");

  /* Zero-length skip is ok (but meaningless) */
  TEST_PTR_EQ(&s, StatefulSkip(&s, 0), "StatefulSkip(0) retval");
  TEST_EQ(0, s.remaining_len, "StatefulSkip(0) len");
  TEST_PTR_EQ(buf + 128, s.remaining_buf, "StatefulSkip(0) buf");
  TEST_EQ(0, s.overrun, "StatefulSkip(0) overrun");

  /* Can't use even one byte past that */
  TEST_PTR_EQ(NULL, StatefulSkip(&s, 1), "StatefulSkip(+1) retval");
  TEST_EQ(0, s.remaining_len, "StatefulSkip(+1) len");
  TEST_EQ(1, s.overrun, "StatefulSkip(+1) overrun");

  /* Overrun */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(NULL, StatefulSkip(&s, 256), "StatefulSkip(256) retval");
  TEST_EQ(1, s.overrun, "StatefulSkip(256) overrun");
  /* Once overrun, always overrun, even if we now ask for a small skip */
  TEST_PTR_EQ(NULL, StatefulSkip(&s, 1), "StatefulSkip(256+1) retval");
  TEST_EQ(1, s.overrun, "StatefulSkip(256+1) overrun");

  /* Overrun with potential wraparound */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(NULL, StatefulSkip(&s, -1), "StatefulSkip(-1) retval");
  TEST_EQ(1, s.overrun, "StatefulSkip(-1) overrun");
}


/* Test StatefulMemset_r */
static void StatefulMemset_rTest(void) {
  MemcpyState s;
  char buf[129];
  char want[129];

  memset(want, 0, sizeof(want));
  memset(buf, 0, sizeof(buf));

  /* Small sets */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(&s, StatefulMemset_r(&s, 'A', 5), "StatefulMemset_r(5) retval");
  TEST_EQ(128 - 5, s.remaining_len, "StatefulMemset_r(5) len");
  TEST_PTR_EQ(buf + 5, s.remaining_buf, "StatefulMemset_r(5) buf");
  /* Using strcmp() is a convenient way to check that we didn't
   * overwrite the 0-byte following what we expected to set. */
  TEST_EQ(0, strcmp("AAAAA", buf), "StatefulMemset_r(5) contents");
  TEST_EQ(0, s.overrun, "StatefulMemset_r(5) overrun");
  TEST_PTR_EQ(&s, StatefulMemset_r(&s, 'B', 3), "StatefulMemset_r(3) retval");
  TEST_EQ(0, strcmp("AAAAABBB", buf), "StatefulMemset_r(3) contents");

  /* Use entire buffer */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(&s, StatefulMemset_r(&s, 'C', 128),
              "StatefulMemset_r(all) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemset_r(all) len");
  TEST_PTR_EQ(buf + 128, s.remaining_buf, "StatefulMemset_r(all) buf");
  TEST_EQ(0, s.overrun, "StatefulMemset_r(all) overrun");
  memset(want, 'C', 128);
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemset_r(all) contents");

  /* Zero-length set is ok (but meaningless) */
  TEST_PTR_EQ(&s, StatefulMemset_r(&s, 'D', 0), "StatefulMemset_r(0) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemset_r(0) len");
  TEST_PTR_EQ(buf + 128, s.remaining_buf, "StatefulMemset_r(0) buf");
  TEST_EQ(0, s.overrun, "StatefulMemset_r(0) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemset_r(0) contents");

  /* Can't use even one byte past that */
  TEST_PTR_EQ(NULL, StatefulMemset_r(&s, 'E', 1),
              "StatefulMemset_r(+1) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemset_r(+1) len");
  TEST_EQ(1, s.overrun, "StatefulMemset_r(+1) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemset_r(+1) contents");

  /* Overrun */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(NULL, StatefulMemset_r(&s, 'F', 256),
              "StatefulMemset_r(256) retval");
  TEST_EQ(1, s.overrun, "StatefulMemset_r(256) overrun");
  /* Once overrun, always overrun, even if we now ask for a small skip */
  TEST_PTR_EQ(NULL, StatefulMemset_r(&s, 'G', 1),
              "StatefulMemset_r(256+1) retval");
  TEST_EQ(1, s.overrun, "StatefulMemset_r(256+1) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemset_r(+1) contents");

  /* Overrun with potential wraparound */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(NULL, StatefulMemset_r(&s, 'H', -1),
              "StatefulMemset_r(-1) retval");
  TEST_EQ(1, s.overrun, "StatefulMemset_r(-1) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemset_r(+1) contents");
}


/* Test StatefulMemcpy_r */
static void StatefulMemcpy_rTest(void) {
  MemcpyState s;
  char buf[129];
  char want[129];
  char* src1 = "Doogie";
  char* src2 = "Howserrr";
  char* src3 = "WholeBuffr";

  memset(want, 0, sizeof(want));
  memset(buf, 0, sizeof(buf));

  /* Small copies */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(src1, StatefulMemcpy_r(&s, src1, 6),
              "StatefulMemcpy_r(6) retval");
  TEST_EQ(128 - 6, s.remaining_len, "StatefulMemcpy_r(6) len");
  TEST_PTR_EQ(buf + 6, s.remaining_buf, "StatefulMemcpy_r(6) buf");
  /* Using strcmp() is a convenient way to check that we didn't
   * overwrite the 0-byte following what we expected to copy. */
  TEST_EQ(0, strcmp("Doogie", buf), "StatefulMemcpy_r(6) contents");
  TEST_EQ(0, s.overrun, "StatefulMemcpy_r(6) overrun");
  TEST_PTR_EQ(src2, StatefulMemcpy_r(&s, src2, 8),
              "StatefulMemcpy_r(8) retval");
  TEST_EQ(0, strcmp("DoogieHowserrr", buf), "StatefulMemcpy_r(8) contents");

  /* Use entire buffer */
  memset(buf, 42, sizeof(buf));
  StatefulInit(&s, buf, 10);
  TEST_PTR_EQ(src3, StatefulMemcpy_r(&s, src3, 10),
              "StatefulMemcpy_r(all) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemcpy_r(all) len");
  TEST_PTR_EQ(buf + 10, s.remaining_buf, "StatefulMemcpy_r(all) buf");
  TEST_EQ(0, s.overrun, "StatefulMemcpy_r(all) overrun");
  TEST_EQ(0, memcmp(src3, buf, 10), "StatefulMemcpy_r(all) contents");
  TEST_EQ(42, buf[10], "StatefulMemcpy_r(all) contents+1");

  /* Zero-length copy is ok (but meaningless) */
  TEST_PTR_EQ(src1, StatefulMemcpy_r(&s, src1, 0),
              "StatefulMemcpy_r(0) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemcpy_r(0) len");
  TEST_PTR_EQ(buf + 10, s.remaining_buf, "StatefulMemcpy_r(0) buf");
  TEST_EQ(0, s.overrun, "StatefulMemcpy_r(0) overrun");
  TEST_EQ(42, buf[10], "StatefulMemcpy_r(0) contents+1");

  /* Can't use even one byte past that */
  TEST_PTR_EQ(NULL, StatefulMemcpy_r(&s, src1, 1),
              "StatefulMemcpy_r(+1) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemcpy_r(+1) len");
  TEST_EQ(1, s.overrun, "StatefulMemcpy_r(+1) overrun");
  TEST_EQ(42, buf[10], "StatefulMemcpy_r(+1) contents");

  /* Overrun */
  memset(buf, 0, sizeof(buf));
  StatefulInit(&s, buf, 8);
  TEST_PTR_EQ(NULL, StatefulMemcpy_r(&s, "MoreThan8", 9),
              "StatefulMemcpy_r(9) retval");
  TEST_EQ(1, s.overrun, "StatefulMemcpy_r(9) overrun");
  /* Once overrun, always overrun, even if we now ask for a small skip */
  TEST_PTR_EQ(NULL, StatefulMemcpy_r(&s, "Less", 4),
              "StatefulMemcpy_r(9+1) retval");
  TEST_EQ(1, s.overrun, "StatefulMemcpy_r(9+1) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemcpy_r(+1) contents");

  /* Overrun with potential wraparound */
  StatefulInit(&s, buf, 128);
  TEST_PTR_EQ(NULL, StatefulMemcpy_r(&s, "FOO", -1),
              "StatefulMemcpy_r(-1) retval");
  TEST_EQ(1, s.overrun, "StatefulMemcpy_r(-1) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemcpy_r(+1) contents");
}


/* Test StatefulMemcpy */
static void StatefulMemcpyTest(void) {
  MemcpyState s;
  char buf[129];
  char want[129];
  char* src1 = "ThisIsATest";
  char* src2 = "ThisIsOnlyATest";

  memset(want, 0, sizeof(want));
  memset(buf, 0, sizeof(buf));

  /* Small copies */
  StatefulInit(&s, src1, 12);
  TEST_PTR_EQ(buf, StatefulMemcpy(&s, buf, 6), "StatefulMemcpy(6) retval");
  TEST_EQ(6, s.remaining_len, "StatefulMemcpy(6) len");
  TEST_PTR_EQ(src1 + 6, s.remaining_buf, "StatefulMemcpy(6) buf");
  /* Using strcmp() is a convenient way to check that we didn't
   * overwrite the 0-byte following what we expected to copy. */
  TEST_EQ(0, strcmp("ThisIs", buf), "StatefulMemcpy(6) contents");
  TEST_EQ(0, s.overrun, "StatefulMemcpy(6) overrun");
  TEST_PTR_EQ(buf, StatefulMemcpy(&s, buf, 5), "StatefulMemcpy(5) retval");
  /* Note that we shouldn't have copied the last byte out of the
   * stateful buffer, so we don't overwrite the last character of the
   * string that was in buf. */
  TEST_EQ(0, strcmp("ATests", buf), "StatefulMemcpy(5) contents");

  /* Use entire buffer */
  memset(buf, 1, sizeof(buf));
  StatefulInit(&s, src2, 16);
  TEST_PTR_EQ(buf, StatefulMemcpy(&s, buf, 16), "StatefulMemcpy(all) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemcpy(all) len");
  TEST_PTR_EQ(src2 + 16, s.remaining_buf, "StatefulMemcpy(all) buf");
  TEST_EQ(0, s.overrun, "StatefulMemcpy(all) overrun");
  TEST_EQ(0, strcmp(src2, buf), "StatefulMemcpy(all) contents");

  /* Zero-length copy is ok (but meaningless) */
  TEST_PTR_EQ(buf, StatefulMemcpy(&s, buf, 0),
              "StatefulMemcpy(0) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemcpy(0) len");
  TEST_PTR_EQ(src2 + 16, s.remaining_buf, "StatefulMemcpy(0) buf");
  TEST_EQ(0, s.overrun, "StatefulMemcpy(0) overrun");
  TEST_EQ(0, strcmp(src2, buf), "StatefulMemcpy(0) contents");

  /* Can't use even one byte past that */
  TEST_PTR_EQ(NULL, StatefulMemcpy(&s, buf, 1),
              "StatefulMemcpy(+1) retval");
  TEST_EQ(0, s.remaining_len, "StatefulMemcpy(+1) len");
  TEST_EQ(1, s.overrun, "StatefulMemcpy(+1) overrun");
  TEST_EQ(0, strcmp(src2, buf), "StatefulMemcpy(+1) contents");

  /* Overrun */
  memset(buf, 0, sizeof(buf));
  StatefulInit(&s, "Small", 5);
  TEST_PTR_EQ(NULL, StatefulMemcpy(&s, buf, 9), "StatefulMemcpy(9) retval");
  TEST_EQ(1, s.overrun, "StatefulMemcpy(9) overrun");
  /* Once overrun, always overrun, even if we now ask for a small skip */
  TEST_PTR_EQ(NULL, StatefulMemcpy(&s, buf, 4),
              "StatefulMemcpy(9+1) retval");
  TEST_EQ(1, s.overrun, "StatefulMemcpy(9+1) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemcpy(+1) contents");

  /* Overrun with potential wraparound */
  StatefulInit(&s, "Larger", 6);
  TEST_PTR_EQ(NULL, StatefulMemcpy(&s, buf, -1), "StatefulMemcpy(-1) retval");
  TEST_EQ(1, s.overrun, "StatefulMemcpy(-1) overrun");
  TEST_EQ(0, memcmp(want, buf, sizeof(want)), "StatefulMemcpy(+1) contents");
}


int main(int argc, char* argv[]) {
  int error_code = 0;

  StatefulInitTest();
  StatefulSkipTest();
  StatefulMemset_rTest();
  StatefulMemcpy_rTest();
  StatefulMemcpyTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
