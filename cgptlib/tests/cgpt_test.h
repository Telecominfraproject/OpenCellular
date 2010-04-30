/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef VBOOT_REFERENCE_CGPT_TEST_H_
#define VBOOT_REFERENCE_CGPT_TEST_H_

#include <stdio.h>

enum {
  TEST_FAIL = -1,
  TEST_OK = 0,
};

#define TEST_CASE(func) #func, func
typedef int (*test_func)(void);

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

/* ANSI Color coding sequences. */
#define COL_GREEN "\e[1;32m"
#define COL_RED "\e[0;31m"
#define COL_STOP "\e[m"

#define EXPECT(expr) \
  do { \
    if (!(expr)) { \
      printf(COL_RED " fail " COL_STOP "in expression %s in %s() line %d\n",\
             #expr, __FUNCTION__, __LINE__); \
      return TEST_FAIL; \
    } \
  } while (0)

#endif  /* VBOOT_REFERENCE_CGPT_TEST_H_ */
