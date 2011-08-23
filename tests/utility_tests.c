/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STUB_IMPLEMENTATION_  /* So we can use memset() ourselves */

#include "test_common.h"
#include "utility.h"
#include "vboot_common.h"


/* Test Memset */
static void MemsetTest(void) {
  char dest[128];
  char want[128];

  memset(want, 0, 128);
  memset(dest, 0, 128);

  /* Simple fill */
  memset(want, 123, 5);
  TEST_EQ(0, dest - (char*)Memset(dest, 123, 5), "Memset() returns dest");
  TEST_EQ(0, memcmp(dest, want, 128), "Memset()");

  /* Filling length 0 does nothing */
  Memset(dest, 42, 0);
  TEST_EQ(0, memcmp(dest, want, 128), "Memset() size=0");
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

  MemsetTest();
  SafeMemcmpTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
