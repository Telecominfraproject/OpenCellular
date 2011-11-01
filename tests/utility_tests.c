/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for utility functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "test_common.h"
#include "utility.h"
#include "vboot_common.h"


/* Test utility.h and sysincludes.h macros */
static void MacrosTest(void) {
  int64_t a = -10, b = -20;
  uint64_t u = UINT64_C(0xABCD00000000);
  uint64_t v = UINT64_C(0xABCD000000);

  TEST_EQ(CombineUint16Pair(1, 2), 0x00010002, "CombineUint16Pair");
  TEST_EQ(CombineUint16Pair(0xFFFE, 0xFFFF), 0xFFFEFFFF,
          "CombineUint16Pair 2");
  TEST_EQ(CombineUint16Pair(-4, -16), 0xFFFCFFF0,
          "CombineUint16Pair big negative");
  TEST_EQ(CombineUint16Pair(0x10003, 0x10004), 0x00030004,
          "CombineUint16Pair overflow");

  TEST_EQ(Min(1, 2), 1, "Min 1");
  TEST_EQ(Min(4, 3), 3, "Min 2");
  TEST_EQ(Min(5, 5), 5, "Min 5");
  TEST_EQ(Min(a, b), b, "Min uint64 1");
  TEST_EQ(Min(b, a), b, "Min uint64 2");
  TEST_EQ(Min(b, b), b, "Min uint64 same");

  TEST_EQ(UINT64_RSHIFT(u, 8), v, "UINT64_RSHIFT 8");
  TEST_EQ(UINT64_RSHIFT(u, 0), u, "UINT64_RSHIFT 0");
  TEST_EQ(UINT64_RSHIFT(u, 36), UINT64_C(0xABC), "UINT64_RSHIFT 36");

  TEST_EQ(UINT64_MULT32(v, 0), 0, "UINT64_MULT32 0");
  TEST_EQ(UINT64_MULT32(v, 1), v, "UINT64_MULT32 1");
  TEST_EQ(UINT64_MULT32(v, 256), u, "UINT64_MULT32 256");
}


/* Test SafeMemcmp */
static void SafeMemcmpTest(void) {
  /* Zero-length strings are equal */
  TEST_EQ(0, SafeMemcmp("APPLE", "TIGER", 0), "SafeMemcmp() size=0");

  /* Test equal arrays */
  TEST_EQ(0, SafeMemcmp("clonebob", "clonebob", 8), "SafeMemcmp() equal");
  /* Inequality past end of array doesn't affect result */
  TEST_EQ(0, SafeMemcmp("clonebob", "clonedan", 5), "SafeMemcmp() equal2");

  TEST_EQ(1, SafeMemcmp("APPLE", "TIGER", 5), "SafeMemcmp() unequal");
  TEST_EQ(1, SafeMemcmp("APPLE", "APPLe", 5), "SafeMemcmp() unequal 2");
}


/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))

int main(int argc, char* argv[]) {
  int error_code = 0;

  MacrosTest();
  SafeMemcmpTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
