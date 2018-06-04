/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Tests for string utility functions.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"
#include "utility.h"
#include "vboot_common.h"


/* Test string concatenation */
static void StrncatTest(void) {
  char dest[128];

  /* Null inputs */
  TEST_EQ(0, StrnAppend(dest, NULL, sizeof(dest)), "StrnAppend('', null)");
  TEST_EQ(0, StrnAppend(NULL, "Hey!", sizeof(dest)), "StrnAppend(null, '')");

  /* Empty <-- empty */
  *dest = 0;
  TEST_EQ(0, StrnAppend(dest, "", sizeof(dest)), "StrnAppend('', '')");
  TEST_EQ(0, strcmp(dest, ""), "StrnAppend('', '') result");

  /* Nonempty <-- empty */
  strcpy(dest, "Bob");
  TEST_EQ(3, StrnAppend(dest, "", sizeof(dest)), "StrnAppend(B, '')");
  TEST_EQ(0, strcmp(dest, "Bob"), "StrnAppend(B, '') result");

  /* Empty <-- nonempty */
  *dest = 0;
  TEST_EQ(5, StrnAppend(dest, "Alice", sizeof(dest)), "StrnAppend('', A)");
  TEST_EQ(0, strcmp(dest, "Alice"), "StrnAppend('', A) result");

  /* Nonempty <-- nonempty */
  strcpy(dest, "Tigre");
  TEST_EQ(10, StrnAppend(dest, "Bunny", sizeof(dest)), "StrnAppend(T, B)");
  TEST_EQ(0, strcmp(dest, "TigreBunny"), "StrnAppend(T, B) result");

  /* Test clipping */
  strcpy(dest, "YesI");
  TEST_EQ(7, StrnAppend(dest, "Can't", 8), "StrnAppend(Y, over)");
  TEST_EQ(0, strcmp(dest, "YesICan"), "StrnAppend(Y, over) result");

  /* Test clipping if dest already overflows its claimed length */
  strcpy(dest, "BudgetDeficit");
  TEST_EQ(6, StrnAppend(dest, "Spending", 7), "StrnAppend(over, over)");
  TEST_EQ(0, strcmp(dest, "Budget"), "StrnAppend(over, over) result");
}


static void TestU64ToS(uint64_t value, uint32_t radix, uint32_t zero_pad_width,
                       const char *expect) {
  char dest[UINT64_TO_STRING_MAX];

  TEST_EQ(strlen(expect),
          Uint64ToString(dest, sizeof(dest), value, radix, zero_pad_width),
          "Uint64ToString");
  printf("Uint64ToString expect %s got %s\n", expect, dest);
  TEST_EQ(0, strcmp(dest, expect), "Uint64ToString result");
}


/* Test uint64 to string conversion */
static void Uint64ToStringTest(void) {
  char dest[UINT64_TO_STRING_MAX];

  /* Test invalid inputs */
  TEST_EQ(0, Uint64ToString(NULL, 8, 123, 10, 8), "Uint64ToString null dest");
  TestU64ToS(0, 1, 0, "");
  TestU64ToS(0, 37, 0, "");

  /* Binary */
  TestU64ToS(0, 2, 0, "0");
  TestU64ToS(0x9A, 2, 0, "10011010");
  TestU64ToS(0x71, 2, 12, "000001110001");
  TestU64ToS(
      ~0ULL, 2, 0,
      "1111111111111111111111111111111111111111111111111111111111111111");

  /* Decimal */
  TestU64ToS(0, 10, 0, "0");
  TestU64ToS(12345, 10, 0, "12345");
  TestU64ToS(67890, 10, 8, "00067890");
  TestU64ToS(~0ULL, 10, 0, "18446744073709551615");

  /* Hex */
  TestU64ToS(0, 16, 0, "0");
  TestU64ToS(0x12345678, 16, 0, "12345678");
  TestU64ToS(0x9ABCDEF, 16, 8, "09abcdef");
  TestU64ToS(~0ULL, 16, 0, "ffffffffffffffff");

  /* Zero pad corner cases */
  /* Don't pad if over length */
  TestU64ToS(0x1234567890ULL, 16, 8, "1234567890");
  /* Fail if padding won't fit in buffer */
  TEST_EQ(0, Uint64ToString(dest, 8, 123, 10, 8), "Uint64ToString bad pad");
  TEST_EQ(0, strcmp(dest, ""), "Uint64ToString bad pad result");

}


int main(int argc, char* argv[]) {
  int error_code = 0;

  StrncatTest();
  Uint64ToStringTest();

  if (!gTestSuccess)
    error_code = 255;

  return error_code;
}
